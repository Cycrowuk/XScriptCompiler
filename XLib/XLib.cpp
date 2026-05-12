// XLib.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"

#include "XLib.h"
#include "zlib/zlib.h"
#include "unzip.h"
#include "FileIO.h"

#define PCKHEADERSIZE 10
#define GZ_FLAG_TEXT     1     // 0
#define GZ_FLAG_HCRC     2     // 1
#define GZ_FLAG_EXTRA    4     // 2
#define GZ_FLAG_FILENAME 8     // 3
#define GZ_FLAG_COMMENT  16    // 4
#define GZ_FLAG_RES1     32    // 5
#define GZ_FLAG_RES2     64    // 6
#define GZ_FLAG_RES3     128   // 7
using namespace XLib;

unsigned char* XLib::CompressPCKData(unsigned char* buffer, size_t size, size_t* retsize, time_t mtime)
{
	size_t newsize = (size * 2) + 20;
	unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * newsize);

	z_stream zs;
	char flags = 0;

	//	error(0);

	unsigned char* d = data;
	//	if(m_pszComment && strlen(m_pszComment) > 0) flags|=GZ_F_COMMENT;
	//	if(m_pszFileName && strlen(m_pszFileName) > 0) flags|=GZ_F_FILENAME;

	int pos = PCKHEADERSIZE;
	*d = 0x1F; 		d++;
	*d = 0x8B; 		d++;
	*d = 8;			d++;
	*d = flags; 	d++;
	memcpy(d, &mtime, sizeof(mtime));
	d += 4;
	*d = 0; 		d++;
	*d = 11; 		d++;
	//	if(flags & GZ_F_FILENAME) put((const char *)m_pszFileName);
	//	if(flags & GZ_F_COMMENT) put((const char *)m_pszComment);

	memset(&zs, 0, sizeof(zs));
	zs.next_in = buffer;
	zs.avail_in = (long)size;

	int ret;
	unsigned long ubound;
	ret = deflateInit2(&zs, 9, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
	if (ret != Z_OK)
		return NULL;

	ubound = deflateBound(&zs, (unsigned long)size);
	if (newsize < ubound)
	{
		newsize += ubound;
		data = (unsigned char*)realloc(data, sizeof(unsigned char) * newsize);
	}


	zs.next_out = d;
	zs.avail_out = (unsigned int)newsize - pos;

	while ((ret = deflate(&zs, Z_FINISH)) == Z_OK)
	{
		newsize += 1024;
		data = (unsigned char*)realloc(data, sizeof(unsigned char) * newsize);
		zs.next_out = data + zs.total_out;
		zs.avail_out = (unsigned int)newsize - zs.total_out;
	}
	pos += zs.total_out;

	deflateEnd(&zs);

	unsigned long crc = crc32(0, NULL, 0);
	crc = crc32(crc, buffer, (unsigned int)size);

	int s = sizeof(crc) + sizeof(size);
	if (newsize < (size_t)(s + pos))
	{
		newsize += (s + pos) - newsize;
		data = (unsigned char*)realloc(data, sizeof(unsigned char) * newsize);
	}

	memcpy(&data[pos], &crc, sizeof(crc));
	pos += sizeof(crc);
	unsigned long lSize = static_cast<unsigned long>(size);
	memcpy(&data[pos], &lSize, sizeof(lSize));
	pos += sizeof(lSize);

	newsize = pos;

	unsigned char* retdata = NULL;
	if (ret == Z_STREAM_END)
	{
		*retsize = newsize;
		retdata = new unsigned char[newsize];
		memcpy(retdata, data, newsize);
	}
	free(data);

	return retdata;
}

unsigned char* XLib::PCKData(unsigned char* data, size_t oldsize, size_t* newsize, bool bXor)
{
	unsigned char* newdata = CompressPCKData(data, oldsize, newsize, time(NULL));
	if (!bXor)
		return newdata;

	if (newdata)
	{
		char magic = (char)clock(), m;
		m = magic ^ 0xC8;

		unsigned char* ptr = newdata, * end = newdata + *newsize;
		// XOR encryption
		if (bXor)
		{
			for (; ptr < end; ptr++)
				(*ptr) ^= magic;
		}

		unsigned char* finalData = new unsigned char[*newsize + 1];
		finalData[0] = m;
		memcpy(finalData + 1, newdata, *newsize);
		delete[] newdata;
		(*newsize)++;
		return finalData;
	}

	return NULL;
}

unsigned char* XLib::UnPCKFile(const char* file, size_t* len, bool nocrypt)
{
	XLib::FileIO f(file);

	if (!f.startRead()) return NULL;

	size_t size;
	char* data = f.readAll(&size);
	f.close();

	if (data) {
		unsigned char* unData = UnPCKData((unsigned char *)data, size, len, nocrypt);
		delete data;
		return unData;
	}

	return NULL;
}

unsigned char* XLib::UnPCKData(unsigned char* data, size_t datasize, size_t* len) { return UnPCKData(data, datasize, len, IsDataPCK(data, datasize)); }
unsigned char* XLib::UnPCKData(unsigned char* data, size_t datasize, size_t* len, bool nocrypt)
{
	bool isPCK = IsDataPCK(data, datasize);

	unsigned char* newData = data;
	unsigned char* tempData = NULL;

	if (nocrypt)
	{
		tempData = new unsigned char[datasize];
		newData = tempData;
		memcpy(newData, data, datasize);
		unsigned char magic = newData[0] ^ 0xC8;

		for (size_t i = 1; i < datasize; i++)
			newData[i] ^= magic;
		++newData;
		--datasize;
	}

	size_t* uncomprLenSize = (size_t*)(newData + (datasize - 4));
	unsigned long uncomprLen = (unsigned long)*uncomprLenSize;
	if (uncomprLen > (datasize * 100))
	{
		if (tempData) delete[]tempData;
		*len = 0;
		return NULL;
	}
	unsigned char* uncompr = new unsigned char[uncomprLen + 1];
	if (!uncompr) {
		if (tempData) delete[]tempData;
		return NULL;
	}
	memset(uncompr, 0, sizeof(uncompr));


	// find header size
	unsigned char* buf = newData + PCKHEADERSIZE;

	//	buf = data + (6 + sizeof(time_t));
	char flag = newData[3];

	if (flag & GZ_FLAG_EXTRA)
	{
		size_t xlen = *((short int*)(buf));
		buf += xlen;
	}

	if (flag & GZ_FLAG_FILENAME)
	{
		char* origname = (char*)(buf);
		buf += strlen(origname) + 1;
	}
	if (flag & GZ_FLAG_COMMENT)
	{
		char* comment = (char*)(buf);
		buf += strlen(comment) + 1;
	}
	if (flag & GZ_FLAG_HCRC)
		buf += 2;
	long bufSize = (long)(datasize - (buf - newData) - 8);

	int err = uncompress2(uncompr, &uncomprLen, buf, bufSize);
	if (err != Z_OK)
	{
		if (tempData) delete[]tempData;
		delete uncompr;
		*len = 0;
		return NULL;
	}

	*len = uncomprLen;
	uncompr[uncomprLen] = '\0';
	if (tempData) delete[]tempData;
	return uncompr;
}

bool XLib::IsDataPCK(const unsigned char* data, size_t size)
{
	if (size >= 3)
	{
		unsigned char magic = data[0] ^ 0xC8;
		return ((data[1] ^ magic) == 0x1F && (data[2] ^ magic) == 0x8B);
	}
	else
		return false;

}
