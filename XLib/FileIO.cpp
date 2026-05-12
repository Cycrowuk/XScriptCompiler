#include "pch.h"
#include "FileIO.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <locale>
#include "DirIO.h"
#include "Log.h"

using namespace XLib;

FileIO::FileIO () : m_lSize(0), m_bBinary(true), m_bAutoDelete(false), m_bSeekP(false)
{
}

FileIO::FileIO(const XLib::String &sFilename, bool bBinary) : m_lSize(0), m_bBinary(bBinary), m_bAutoDelete(false)
{
	open(sFilename, bBinary);
}

FileIO::FileIO(const wchar_t* sFilename, bool bBinary) : m_lSize(0), m_bBinary(bBinary), m_bAutoDelete(false)
{
	open(sFilename, bBinary);
}
FileIO::FileIO(const char* sFilename, bool bBinary) : m_lSize(0), m_bBinary(bBinary), m_bAutoDelete(false)
{
	open(sFilename, bBinary);
}


FileIO::~FileIO()
{
	if ( this->isOpened() ) m_fId.close();
	if ( m_bAutoDelete ) this->remove();
}

void FileIO::setAutoDelete(bool bDelete)
{
	m_bAutoDelete = true;
}

bool FileIO::open(const XLib::String &sFilename, bool bBinary)
{
	m_bBinary = bBinary;
	m_sFilename = sFilename.asFilename();

	// check if there are any directories, and split the file and directory parts
	if ( m_sFilename.isin('/') ) {
		m_sDirIO.setDir(m_sFilename.tokens("/", 1, -2));
		m_sFile = m_sFilename.token("/", -1);
	}
	else
		m_sFile = m_sFilename;

	m_sFile.removeFirstSpace();
	m_sFile.removeChar(13);

	this->_readFileSize();
	return true;
}

char *FileIO::read(size_t iAmount)
{
	if ( !this->isOpened() ) startRead();
	if ( !this->isOpened() ) return NULL;

	if ( iAmount > m_lSize ) iAmount = m_lSize;

	char *data;
	try {
		data = new char[iAmount + 1];
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "ERROR: FileIO::read(size_t) unable to malloc data, %d (%s)", iAmount + 1, e.what());
		return NULL;
	}

	if ( !read(data, iAmount, true) ) {
		CLog::logf(CLog::LogType::IO, 2, "ERROR: FileIO::read(size_t) unable to read data from file, %s/%d", m_sFilename.c_str(), iAmount);
		delete []data;
		return NULL;
	}

	return data;
}

char *FileIO::readAll(size_t *pSize)
{
	if ( pSize ) (*pSize) = 0;
	if ( !this->isOpened() ) startRead();
	if ( !this->isOpened() ) return NULL;

	char *data;
	try {
		data = new char[m_lSize + 1];
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "ERROR: FileIO::readAll() unable to malloc data, %d (%s)", m_lSize + 1, e.what());
		return NULL;
	}

	if ( !read(data, m_lSize, true) ) {
		CLog::logf(CLog::LogType::IO, 2, "ERROR: FileIO::readAll() unable to read data from file, %s/%d", m_sFilename.c_str(), m_lSize);
		delete[] data;
		return NULL;
	}

	if ( pSize ) (*pSize) = m_lSize;
	return data;
}

bool FileIO::read(char *buf, size_t iSize, bool bEndChar)
{
	if ( !this->isOpened() ) startRead();
	if ( !this->isOpened() ) return false;

	if ( iSize > m_lSize ) iSize = m_lSize;
	try {
		m_fId.read(buf, iSize);
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 3, "ERROR: FileIO::read() unable to read from file: %s (%s)", m_sFilename.c_str(), e.what());
		return false;
	}

	if ( bEndChar ) buf[iSize] = '\0';

	return !m_fId.bad();
}

