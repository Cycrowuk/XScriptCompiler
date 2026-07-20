// File.h: interface for the C_File class.
//
//////////////////////////////////////////////////////////////////////

/*
File class, Created by Matthew Gravestock (Cycrow)

	This class handles the store of data for each of the file in the package.
	It also includes all the compression functions to compress and uncompress the files from the package

	Enums:
		Compression Type - The type of compression for each section and file, theres currently 3 types
			7Zip - Compress via 7Zip compression, only works on windows version, but linux version can decompress them. Default for file data on Windows version
			ZLIB - Compress via the ZLIB libray, ie Zip copression. Default compression for headers on all versions, and file data on Linux Version
			None - No compression, just write in plain text (Not really used, prodcudes much larger files)

		FileType - This is what type of file they are, deterimes how and where the file is installed
			Script - A script file, goes into the X3/Scripts directory
			Text - A Text file, ie 447532.xml, contains all text data used by script/mod.  Goes in the X3/t directory
			Readme - A Readme .txt file, can be displayed in installer in Rich Text format
			Map - A Map script, these are xml files that create a new universe
			Mod - .cat/.dat file pair, can also be a fake patch if named as a number, ie 01.cat
			Uninstall - An uninstall script, only goes to X3/Scripts directory when plugin is uninstalled, allows plugins to clean themselves when removed
			Sound - Soundtrack files, goes in the X3/Soundtrack directory, used for the sector music
			Screen - Screen shot file, goes in the X3/loadscr directory, will be displayed as a loading screen
			Extra - Any other file that doesn't fit in the section, can be placed in any directory thats required

		Error - Stores the last error found (currently only used for a few errors)
			None - No error, function was successful
			Malloc - Error trying to malloc memory space, possible cause (Not enough memory)
			Fileopen - Unable to open the file, possible cause (File doesn't exist)
			Fileread - Error trying to read a file, happens after the file is open

	Class includes all files needed for both 7Zip and ZLIB Libraries, and acts as the path between them

	Functions (Windows Only):
		LZMAEncodeData - Encodes a data stream using 7Zip, returns the encoded data as char array
		LZMADecodeData - Decodes a compressed stream using 7Zip, returns the uncompressed data
		LZMAEncodeFile - Encodes a file stream using 7Zip, writes to another file
		LZMADecodeFile - Decodes a file stream using 7Zip, writes uncomressed data to file

	Classes
		CProgressInfo - Used to store progress info, basse class that needs to be dervived from to handle the progress update.
						When decoding multi files, calls DoingFile() for each functions, allows class to display the current progress of the file
						ProgressUpdated() is called for each progress, as the processed size and max size
		CProgressInfo7Zip (Windows Only) - 7Zip specific progress, dervive from this for using progress on 7Zip Compression. (Currently no progress for ZLIB)

	C_File:
		The files here are stored in 1 of 2 ways, either file data, or a file pointer

		File Data:
			Uses the m_sData and m_lDataSize varibles.  As well as the m_iDataCompression.
			This method is used when reading the file to memory, either from a file or direct from the SPK Package.
			This can be stored in compressed form so when writing the SPK File, it doesn't need to be compressed again.
			CompressData() will compress the current data stream to m_sData, changes m_lDataSize and m_iDataCompression to match
			UncomressData() will uncompress the data, it can either uncomress and return the uncomressed data stream, without effecting m_sData,
			or it can change m_sData to the uncompressed data.

		File Pointer:
			When adding a new file, it will be first added as a pointer, this means that there is no data loaded, it just points to a filename on disk.
			ReadFileSize() will read the size of the file to m_lSize
			ReadLastModifed() will read the modified data tag of the file and store it in the class
			ReadFromFile() will open and read the file to the data stream, m_sData, the compression will be set to None.
			ReadFromFile (FILE *id) will read from a currently open file stream into the data, can be used to read a file from an existing SPK Package


*/

#if !defined(AFX_FILE_H__A0C15B81_4FD1_40D7_8EE8_2ECF5824BB8B__INCLUDED_)
#define AFX_FILE_H__A0C15B81_4FD1_40D7_8EE8_2ECF5824BB8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define LZMA_LEVEL	5
#define LZMA_DICT	(1 << 26)

#define PCKHEADERSIZE 10
#define DEFAULT_COMPRESSION_LEVEL	5

#include "spkdefines.h"
#include "File_IO.h"
#include "zlib/zlib.h"
#include "enums.h"
//#include "x2bc/x2bc_common/bob_dom.h"
//#include "x2bc/x2bc_common/bob_realfile_stream.h"

// compression type
enum { SPKCOMPRESS_NONE, SPKCOMPRESS_ZLIB, SPKCOMPRESS_7ZIP, SPKCOMPRESS_LZMA, SPKCOMPRESS_BEST };

