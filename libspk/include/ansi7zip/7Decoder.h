/* 7zDecode.h */

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "../lzma/Types.h"
#include "../spkdll.h"

#include <stdio.h>

SPKEXPORT unsigned char *LZMADecode_C ( unsigned char *inBuffer, size_t inSize, size_t *outSizeProcessed, int *result );

#endif