bool FileIO::WritePartFile ( size_t *offsets, size_t numOffset )
{
	if (noFile())
		return false;

	std::fstream File(m_sFilename, _in());
	if ( !File.is_open() ) return false;

	// first find file size
	File.seekg(0, std::ios::end);
	size_t fullsize = static_cast<size_t>(File.tellg()), size = fullsize;
	File.seekg(0, std::ios::beg);

	//TODO: add temproary directory
	std::fstream writeFile("temp.tmp", _out());
	if ( !File.is_open() ) {
		File.close();
		return false;
	}

	size_t off = 0;
	size_t startPos = 0;
	size_t remainingSize = fullsize;

	while ( off < numOffset ) {
		startPos = static_cast<size_t>(File.tellg());
		size_t offset = offsets[off++];
		size_t size = offset - startPos;
		try {
			char *data = new char[size];
			File.read(data, size);
			writeFile.write(data, size);
			delete data;
		}
		catch(std::exception &e) {
			CLog::logf(CLog::LogType::IO, 2, "FileIO::WritePartFilea() unable to read data from file: %s (%s)", m_sFilename.c_str(), e.what());
			return false;
		}

		size_t datasize = offsets[off++];
		File.seekg(datasize, std::ios::beg);
		remainingSize = fullsize - offset - datasize;
	}

	if ( remainingSize ) {
		char data[1000000];
		size_t amountLeft = 1000000;
		while ( remainingSize )
		{
			if ( amountLeft > remainingSize ) amountLeft = remainingSize;
			try {
				File.read(data, amountLeft);
				writeFile.write(data, amountLeft);
			}
			catch(std::exception &e) {
				CLog::logf(CLog::LogType::IO, 2, "FileIO::WritePartFilea() unable to read data from file: %s (%s)", m_sFilename.c_str(), e.what());
				return false;
			}

			remainingSize -= amountLeft;
			amountLeft = 1000000;
		}
	}

	File.close();
	writeFile.close();

	// now copy to original file
	std::remove(m_sFilename);
	//TODO: add temproary directory
	std::rename("temp.tmp", m_sFilename);

	return true;
}


void FileIO::setDir(const XLib::String &dir)
{
	if ( m_sFile.empty() )	m_sDirIO.setDir(dir);
	else					open(dir + "/" + m_sFile, m_bBinary);
}

void FileIO::_readFileSize ()
{
	m_lSize = 0;

	std::fstream file(m_sFilename, (m_bBinary) ? (std::ios::in | std::ios::binary) : (std::ios::in));
	if ( !file.is_open() ) return;

	file.seekg(0, std::ios::end);
	m_lSize = static_cast<size_t>(file.tellg());
	file.close();
}

bool FileIO::WipeFile()
{
	if (noFile())
		return false;
	std::ofstream file;
	file.open(m_sFilename, std::ios::out | std::ios::trunc);
	bool bDone = file.is_open();
	file.close();

	return bDone;
}


int FileIO::TruncateFile ( size_t offset, size_t datasize )
{
	if (noFile())
		return FILEERR_NOFILE;

	if ( !this->startRead() ) return FILEERR_NOOPEN;
	if ( (offset + datasize) > m_lSize ) return FILEERR_TOSMALL;

	//TODO: add temporary directory
	FileIO File("temp.tmp", m_bBinary);
	if ( !File.startWrite() ) return FILEERR_NOWRITE;
	File.setAutoDelete(true);

	if ( !File.write(*this, offset) ) return false;

	// next fseek after and write
	this->seek(datasize);
	size_t size = m_lSize - offset - datasize;
	if ( size > 0 ) {
		File.write(*this, size);
	}

	File.close();
	this->close();
	File.setAutoDelete(false);

	// now copy to original file
	if ( std::remove(_convertFilename(m_sFilename).c_str()) == 0 ) {
		std::rename("temp.tmp", _convertFilename(m_sFilename).c_str());
		size_t oldSize = m_lSize;
		size_t checkFileSize = m_lSize - datasize;
		this->_readFileSize();
		if ( checkFileSize != m_lSize ) {
			CLog::log(CLog::LogType::IO, 3, XLib::String("WARNING: FileIO::TruncateFile, file size mismatch, ") + (long)checkFileSize + " => " + (long)m_lSize);
		}
		return FILEERR_NONE;
	}

	return FILEERR_NOWRITE;
}