// special file types used internally
enum {
	FILETYPE_SCRIPT_UNINSTALL		= 1000,
};
// error
enum {SPKERR_NONE, SPKERR_MALLOC, SPKERR_FILEOPEN, SPKERR_FILEREAD, SPKERR_UNCOMPRESS, SPKERR_WRITEFILE, SPKERR_CREATEDIRECTORY, SPKERR_FILEMISMATCH};
enum {STATUS_NONE, STATUS_COMPRESS, STATUS_WRITE};


bool IsDataPCK ( const unsigned char *data, size_t size );
unsigned char SPKEXPORT *UnPCKData ( unsigned char *data, size_t datasize, size_t *len, bool nocrypt );
unsigned char SPKEXPORT *UnPCKFile (const Utils::WString &file, size_t *len, bool nocrypt );
unsigned char SPKEXPORT *UnPCKData ( unsigned char *data, size_t datasize, size_t *len );
int ReadScriptVersionFromData ( unsigned char *data, long size );
bool ReadSignedFromData ( unsigned char *data, long size );

struct SMultiSpkFile;

class C_File;
class SPKEXPORT CProgressInfo
{
public:
	CProgressInfo () { m_bDoIn = false; m_lMaxSize = 0; m_bDoHalf = false; m_bSecondHalf = false; m_bDoSecond = false; m_iStatus = -1; m_iDone = 0;}

	void SetIn ( bool in ) { m_bDoIn = in; }
	void SetMax ( long max ) { m_lMaxSize = max; }
	void DoHalf() { m_bDoHalf = true; m_bSecondHalf = false; }
	void SecondHalf() { m_bSecondHalf = true; }
	void SwitchSecond() { m_bDoSecond = !m_bDoSecond; }
	bool IsSecond() { return m_bDoSecond; }
	void UpdateStatus(int i ) { m_iStatus = i; StatusUpdated(i); }
	int  GetStatus() { return m_iStatus; }
	void IncDone(int i) { m_iDone += i; }
	void UpdateProgress ( const long cur, const long max )
	{
		if ( !m_bDoSecond )
			ProgressUpdated(cur, max);
		else
			ProgressUpdated2(cur, max);
	}
	virtual void UpdateDisplay(const Utils::WString& display) {};
	void UpdateFile(C_File *f) { DoingFile(f); }
	void UpdatePackage(SMultiSpkFile *f) { DoingPackage(f); }
	unsigned long *GetDonePointer() { return &m_iDone; }
	unsigned long GetDone() { return m_iDone; }
	void SetDone(int i) { m_iDone = i; }

protected:
	virtual void ProgressUpdated ( const long cur, const long max ) = 0;
	virtual void ProgressUpdated2 ( const long cur, const long max ) { };
	virtual void StatusUpdated(int i) { }
	virtual void DoingFile ( C_File *file ) = 0;
	virtual void DoingPackage ( SMultiSpkFile *file ) { }

	bool m_bDoIn;
	long m_lMaxSize;
	bool m_bDoHalf;
	bool m_bSecondHalf;
	bool m_bDoSecond;
	int	 m_iStatus;

	unsigned long m_iDone;
};


class SPKEXPORT CProgressInfoDone : public CProgressInfo
{
public:
	CProgressInfoDone () : CProgressInfo() { m_pOnFile = NULL; m_bDontDoMax = false; m_pOnPackage = NULL; }

	unsigned long GetMax() { return m_lMaxSize; }
	C_File *GetFile() { return m_pOnFile; }
	SMultiSpkFile *GetPackage() { return m_pOnPackage; }

	void DoMax() { m_bDontDoMax = false; }
	void DontDoMax() { m_bDontDoMax = true; }

protected:
	virtual void ProgressUpdated ( const long cur, const long max ) { m_iDone = cur; m_lMaxSize = max; }
	virtual void DoingFile ( C_File *file );
	virtual void DoingPackage ( SMultiSpkFile *package );
	virtual void StatusUpdated(int i) { if ( i == STATUS_WRITE ) this->DontDoMax(); }

	SMultiSpkFile	*m_pOnPackage;
	C_File			*m_pOnFile;
	bool			 m_bDontDoMax;
};

#include "ansi7zip/7Decoder.h"

#define GZ_FLAG_TEXT     1     // 0
#define GZ_FLAG_HCRC     2     // 1
#define GZ_FLAG_EXTRA    4     // 2
#define GZ_FLAG_FILENAME 8     // 3
#define GZ_FLAG_COMMENT  16    // 4
#define GZ_FLAG_RES1     32    // 5
#define GZ_FLAG_RES2     64    // 6
#define GZ_FLAG_RES3     128   // 7

