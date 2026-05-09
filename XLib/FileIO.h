#ifndef __FILE_IO_H__
#define __FILE_IO_H__

#pragma warning( push )
#pragma warning( disable : 4251)

#include <fstream>
#include "DirIO.h"

#include <time.h>

#if defined(_WIN32) || defined(OS2) || defined(MSDOS)
#include <fcntl.h>
#include <io.h>
#define MY_SET_BINARY_MODE(file) setmode(fileno(file),O_BINARY)
#else
#define MY_SET_BINARY_MODE(file)
#endif

#define EPOCH_DIFF 0x019DB1DED53E8000LL /* 116444736000000000 nsecs */
#define RATE_DIFF 10000000 /* 100 nsecs */

namespace XLib
{

	class C_File;

	enum { FILEERR_NONE, FILEERR_NOFILE, FILEERR_NOOPEN, FILEERR_TOSMALL, FILEERR_NOWRITE };

	class SPKEXPORT FileIO
	{
	public:
		static bool Remove(const XLib::String& filename);
		static bool Exists(const XLib::String& filename);

	public:
		FileIO();
		FileIO(const XLib::String& sFilename, bool bBinary = true);
		FileIO(const wchar_t* sFilename, bool bBinary = true);
		FileIO(const char* sFilename, bool bBinary = true);
		~FileIO();

		bool startWrite();
		bool startRead();
		bool startModify();
		bool startAppend();
		bool atEnd() const;

		size_t fileSize() const;

		time_t modifiedTime();
		time_t creationTime();
		void setCreationTime(time_t time);

		bool writeSize(size_t iSize);
		bool write(FileIO& file, size_t iSize);
		bool write(const char* buf, ...);
		bool write(const char* buf, size_t iSize);
		bool put(const char c);

		void close();

		XLib::String readEndOfLine();

		void seek(size_t iPos);
		void seekEnd(size_t iPos = 0);
		void seekStart(size_t iPos = 0);
		size_t position();

		int readSize();
		bool read(char* buf, size_t iSize, bool bEndChar = false);
		char* read(size_t iAmount);
		char* readAll(size_t* pSize = NULL);

		std::fstream& stream();

		StringList readLines() const;
		void readLines(StringList &files) const;

		bool writeFile(const StringList &lines) const;
		bool writeFileUTF(const StringList& lines) const;

		bool remove();
		bool rename(const XLib::String &toFile);
		bool copy(const XLib::String& toFile, bool keepTime = false);

		bool isOpened() const;
		bool open(const XLib::String& sFilename, bool bBinary = true);

		XLib::String baseName() const;
		const XLib::String& fullFilename() const { return m_sFilename; }
		const XLib::String& filename() const { return m_sFile; }
		const XLib::String& dir() const { return m_sDirIO.dir(); }
		DirIO& dirIO() { return m_sDirIO; }

		bool noFile() { return m_sFilename.empty(); }

		char* readToData(size_t* size);
		bool WriteData(const char* data, size_t size);
		bool writeString(const XLib::String& data);
		bool writeLine(const XLib::String& data);

		bool WritePartFile(size_t* offsets, size_t numOffset);
		int TruncateFile(size_t offset, size_t datasize);
		bool WipeFile();

		bool exists() const;
		bool ExistsOld() const;
		bool appendFile(const XLib::String& filename);
		bool appendData(const char* d, size_t size);
		bool appendDataToPos(const char* d, size_t size, size_t start);

		bool isFileExtension(const XLib::String& ext) { return (extension().Compare(ext)) ? true : false; }

		XLib::String extension();
		XLib::String windowsFilename();
		XLib::String changeFileExtension(const XLib::String& ext) const;

		void setDir(const XLib::String& dir);
		void setAutoDelete(bool bDelete);

	private:
		int _in() const;
		int _out() const;
		int _append() const;
		int _inout() const;
		bool _start(int iFlags, bool bSeekP);

		void _readFileSize();
		void _seek(size_t iPos, size_t iFrom);

		bool _write(const char* buf, va_list args);

		std::string _convertFilename(const XLib::String& str) const;

	private:
		XLib::String m_sFilename;
		XLib::String m_sFile;
		DirIO   m_sDirIO;

		bool m_bSeekP;
		bool m_bBinary;
		bool m_bAutoDelete;

		std::fstream m_fId;

		size_t m_lSize;
	};

}

#pragma warning( pop ) 
#endif //__FILE_IO_H__