int FileIO::_in() const
{
	if ( m_bBinary ) return std::ios::in | std::ios::binary;
	return std::ios::in;
}

int FileIO::_out() const
{
	if ( m_bBinary ) return std::ios::out | std::ios::binary;
	return std::ios::out;
}
int FileIO::_inout() const
{
	if ( m_bBinary ) return std::ios::in | std::ios::out | std::ios::binary;
	return std::ios::in | std::ios::out;
}

int FileIO::_append() const
{
	if ( m_bBinary ) return std::ios::out | std::ios::binary | std::ios::app;
	return std::ios::out | std::ios::app;
}

char *FileIO::readToData ( size_t *size )
{
	*size = 0;

	if (noFile()) return NULL;
	if (!m_lSize) this->_readFileSize();
	if (!m_lSize) return NULL;

	std::fstream file(m_sFilename, _in());
	if ( !file.is_open() ) return NULL;

	char *data;
	try {
		data = new char[m_lSize + 1];
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "FileIO::ReadToData() unable to malloc storage, %d (%s)", m_lSize + 1, e.what());
		file.close();
		return NULL;
	}

	try {
		file.read((char *)data, m_lSize);
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "FileIO::ReadToData() unable to read data from file (%s)", e.what());
		file.close();
		return NULL;
	}

	*size = m_lSize;
	file.close();

	return data;
}

bool FileIO::WriteData ( const char *data, size_t size )
{
	if (noFile())
		return false;

	std::fstream File(m_sFilename, _out());
	if ( !File.is_open() ) return false;

	bool ret = true;
	try {
		File.write(data, size);
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "FileIO::ReadToData() unable to read data from file: %s (%s)", m_sFilename.c_str(), e.what());
		ret = false;
	}

	File.close();
	if (ret) ret = File.good();
	if ( ret ) this->_readFileSize();
	return ret;
}

bool FileIO::writeString(const XLib::String& data)
{
	std::string str = data.toString();
	return WriteData(str.c_str(), str.length());
}
bool FileIO::writeLine(const XLib::String& data)
{
	if (!this->isOpened())
	{
		if (!startWrite())
			return false;
	}

	m_fId << data.toString() << std::endl;
	return m_fId.good();
}

bool FileIO::Exists(const XLib::String &filename)
{
	std::fstream File(filename, std::ios::in | std::ios::binary);
	bool bRet = File.good();
	File.close();
	return bRet;
}

bool FileIO::ExistsOld () const
{
	return this->exists();
}
bool FileIO::exists () const
{
	if ( this->isOpened() ) return true;
	std::fstream File(m_sFilename, _in());
	bool bRet = File.good();
	File.close();
	return bRet;
}

bool FileIO::startRead()
{
	return _start(_in(), false);
}

bool FileIO::startWrite()
{
	return _start(_out(), true);
}

bool FileIO::startModify()
{
	return _start(_inout(), true);
}

bool FileIO::startAppend()
{
	return _start(_append(), false);
}

bool FileIO::_start(int iFlags, bool bSeekP)
{
	if ( !m_sFilename.empty() ) {
		if ( this->isOpened() ) this->close();

		m_fId.open(m_sFilename, iFlags);
	}
	m_bSeekP = bSeekP;
	return m_fId.is_open();
}

StringList FileIO::readLines() const
{
	std::vector<XLib::String> file;

	if (m_sFilename.empty()) return file;

	XLib::String line;
	std::wifstream infile (m_sFilename.c_str(), std::ios_base::in);
	while (std::getline(infile, line)) {
		file.push_back(line.removeChar((char)0));
	}

	infile.close();

	return file;
}

void FileIO::readLines(StringList &file) const
{
	if (m_sFilename.empty()) return;

	XLib::String line;
	std::wifstream infile (m_sFilename.c_str(), std::ios_base::in);
	while (std::getline(infile, line))	{
		file.push_back(line.removeChar((char)0));
	}

	infile.close();
}

bool FileIO::put(const char c)
{
	if ( !this->isOpened() ) return false;

	m_fId.put(c);
	return !m_fId.bad();
}

