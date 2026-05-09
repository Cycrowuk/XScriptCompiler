#pragma once
#define SPKEXPORT

#include "String.h"
#include "Log.h"
#include "DirIO.h"
#include "FileIO.h"
#include "TextDB.h"

namespace XLib
{
	bool IsDataPCK(const unsigned char* data, size_t size);
	unsigned char SPKEXPORT* UnPCKData(unsigned char* data, size_t datasize, size_t* len, bool nocrypt);
	unsigned char SPKEXPORT* UnPCKFile(const char* file, size_t* len, bool nocrypt);
	unsigned char SPKEXPORT* UnPCKData(unsigned char* data, size_t datasize, size_t* len);
	unsigned char* CompressPCKData(unsigned char* buffer, size_t size, size_t* retsize, time_t mtime);
	unsigned char* PCKData(unsigned char* data, size_t oldsize, size_t* newsize, bool bXor);
}
