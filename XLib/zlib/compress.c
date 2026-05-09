/* compress.c -- compress a memory buffer
 * Copyright (C) 1995-2003 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#define ZLIB_INTERNAL
#include "zlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ===========================================================================
     Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the byte
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

     compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/
int ZEXPORT compress2 (dest, destLen, source, sourceLen, doneSize, level)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
	unsigned long *doneSize;
    int level;
{
    z_stream strm;
    int err, flush, ret, destRemain;
    unsigned have;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
	unsigned int doneAmount;
	unsigned int pos, remaining;
	Bytef *destPos;

	destRemain = *destLen;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    err = deflateInit(&strm, level);
    if (err != Z_OK) return err;

	pos = 0;
	destPos = dest;
    do {
		flush = Z_NO_FLUSH;
		remaining = sourceLen - pos;
        strm.avail_in = CHUNK;
		if ( strm.avail_in >= remaining )
		{
			strm.avail_in = remaining;
			flush = Z_FINISH;
		}

		memcpy(in, source + pos, strm.avail_in);
        strm.next_in = in;
		doneAmount = (unsigned int)strm.avail_in;
		pos += doneAmount;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            have = CHUNK - strm.avail_out;
			destRemain -= have;
			if ( destRemain < 0 )
				return Z_NOTENOUGH_BUF;
			else
				memcpy(destPos, out, have);

			destPos += have;
        } while (strm.avail_out == 0);

		if ( doneSize )
			(*doneSize) += doneAmount;

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    *destLen = strm.total_out;

    err = deflateEnd(&strm);
    return err;
}

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int zlib_deflevel(FILE *source, FILE *dest, unsigned long *progress, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
	unsigned int doneAmount;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = (uInt)fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

		doneAmount = (unsigned int)strm.avail_in;
        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
//        assert(strm.avail_in == 0);     /* all input will be used */

		if ( progress )
			(*progress) += doneAmount;

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
//    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}
int zlib_def(FILE *source, FILE *dest, unsigned long *progress)
{
	return zlib_deflevel(source, dest, progress, Z_DEFAULT_COMPRESSION);
}


/* ===========================================================================
 */
int ZEXPORT compress (dest, destLen, source, sourceLen, doneSize)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
	unsigned long *doneSize;
{
    return compress2(dest, destLen, source, sourceLen, doneSize, Z_DEFAULT_COMPRESSION);
}

/* ===========================================================================
     If the default memLevel or windowBits for deflateInit() is changed, then
   this function needs to be updated.
 */
uLong ZEXPORT compressBound (sourceLen)
    uLong sourceLen;
{
    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + 11;
}


