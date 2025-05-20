#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>

#include <nbdkit-filter.h>
#include <zstd.h>

#include "cleanup.h"
#include "poll.h"
#include "minmax.h"
#include "utils.h"

int unzstd_debug_flag;
size_t bufInSize;
size_t bufOutSize;
void *bufIn;
void *bufOut;
ZSTD_DCtx *dctx;
FILE *fdebug;

static const char *entry; /* File within tar (tar-entry=...) */
static int64_t tar_limit = 0;
static const char *tar_program = "tar";

/* Offset and size within tarball.
 *
 * These are calculated once in the first connection that calls
 * tar_prepare.  They are protected by the lock.
 */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static bool initialized = false;
static uint64_t tar_offset, tar_size;

int zstd_init() {
  bufInSize = ZSTD_DStreamInSize();
  bufOutSize = ZSTD_DStreamOutSize();
  dctx = ZSTD_createDCtx();
  if (dctx == NULL)
    return 1;
  bufIn = malloc(bufInSize);
  if (bufIn == NULL)
    return 1;
  bufOut = malloc(bufOutSize);
  if (bufOut == NULL)
    return 1;
  fdebug = fopen("/home/evgen/work/nbdkit/build/filters/unzstd/unzstd.out", "w");
  if (!fdebug)
    return 1;
  return 0;
}

int zstd_finilize() {
  ZSTD_freeDCtx(dctx);
  free(bufIn);
  free(bufOut);
  fclose(fdebug);
  return 0;
}

void zstd_decompress() {}

static int tar_config(nbdkit_next_config *next, nbdkit_backend *nxdata, const char *key, const char *value) {
  if (strcmp(key, "tar-entry") == 0) {
    if (entry) {
      nbdkit_error("only one tar-entry parameter can be given");
      return -1;
    }
    entry = value;
    return 0;
  } else if (strcmp(key, "tar-limit") == 0) {
    tar_limit = nbdkit_parse_size(value);
    if (tar_limit == -1)
      return -1;
    return 0;
  } else if (strcmp(key, "tar") == 0) {
    tar_program = value;
    return 0;
  }

  return next(nxdata, key, value);
}

static int tar_config_complete(nbdkit_next_config_complete *next, nbdkit_backend *nxdata) {
  if (entry == NULL) {
    nbdkit_error("you must supply the tar-entry=<FILENAME> parameter");
    return -1;
  }

  return next(nxdata);
}

#define tar_config_help                                                                                                \
  "tar-entry=<FILENAME> (required) The path inside the tar file to serve.\n"                                           \
  "tar-limit=SIZE                  Limit on reading to find entry.\n"                                                  \
  "tar=<PATH>                      Path of the tar binary."

struct handle {
  /* These are copied from the globals during tar_prepare, so that we
   * don't have to keep grabbing the lock on each request.
   */
  uint64_t offset, size;
};

static void *tar_open(nbdkit_next_open *next, nbdkit_context *nxdata, int readonly, const char *exportname,
                      int is_tls) {
  if (zstd_init() != 0) {
    nbdkit_error("zstd_init");
    return NULL;
  }

  struct handle *h;
  if (next(nxdata, readonly, exportname) == -1)
    return NULL;

  h = calloc(1, sizeof *h);
  if (h == NULL) {
    nbdkit_error("calloc: %m");
    return NULL;
  }
  if (unzstd_debug_flag)
    nbdkit_debug("open");
  return h;
}

static void tar_close(void *handle) {
  zstd_finilize();
  if (unzstd_debug_flag)
    nbdkit_debug("close");
  free(handle);
}

/* Calculate the offset of the entry within the tarball.  This is
 * called with the lock held.  The method used is described here:
 * https://www.redhat.com/archives/libguestfs/2020-July/msg00017.html
 */