class CProgressInfo2
{
public:
	virtual void ProgressPercent ( float percent ) = 0;
};


Utils::WString SPKEXPORT GetFileTypeString ( int type );
FileType SPKEXPORT GetFileTypeFromString(const Utils::WString &type);
Utils::WString SPKEXPORT FormatErrorString (int error, const Utils::WString &rest);
float	 SPKEXPORT GetLibraryVersion ();
float	 SPKEXPORT GetFileFormatVersion ();

class CBaseFile;
class SPKEXPORT C_File
{
public:
	static Utils::WString GetDirectory(FileType eType, const Utils::WString &filename, CBaseFile *file);
	static bool DoesTypeHaveExtraDir(int i);

public:

	void clearUsed () { _iUsed = 0; }
	void incUsed () { ++_iUsed; }
	void DeleteData ();
	// varible setting functions
	void setFileType(FileType t) { m_iFileType = t; }
	void setFilename(const Utils::WString &filename);
	void setName(const Utils::WString &name) { _sName = name; }
	void setDir(const Utils::WString &dir) { _sDir = dir; }
	void SetCreationTime ( time_t time ) { m_tTime = time; }
	void SetFileSize ( long size ) { m_lSize = size; }
	void SetDataSize ( long size ) { m_lDataSize = size; }
	void SetUncompressedDataSize ( long size ) { m_lUncomprDataSize = size; }
	void SetDataCompression ( int c ) { m_iDataCompression = c; }
	void SetShared ( bool b ) { m_bShared = b; }
	void SetSigned ( bool b ) { m_bSigned = b; }
	void setFullDir(const Utils::WString &dir) { _sFullDir = dir; }
	void setInMod (const Utils::WString &s) { _sInMod = s; }
	void SetCompressedToFile ( bool b ) { m_bCompressedToFile = b; }
	void SetData(const unsigned char *data, size_t size) { m_sData = (unsigned char *)data, m_lDataSize = (long)size; }
	void copyData(const unsigned char *data, size_t size);

	// get functions
	int getUsed () { return _iUsed; }
	int GetLastError () { return m_iLastError; }
	int GetFileType() { return m_iFileType; }
	FileType fileType() { return m_iFileType; }
	const Utils::WString &filename() const { return _sName; }
	Utils::WString fullFilename() { return _sFullDir + "/" + _sName; }
	long GetSize () { return m_lSize; }
	time_t GetCreationTime () { return m_tTime; }
	const Utils::WString &name() { return _sName; }
	const Utils::WString &dir() { return _sDir; }
	Utils::WString dataSizeString() const;
	Utils::WString uncompressedSizeString() const;
	Utils::WString getDirectory(CBaseFile *spkfile) const;
	int GetVersion () { return m_iVersion; }
	time_t GetLastModified() { return m_tTime; }
	Utils::WString fileTypeString() const { return ::GetFileTypeString(m_iFileType); }
	Utils::WString creationTimeString() const;
	const Utils::WString &getTempFile () const { return _sTmpFile; }
	const Utils::WString fullDir() const { return _sFullDir; }
	Utils::WString baseName() const;
	const Utils::WString &originalName() const { return _sOriginalName; }
	const Utils::WString &getInMod() const { return _sInMod; }
	const Utils::WString &getSignature() { if ( !m_bUpdatedSignature ) updateSignature(); return _sSignature; }
	void updateSignature();
	void changeBaseName(const Utils::WString &b) { _sName = b + "." + this->fileExt(); }
	bool renameScript(const Utils::WString &baseName);
	bool isInMod() { return !_sInMod.empty(); }

	void FixOriginalName() { _sOriginalName = _sName; }
	void SetOriginalName(const Utils::WString &name) { _sOriginalName = name; }

	bool isFileInAddon() const;
	bool IsFakePatch () const;
	bool isAutoTextFile();
	int  textFileID(const Utils::WString &name = Utils::WString::Null()) const;
	bool IsCompressedToFile () { return m_bCompressedToFile; }
	bool CompareNew ( C_File *file );
	Utils::WString fileExt() const;
	bool checkFileExt(const Utils::WString& ext);
	const Utils::WString &changeFileExt(const Utils::WString &ext);
	bool CheckPackedExtension();
	bool shouldCheckBaseName() const;

	int GetCompressionType () { return m_iDataCompression; }
	long GetDataSize () { return m_lDataSize; }
	// returns the Uncompressed Data size, based on how the file is loaded, ie view data, view pointer, etc
	long uncompressedDataSize() const;
	Utils::WString getNameDirectory(CBaseFile *spkfile) const;

	unsigned int game() const;
	bool isForGame(int game) const;
	int getForSingleGame() const;
	void setGame(unsigned int i);
	//TODO
	unsigned int GetGame() const { return _iGame;  }