bool FileIO::writeSize(size_t iSize)
{
	if ( !this->isOpened() ) return false;
	if ( !this->put(static_cast<unsigned char>(iSize >> 24)) ) return false;
	if ( !this->put(static_cast<unsigned char>(iSize >> 16)) ) return false;
	if ( !this->put(static_cast<unsigned char>(iSize >> 8)) ) return false;
	if ( !this->put(static_cast<unsigned char>(iSize)) ) return false;
	m_lSize += 4;
	return true;
}

bool FileIO::write(FileIO &file, size_t iSize)
{
	if ( !this->isOpened() ) startWrite();
	if ( !this->isOpened() ) return false;

	size_t iMaxSize = 100000;

	char *data;
	try 
	{
		data = new char[iMaxSize + 1];
	}
	catch (std::exception &e)
	{
		CLog::logf(CLog::LogType::IO, 2, "ERROR: FileIO::write(FileIO, size_t) unable to malloc data, %d (%s)", iMaxSize + 1, e.what());
		return false;
	}

	size_t iSizeLeft = iSize;
	bool bSuccess = true;
	while ( iSizeLeft > 0 )
	{
		size_t iDoSize = iMaxSize;
		if ( iDoSize > iSizeLeft ) iDoSize = iSizeLeft;

		if ( !file.read(data, iDoSize) ) bSuccess = false;
		if ( bSuccess && !this->write(data, iDoSize) ) bSuccess = false;
		if ( !bSuccess ) break;
		iSizeLeft -= iDoSize;
	}

	delete[] data;

	return bSuccess;
}

bool FileIO::write(const char *buf, size_t iSize)
{
	if ( !this->isOpened() ) startWrite();
	if ( !this->isOpened() ) return false;
	try {
		m_fId.write(buf, iSize);
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "FileIO::write() unable to write to file: %s (%s)", m_sFilename.c_str(), e.what());
		return false;
	}
	m_lSize += iSize;
	return m_fId.good();
}
bool FileIO::write(const char *buf, ...)
{
	va_list args;
	va_start(args, buf);
	bool ret = this->_write(buf, args);
	va_end(args);
	return ret;
}
bool FileIO::_write(const char *buf, va_list args) 
{
	if ( !this->isOpened() ) return false;

	char buffer[8000];

#ifdef _WIN32
	vsprintf_s(buffer, buf, args);
#else
	vsprintf(buffer, buf, args);
#endif
	try {
		size_t iLen = std::strlen(buffer);
		m_fId.write(buffer, iLen);
		m_lSize += iLen;
	}
	catch (std::exception &e) {
		CLog::logf(CLog::LogType::IO, 2, "FileIO::write() unable to write to file: %s (%s)", m_sFilename.c_str(), e.what());
		return false;
	}
	return !m_fId.bad();
}

void FileIO::_seek(size_t iPos, size_t iFrom)
{
	if ( !this->isOpened() ) {
		if ( !this->startRead() ) return;
	}
	if ( m_bSeekP ) m_fId.seekp(iPos, static_cast<std::ios_base::seekdir>(iFrom));
	else m_fId.seekg(iPos, static_cast<std::ios_base::seekdir>(iFrom));
}

void FileIO::seek(size_t iPos)
{
	_seek(iPos, std::ios::cur);
}

void FileIO::seekEnd(size_t iPos)
{
	_seek(iPos, std::ios::end);
}

void FileIO::seekStart(size_t iPos)
{
	_seek(iPos, std::ios::beg);
}

std::fstream &FileIO::stream()
{
	return m_fId;
}

bool FileIO::isOpened() const
{
	if ( m_fId.is_open() )
		return true;
	return false;
}

void FileIO::close()
{
	if ( this->isOpened() ) m_fId.close();
	this->_readFileSize();
}

int FileIO::readSize()
{
	char size[4];
	if ( this->read(size, 4) ) return (size[0] << 24) + (size[1] << 16) + (size[2] << 8) + size[3];
	return 0;
}