static int calculate_offset_of_entry(nbdkit_next *next) {
  const size_t bufsize = 65536;
  char output[] = "/tmp/tarXXXXXX";
  int fd;
  FILE *fp;
  CLEANUP_FREE char *cmd = NULL;
  size_t cmdlen = 0;
  CLEANUP_FREE char *buf = NULL;
  int64_t i, copysize;
  bool scanned_ok = false;

  assert(entry);

  /* Temporary file to capture the output from the tar command. */
  fd = mkstemp(output);
  if (fd == -1) {
    nbdkit_error("mkstemp: %m");
    return -1;
  }
  close(fd);

  /* Construct the tar command to examine the tar file. */
  fp = open_memstream(&cmd, &cmdlen);
  if (fp == NULL) {
    nbdkit_error("open_memstream: %m");
    return -1;
  }
  /* https://listman.redhat.com/archives/libguestfs/2021-April/msg00072.html */
  fprintf(fp, "LANG=C ");
  shell_quote(tar_program, fp);
  fprintf(fp, " --no-auto-compress -t --block-number -v -f - ");
  shell_quote(entry, fp);
  fprintf(fp, " > ");
  shell_quote(output, fp);
  /* Unfortunately we have to hide stderr since we are
   * expecting tar to warn:
   *   tar: Unexpected EOF in archive
   *   tar: Error is not recoverable: exiting now
   * when we close the connection abruptly.
   */
  fprintf(fp, " 2>/dev/null");
  if (fclose(fp) == EOF) {
    nbdkit_error("memstream failed: %m");
    return -1;
  }

  /* Prepare the copy buffer and copy size. */
  buf = malloc(bufsize);
  if (buf == NULL) {
    nbdkit_error("malloc: %m");
    return -1;
  }
  copysize = next->get_size(next);
  if (copysize == -1)
    return -1;
  if (tar_limit > 0 && copysize > tar_limit)
    copysize = tar_limit;

  /* Run the tar command. */
  nbdkit_debug("%s", cmd);
  fp = popen(cmd, "w");
  if (fp == NULL) {
    nbdkit_error("tar: %m");
    return -1;
  }

  /* Now loop, writing data from the plugin (the tar file) until we
   * detect that tar has written something to the output file or we
   * run out of plugin.  We're making the assumption that the plugin
   * is not going to be sparse, which is probably true of most tar
   * files.
   */
  for (i = 0; i < copysize; i += bufsize) {
    int err, r;
    const int64_t count = MIN(bufsize, copysize - i);
    int64_t j;
    struct stat statbuf;

    r = next->pread(next, buf, count, i, 0, &err);
    if (r == -1) {
      errno = err;
      nbdkit_error("pread: %m");
      pclose(fp);
      return -1;
    }
    for (j = 0; j < count;) {
      size_t written = fwrite(&buf[j], 1, count - j, fp);
      if (written == 0) {
        nbdkit_error("tar: error writing to subprocess");
        pclose(fp);
        return -1;
      }
      j += written;
    }

    /* Did we get something in the output file yet? */
    if (stat(output, &statbuf) == 0 && statbuf.st_size > 0)
      break;
  }
  pclose(fp);

  /* Open the tar output and try to parse it. */
  fp = fopen(output, "r");
  if (fp == NULL) {
    nbdkit_error("%s: %m", output);
    return -1;
  }
  scanned_ok = fscanf(fp, "block %" SCNu64 ": %*s %*s %" SCNu64, &tar_offset, &tar_size) == 2;
  fclose(fp);
  unlink(output);

  if (!scanned_ok) {
    nbdkit_error("tar subcommand failed, "
                 "check that the file really exists in the tarball");
    return -1;
  }

  /* Adjust the offset: Add 1 for the tar header, then multiply by the
   * block size.
   */
  tar_offset = (tar_offset + 1) * 512;

  nbdkit_debug("tar: %s found at offset %" PRIu64 ", size %" PRIu64, entry, tar_offset, tar_size);

  /* Check it looks sensible.  XXX We ought to check it doesn't exceed
   * the size of the tar file.
   */
  if (tar_offset >= INT64_MAX || tar_size >= INT64_MAX) {
    nbdkit_error("internal error: calculated offset and size are wrong");
    return -1;
  }

  initialized = true;

  return 0;
}

static int tar_prepare(nbdkit_next *next, void *handle, int readonly) {
  struct handle *h = handle;
  ACQUIRE_LOCK_FOR_CURRENT_SCOPE(&lock);

  if (!initialized) {
    if (calculate_offset_of_entry(next) == -1)
      return -1;
  }

  assert(initialized);
  assert(tar_offset > 0);
  h->offset = tar_offset;
  h->size = tar_size;
  return 0;
}

/* Description. */
static const char *tar_export_description(nbdkit_next *next, void *handle) {
  const char *base = next->export_description(next);

  if (!base)
    return NULL;
  return nbdkit_printf_intern("embedded %s from within tar file: %s", entry, base);
}

