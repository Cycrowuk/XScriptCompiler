#ifndef __FILE_IO_H__
#define __FILE_IO_H__

#pragma warning( push )
#pragma warning( disable : 4251)

#include "Utils/String.h"
#include "Utils/WStringList.h"
#include <fstream>
#include <vector>
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

class C_File;

enum {FILEERR_NONE, FILEERR_NOFILE, FILEERR_NOOPEN, FILEERR_TOSMALL, FILEERR_NOWRITE};

class SPKEXPORT CFileIO
{
public:
	static bool Remove(const Utils::WString &filename);
	static bool Exists(const Utils::WString& filename);
	static bool Rename(const Utils::WString& from, const Utils::WString& to);

public:
	CFileIO();
	CFileIO(const Utils::WString &sFilename, bool bBinary);
	CFileIO(const Utils::WString &filename);
	CFileIO(C_File *file);
	~CFileIO ();

	bool startWrite();
	bool startRead();
	bool startModify();
	bool startAppend();
	bool atEnd() const;

	size_t fileSize() const;

	time_t modifiedTime() const;
	time_t creationTime() const;
	void setCreationTime(time_t time);

	bool writeSize(size_t iSize);
	bool write(CFileIO &file, size_t iSize);
	bool write(const unsigned char *buf, ...);
	bool write(const char *buf, ...);
	bool write(const char *buf, size_t iSize);
	bool write(const unsigned char *buf, size_t iSize);
	bool put(const unsigned char c);

	void close();

	Utils::String readEndOfLineStr();
	Utils::WString readEndOfLine();

	void seek(size_t iPos);
	void seekEnd(size_t iPos = 0);
	void seekStart(size_t iPos = 0);
	size_t position();

	int readSize();
	bool read(unsigned char* buf, size_t iSize, bool bEndChar = false);
	bool read(wchar_t* buf, size_t iSize, bool bEndChar = false);
	unsigned char *read(size_t iAmount);
	unsigned char *readAll(size_t *pSize = NULL);

	std::fstream &stream();

	size_t readLines(std::vector<Utils::String>& to) const;
	size_t readLines(std::vector<Utils::WString>& to) const;
	size_t readLinesUTF(std::vector<Utils::WString>& to) const;
	size_t readLines(Utils::WStringList& to) const;

	bool writeFile(std::vector<Utils::String>* lines);
	bool writeFile(const std::vector<Utils::WString>& lines) const;
	bool writeFile(Utils::WStringList* lines);
	bool writeFileUTF(const std::vector<Utils::WString>* lines);
	bool writeFileUTF(const Utils::WStringList &lines);

	bool remove();
	bool Rename(const Utils::WString &toFile);
	bool copy(const Utils::WString &toFile, bool keepTime = false);

	bool isOpened() const;
	bool open(const Utils::WString& sFilename, bool bBinary = true);

	Utils::WString baseName() const;
	const Utils::WString& fullFilename() const;
	const Utils::WString& filename() const;
	const Utils::WString& dir() const { return _sDirIO.dir(); }

	const CDirIO& dirIO() const { return _sDirIO; }
	CDirIO &GetDirIO() { return _sDirIO; }

	bool NoFile () { return _sFilename.empty(); }
	size_t GetFilesize () { return m_lSize; }

	char *ReadToData ( size_t *size );
	bool WriteData ( const char *data, size_t size );
	bool writeString ( const Utils::String &data );

	bool WritePartFile ( size_t *offsets, size_t numOffset );
	int TruncateFile ( size_t offset, size_t datasize );
	bool WipeFile();
	
	bool exists() const;
	bool appendFile ( const Utils::String &filename );
	bool AppendData ( const char *d, size_t size );
	bool AppendDataToPos ( const char *d, size_t size, size_t start );

	bool isFileExtension(const Utils::WString &ext) { return (extension().Compare(ext)) ? true : false; }

	Utils::WString extension() const;
	Utils::WString getWindowsFilename() const;
	Utils::WString changeFileExtension(const Utils::WString &ext) const;

	void setDir(const Utils::WString &dir);
	void setAutoDelete(bool bDelete);

private:
	int _in() const;
	int _out() const;
	int _append() const;
	int _inout() const;
	bool _start(int iFlags, bool bSeekP);

	void _readFileSize ();
	void _seek(int iPos, int iFrom);

	bool _write(const char* buf, va_list args);
	bool _write(const wchar_t* buf, va_list args);

	void _updateFilename();

	std::string _fileData() const;
	static std::string _toFileData(const Utils::WString &str);

private:
	Utils::WString	_sFilename;
	Utils::WString	_sFile;
	Utils::WString	_sExt;
	CDirIO			_sDirIO;

	bool m_bSeekP;
	bool m_bBinary;
	bool m_bAutoDelete;

	std::fstream m_fId;

	size_t m_lSize;
};

#pragma warning( pop ) 
#endif //__FILE_IO_H__