	unsigned char *GetData () { return m_sData; }
	bool IsShared () { return m_bShared; }
	bool IsSigned () { return m_bSigned; }
	bool UpdateSigned();
	bool ReadSignedFile();

	// file reading functions
	long ReadFileSize ();
	time_t ReadLastModified ();
	bool readFromFile(const Utils::WString filename);
	bool ReadFromFile ();
	bool readFromFile(CFileIO &File);
	bool readFromFile(CFileIO &File, int lSize, bool bDoSize = true);
	bool ReadFromData ( char *data, long size );
	int ReadScriptVersion ();
	Utils::WString filePointer() const;
	bool isExternalFile() const;

	// file writing functions
	bool writeToDir(const Utils::WString &dir, CBaseFile *, bool = true, const Utils::WString &appendDir = Utils::WString::Null(), unsigned char * = NULL, long = 0);
	bool writeToFile(const Utils::WString &filename, unsigned char * = NULL, long = 0);
	bool writeFilePointer(unsigned char *cData = NULL, long len = 0);

	// file compression functions
	unsigned char *CompressToData(int compressionType, size_t *outSize, CProgressInfo *progress = NULL, int level = DEFAULT_COMPRESSION_LEVEL);
	bool uncompressToFile(const Utils::WString &toFile, CBaseFile *spkFile, bool includeDir = true, CProgressInfo *progress = NULL);
	bool ChangeCompression(int compresstyp, CProgressInfo *progress);
	bool CompressData ( int compressionType, CProgressInfo * = NULL, int level = DEFAULT_COMPRESSION_LEVEL );
	bool CompressFile ( CProgressInfo * = NULL );
	bool UncompressData ( CProgressInfo * = NULL );
	unsigned char *UncompressData ( long *size, CProgressInfo * = NULL );

	void CopyData(C_File *, bool includeData = true);

	bool IsDisabled () { return m_bDisabled; }
	void SetDisabled ( bool b ) { m_bDisabled = b; }

	void MarkSkip () { m_bSkip = true; }
	bool Skip () { return m_bSkip; }
	bool CheckPCK ();
	bool CheckValidFilePointer ();
	unsigned char *UnPCKFile ( size_t *len );
	bool MatchFile ( C_File *file );
	bool UnPCKFile ();
	bool PCKFile();

	void SetPos(int i)	{ m_iPos = i; }
	int  GetPos()		{ return m_iPos; }

	unsigned char* BobDecompile(size_t* size);
	unsigned char* BobCompile(size_t* size, bool isCut = false);

	// contructor/decontructor
	C_File();
	C_File(const Utils::WString &filename);
	virtual ~C_File();

protected:
	static int m_iTempNum; // Used when creating temp files, allows new tmp files without overrighing used ones

	// private functions
	void Reset();
	Utils::WString _getFullFileToDir(const Utils::WString &dir, bool includedir, CBaseFile *file) const;

	Utils::WString  _sName; // just the filename
	Utils::WString  _sDir;  // the extra dir (only for extras)
	Utils::WString  _sInMod; // in a mod files
	Utils::WString  _sSignature;

	// file pointer varibles
	Utils::WString  _sFullDir; // the full dir path of the file (This is used for file pointers when not loaded data in file)
	long    m_lSize;  // size of current file pointer

	// Main file varibles
	int		m_iVersion;  // used when reading script versions
	bool	m_bSigned;   // signed status of file, installer only, read from file
	bool    m_bShared;   // is file a marked shared file (Not used much anymore)
	int		_iUsed;     // used by installer, number of plugins that uses the file
	time_t	m_tTime;     // Creation time of the file

	// File data varibles
	long	m_lDataSize;  // size of the data stream in what ever compression is set
	long	m_lUncomprDataSize; // size of stream if it was uncompressed
	unsigned char   *m_sData;  // stores the file data if loaded in memory
	int		m_iDataCompression; // the current compression of m_sData

	FileType m_iFileType; // type if file, ie Script, Text, etc

	bool    m_bUsedMalloc; // malloc type of m_sData, so it can use with free() or delete
	Utils::WString	_sTmpFile;

	bool	m_bCompressedToFile;

	bool    m_bSkip;
	bool	m_bLoaded;
	bool	m_bDisabled;	// if the file has be disabled
	bool	m_bUpdatedSignature;

	int		m_iPos;
	unsigned int _iGame;

	Utils::WString _sOriginalName;

	int m_iLastError;
	bool	m_bDontDeleteData; // fix for bad data deleteion
};

#endif // !defined(AFX_FILE_H__A0C15B81_4FD1_40D7_8EE8_2ECF5824BB8B__INCLUDED_)