/* Get the file size. */
static int64_t tar_get_size(nbdkit_next *next, void *handle) {
  struct handle *h = handle;
  int64_t size;

  /* We must call underlying get_size even though we don't use the
   * result, because it caches the plugin size in server/backend.c.
   */
  size = next->get_size(next);
  if (unzstd_debug_flag)
    nbdkit_debug("size=%lu h->size=%lu", size, h->size);

  if (size == -1)
    return -1;

//   return h->size;
  return size;
}

/* Read data from the file. */
static int tar_pread(nbdkit_next *next, void *handle, void *buf, uint32_t count, uint64_t offs, uint32_t flags,
                     int *err) {
  struct handle *h = handle;
  return next->pread(next, buf, count, offs + h->offset, flags, err);
}

/* Write data to the file. */
static int tar_pwrite(nbdkit_next *next, void *handle, const void *buf, uint32_t count, uint64_t offs, uint32_t flags,
                      int *err) {
  struct handle *h = handle;
  if (unzstd_debug_flag) {
    nbdkit_debug("count %u", count);
    char *tmp = strndup(buf, count);
    nbdkit_debug("buf %s", tmp);
    free(tmp);
  }

  if (unzstd_debug_flag)
    nbdkit_debug("count=%u offs=%lu h->offset=%lu h->size=%lu", count, offs, h->offset, h->size);

//   zstd_decompress();
  ZSTD_inBuffer input = {buf, count, 0};
//   while (input.pos < input.size) {
    if (unzstd_debug_flag)
      nbdkit_debug("input.pos=%u input.size=%lu h->offset=%lu h->size=%lu", count, offs, h->offset, h->size);
    ZSTD_outBuffer output = {bufOut, bufOutSize, 0};
    size_t const ret = ZSTD_decompressStream(dctx, &output, &input);
    if (unzstd_debug_flag)
      nbdkit_debug("bufOut->size=%lu bufOutSize=%lu", bufOut->size, bufOutSize);
    fwrite(bufOut, 1, output.pos, fdebug);
    return next->pwrite(next, bufOut, count, offs, flags, err);
//   }

//   if (unzstd_debug_flag)
//     nbdkit_debug("call next->pwrite count=%u", count);
//   return next->pwrite(next, buf, count, offs + h->offset, flags, err);
}

/* Trim data. */
static int tar_trim(nbdkit_next *next, void *handle, uint32_t count, uint64_t offs, uint32_t flags, int *err) {
  struct handle *h = handle;
  return next->trim(next, count, offs + h->offset, flags, err);
}

/* Zero data. */
static int tar_zero(nbdkit_next *next, void *handle, uint32_t count, uint64_t offs, uint32_t flags, int *err) {
  struct handle *h = handle;
  return next->zero(next, count, offs + h->offset, flags, err);
}

/* Extents. */
static int tar_extents(nbdkit_next *next, void *handle, uint32_t count, uint64_t offs, uint32_t flags,
                       struct nbdkit_extents *extents, int *err) {
  struct handle *h = handle;
  size_t i;
  CLEANUP_EXTENTS_FREE struct nbdkit_extents *extents2 = NULL;
  struct nbdkit_extent e;

  extents2 = nbdkit_extents_new(offs + h->offset, h->offset + h->size);
  if (extents2 == NULL) {
    *err = errno;
    return -1;
  }
  if (next->extents(next, count, offs + h->offset, flags, extents2, err) == -1)
    return -1;

  for (i = 0; i < nbdkit_extents_count(extents2); ++i) {
    e = nbdkit_get_extent(extents2, i);
    e.offset -= h->offset;
    if (nbdkit_add_extent(extents, e.offset, e.length, e.type) == -1) {
      *err = errno;
      return -1;
    }
  }
  return 0;
}

/* Cache data. */
static int tar_cache(nbdkit_next *next, void *handle, uint32_t count, uint64_t offs, uint32_t flags, int *err) {
  struct handle *h = handle;
  return next->cache(next, count, offs + h->offset, flags, err);
}

static struct nbdkit_filter filter = {
    .name = "unzstd",
    //   .longname           = "nbdkit unzstd filter",
    //   .config             = tar_config,
    //   .config_complete    = tar_config_complete,
    //   .config_help        = tar_config_help,
    .open = tar_open,
    .close = tar_close,
    //   .prepare            = tar_prepare,
    //   .export_description = tar_export_description,
    .get_size = tar_get_size,
    //   .pread = tar_pread,
    .pwrite = tar_pwrite,
    //   .trim               = tar_trim,
    //   .zero               = tar_zero,
    //   .extents            = tar_extents,
    //   .cache              = tar_cache,
};

NBDKIT_REGISTER_FILTER(filter)