bool FileIO::atEnd() const
{
	if ( !this->isOpened() ) return true;
	if ( m_fId.eof() )		 return true;
	return false;
}

bool FileIO::appendFile ( const XLib::String &filename )
{
	std::fstream fromFile(filename, _in());
	if ( !fromFile.is_open() ) return false;

	std::fstream toFile(m_sFilename, _append());
	if ( !toFile.is_open() ) {
		fromFile.close();
		return false;
	}

	// move to the end of the file
	toFile.seekg(0, std::ios::end);

	// get size of file
	fromFile.seekg(0, std::ios::end);
	size_t size = static_cast<size_t>(fromFile.tellg());
	fromFile.seekg(0, std::ios::beg);

	char *data = new char[500000];
	while ( size > 0 )
	{
		size_t read = 500000;
		if ( read > size )
			read = size;

		size -= read;

		fromFile.read(data, read);
		toFile.write(data, read);
	}

	delete[]data;
	fromFile.close();
	toFile.close();
	return true;
}

bool FileIO::appendData(const char *d, size_t size )
{
	std::ofstream File(m_sFilename, _append());
	if ( !File.is_open() ) return false;

	// move to the end of the file
	//File.seekg(0, std::ios::end);

	char *pos = (char *)d;
	while ( size > 0 ) {
		size_t read = 500000;
		if ( read > size ) read = size;

		size -= read;

		try {
			File.write(pos, read);
		} 
		catch(std::exception &e) {
			CLog::logf(CLog::LogType::IO, 2, "FileIO::AppendData() unable to write data to file: %s (%s)", m_sFilename.c_str(), e.what());
			File.close();
			return false;
		}
		pos += read;
	}

	File.close();
	return true;
}

bool FileIO::appendDataToPos(const char *d, size_t size, size_t start )
{
	if ( start > m_lSize ) return false;
	if ( m_lSize <= 0 ) {
		if ( !this->startWrite() ) return false;
	}
	else if ( start == m_lSize ) {
		if ( !this->startAppend() ) return false;
	}
	else {
		if ( !this->startModify() ) return false;
		this->seekStart(start);
	}

	return this->write(d, size);
}

XLib::String FileIO::readEndOfLine()
{
	if ( !this->isOpened() ) {
		if ( !startRead() )	return "";
	}

	std::string str;
	std::getline(m_fId, str);
	return XLib::String(str);
}

XLib::String FileIO::baseName() const
{
	return m_sFile.tokens(".", 1, -2);
}

XLib::String FileIO::extension ()
{
	if ( m_sFilename.empty() ) return XLib::String::Null();
	return m_sFilename.token(".", -1);
}

XLib::String FileIO::changeFileExtension(const XLib::String &ext) const
{
	if ( m_sFilename.empty() )
		return XLib::String::Null();
	
	return m_sFilename.tokens(".", 1, -2) + "." + ext;
}

bool FileIO::Remove(const XLib::String &rem)
{
	//if ( !Exists() ) return false;
	return (std::remove(rem) == 0) ? true : false;
}

bool FileIO::remove()
{
	if ( this->isOpened() ) this->close();
	if ( !this->exists() ) return false;
	if ( std::remove(_convertFilename(m_sFilename).c_str()) == 0 ) return true;
	return false;
}

bool FileIO::writeFileUTF(const StringList &lines) const
{
	if (m_sFilename.empty() || lines.empty())
		return false;

	// we need to create the directory
	if (!m_sDirIO.exists())
	{
		if (!m_sDirIO.create())
			return false;
	}

#ifdef _WIN32
	TCHAR buf[5000];
	wsprintf(buf, L"%hs", m_sFilename.c_str());
	FILE* id;
	_wfopen_s(&id, buf, L"wt+,ccs=UTF-8");
	if (!id)
		return false;

	// write the rest
	for(auto itr = lines.begin(); itr != lines.end(); itr++)
	{
		XLib::String l = (*itr);
		if (l.isin('\n'))
		{
			StringList strs = l.tokenise("\n");
			for (int i = 0; i < strs.size(); i++)
			{
				XLib::String line = strs[i];
				line += "\n";
				int size = wsprintf(buf, L"%hs", line.c_str());
				fwrite(buf, sizeof(TCHAR), wcslen(buf), id);
			}
		}
		else
		{
			l += "\n";
			int size = wsprintf(buf, L"%hs", l.c_str());
			fwrite(buf, sizeof(TCHAR), wcslen(buf), id);
		}
	}

	fclose(id);

	return true;
#else
	//TODO: write utf8 file writing function
	return false;
#endif
}



bool FileIO::writeFile(const StringList& lines) const
{
	if (lines.empty() || m_sFilename.empty())
		return false;

	// we need to create the directory
	if (!m_sDirIO.exists())
	{
		if (!m_sDirIO.create())
			return false;
	}

	std::ofstream out(m_sFilename.c_str());
	if (!out)
		return false;

	for (auto itr = lines.begin(); itr != lines.end(); itr++)
		out << (*itr).c_str() << std::endl;

	out.close();

	return true;
}

bool FileIO::rename(const XLib::String &toFile)
{
	if (std::rename(_convertFilename(m_sFilename).c_str(), _convertFilename(toFile).c_str()) == 0)
		return open(toFile, m_bBinary);
	return false;
}

XLib::String FileIO::windowsFilename()
{
	XLib::String returnString = m_sFilename;
	returnString = returnString.findReplace("/", "\\");
	return returnString;
}

void FileIO::setCreationTime(time_t time)
{
#ifdef _WIN32
	// Note that LONGLONG is a 64-bit value
    LONGLONG ll;

	FILETIME ft;
    ll = Int32x32To64(time, 10000000) + 116444736000000000;
    ft.dwLowDateTime = (DWORD)ll;
    ft.dwHighDateTime = ll >> 32;

	HANDLE filename = CreateFile(windowsFilename().c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	// Set the file time on the file
	SetFileTime(filename,(LPFILETIME) NULL,(LPFILETIME) NULL,&ft);
	// Close our handle.
	CloseHandle(filename);
#endif
}

time_t FileIO::creationTime()
{
	return modifiedTime();
}

time_t FileIO::modifiedTime()
{
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA wfad;
	GetFileAttributesEx(windowsFilename().c_str(), GetFileExInfoStandard, &wfad);

	LARGE_INTEGER date, adjust;
	date.HighPart = wfad.ftLastWriteTime.dwHighDateTime;
	date.LowPart = wfad.ftLastWriteTime.dwLowDateTime;

	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;

	// removes the diff between 1970 and 1601
	date.QuadPart -= adjust.QuadPart;

	// converts back from 100-nanoseconds to seconds
	return (time_t)(date.QuadPart / 10000000);
#else
	struct stat fileStat;
	if ( !stat(GetWindowsFilename().c_str(), &fileStat) )
		return (time_t)fileStat.st_atime;
#endif
	return 0;
}

size_t FileIO::position()
{
	return static_cast<size_t>(m_fId.tellg());
}

/**
 * Copys the contents of a file to another
 *
 * Reads and writes the files in block
 */
bool FileIO::copy(const XLib::String &toFile, bool keepTime)
{
	time_t time = creationTime();

	FileIO File(toFile, m_bBinary);
	if ( File.write(*this, m_lSize) ) {
		this->close();
		File.close();
		if ( keepTime )	File.setCreationTime(time);
		return true;
	}	
/*
	std::fstream f(GetWindowsFilename().c_str(), std::fstream::in | std::fstream::binary);
	f << std::noskipws;
	std::istream_iterator<unsigned char> begin(f);
	std::istream_iterator<unsigned char> end;
 
	std::fstream f2(toFile.c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);
	std::ostream_iterator<char> begin2(f2);
 
	std::copy(begin, end, begin2);
	return f2.good();
*/
	return false;
}

size_t FileIO::fileSize() const
{
	return m_lSize;
}

std::string FileIO::_convertFilename(const XLib::String& str) const
{
	std::string s;
	std::transform(str.begin(), str.end(), std::back_inserter(s), [](wchar_t c) {
		return (char)c;
	});

	return s;
}
