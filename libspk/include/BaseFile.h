// BaseFile.h: interface for the CBaseFile class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BASEFILE_H__
#define __BASEFILE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "lists.h"
#include "File.h"
#include "Utils/String.h"
#include "archive/zip.h"

#include "Package/CorePackage.h"

#define GAME_ALL			0
#define GAME_X2				1
#define GAME_X3				2
#define GAME_X3TC			3
#define GAME_X3AP			4
#define GAME_X3FL			5
#define GAME_MAX			(GAME_X3FL)
#define GAME_ALLNEW			(1 << 31)

#define FILEVERSION 4.40f

#define WRITECHUNK		50000

class CPackages;
class CGameExe;

namespace SPK {
	class CTextDB;
}

using namespace SPK;

enum {
	READFLAG_NONE = 0,
	READFLAG_NOUNCOMPRESS = 1
};

enum {SPKFILE_INVALID, SPKFILE_SINGLE, SPKFILE_MULTI, SPKFILE_SINGLESHIP, SPKFILE_OLD, SPKFILE_BASE};
enum {SPKREAD_ALL, SPKREAD_NODATA, SPKREAD_VALUES, SPKREAD_HEADER};
enum {DELETESTATE_NONE, DELETESTATE_WAITING, DELETESTATE_DONE};
enum {SPKINSTALL_CREATEDIRECTORY, SPKINSTALL_CREATEDIRECTORY_FAIL, SPKINSTALL_WRITEFILE, SPKINSTALL_WRITEFILE_FAIL, SPKINSTALL_DELETEFILE, SPKINSTALL_DELETEFILE_FAIL,
		SPKINSTALL_SKIPFILE, SPKINSTALL_REMOVEDIR, SPKINSTALL_ENABLEFILE, SPKINSTALL_DISABLEFILE, SPKINSTALL_ENABLEFILE_FAIL, SPKINSTALL_DISABLEFILE_FAIL,
		SPKINSTALL_UNINSTALL_MOVE, SPKINSTALL_UNINSTALL_COPY, SPKINSTALL_UNINSTALL_MOVE_FAIL, SPKINSTALL_UNINSTALL_COPY_FAIL, SPKINSTALL_UNINSTALL_REMOVE, SPKINSTALL_UNINSTALL_REMOVE_FAIL,
		SPKINSTALL_ORIGINAL_BACKUP, SPKINSTALL_ORIGINAL_RESTORE, SPKINSTALL_ORIGINAL_BACKUP_FAIL, SPKINSTALL_ORIGINAL_RESTORE_FAIL, SPKINSTALL_FAKEPATCH, SPKINSTALL_FAKEPATCH_FAIL, 
		SPKINSTALL_MISSINGFILE, SPKINSTALL_SHARED, SPKINSTALL_SHARED_FAIL, SPKINSTALL_ORPHANED, SPKINSTALL_ORPHANED_FAIL, SPKINSTALL_UNCOMPRESS_FAIL, SPKINSTALL_AUTOTEXT, SPKINSTALL_AUTOTEXT_FAIL
};

enum {PACKAGETYPE_NORMAL, PACKAGETYPE_LIBRARY, PACKAGETYPE_CUSTOMSTART, PACKAGETYPE_PATCH, PACKAGETYPE_UPDATE, PACKAGETYPE_MOD, PACKAGETYPE_SHIP, PACKAGETYPE_FAKEPATCH};

enum BaseFileType {TYPE_BASE, TYPE_SPK, TYPE_XSP, TYPE_ARCHIVE};

// spk header struct
tstruct SSPKHeader {
	SSPKHeader () { fVersion = 0; iValueCompression = lValueCompressSize = 0; }
	float fVersion;
	int iValueCompression;
	unsigned long lValueCompressSize;
} SSPKHeader;


tstruct SSPKHeader2 {
	SSPKHeader2 () : iFileCompression(0), iDataCompression(0), iNumFiles(0), lSize(0), lFullSize(0) { }
	int iNumFiles;
	long lSize;
	long lFullSize;
	int iFileCompression;
	int iDataCompression;
} SSPKHeader2;

class SPKEXPORT CBaseFile : public SPK::Package::CCorePackage
{
public:
	// static functions
	static Utils::WString ConvertGameToString(int game);
	static int GetGameFromString(const Utils::WString &game);
	static Utils::WString ErrorString(int error, const Utils::WString &errorStr = Utils::WString::Null());
	static Utils::String GetEndOfLine(FILE* id, int* line = NULL, bool upper = true);
	static int CheckFile(const Utils::WString &filename, float* version = NULL);


	CBaseFile();
	virtual ~CBaseFile();

	// Virtual Functions
	virtual Utils::WString getFullPackageName(int language, const Utils::WString &byString) const;
	virtual Utils::WString getFullPackageName(int language, bool includeVersion = true, const Utils::WString &byString = L"by") const;
	virtual Utils::WString getFullPackageName(const Utils::WString &format, int lang) const;

	virtual Utils::WString createValuesLine() const;

	virtual bool loadPackageData(const Utils::WString& sFirst, const Utils::WString& sRest, const Utils::WString& sMainGame, Utils::WStringList& otherGames, Utils::WStringList& gameAddons, CProgressInfo* progress);
	virtual bool GeneratePackagerScript(bool wildcard, Utils::WStringList* list, int game, const Utils::WStringList& gameAddons, bool datafile = false);
	virtual bool GeneratePackagerScriptFile(bool wildcard, Utils::WStringList* list, int game, const Utils::WStringList& gameAddons);

	// Getters
	Utils::WString getNameValidFile() const;
	const CLinkList<C_File> &fileList() const;
	CLinkList<C_File>& fileList(FileType type, CLinkList<C_File> &list) const;
	CLinkList<C_File>& fileList(FileType type);
	CLinkList<C_File> *GetFileList() { return &m_lFiles; }
	C_File *icon() const { return m_pIconFile; }
	const Utils::WString &iconExt() const { return _sIconExt; }
	int dataCompression () const { return m_SHeader2.iDataCompression; }
	float fileVersion () const { return m_SHeader.fVersion; }
	size_t fileSize() const;
	Utils::WString getAutosaveName() const;
	bool IsMod();
	bool IsFakePatch() const;
	const Utils::WStringList& getGlobals() const { return _lGlobals; }

	// Setters
	void setAutosaveName() { this->setFilename(getAutosaveName()); }
	void SetDataCompression(int c) { m_SHeader2.iDataCompression = c; }
	void SetFileCompression(int c) { m_SHeader2.iFileCompression = c; }
	void SetValueCompression(int c) { m_SHeader.iValueCompression = c; }
	void setIcon(C_File* file, const Utils::WString& ext) { if (m_pIconFile) delete m_pIconFile; _sIconExt = ext.c_str(); m_pIconFile = file; _changed(); }
	void setFtpAddr(const Utils::WString& str) { _sFtpAddr = str; }
	
	void addGlobal(const Utils::WString& global, const Utils::WString& setting);
	void removeGlobal(const Utils::WString& global);

	// clearers
	void clearGlobals();
	 
	// Game Compatability
	SGameCompat* GetGameCompatability(int game);
	bool RemoveGameCompatability(int game);
	void AddGameCompatability(int game, const Utils::WString &version);
	bool CheckGameCompatability(int game);
	bool checkGameVersionCompatability(int game, const Utils::WString &sVersion, int iVersion) const;
	bool AnyGameCompatability() { return !m_lGames.empty(); }
	bool anyGameCompatability() const { return !m_lGames.empty(); }

	// Files
	C_File *GetFirstFile(int type) const;
	C_File *GetNextFile(C_File *prev) const;
	C_File *GetPrevFile(C_File *next) const;
	C_File* findMatchingMod(C_File* f) const;

	void convertNormalMod(C_File *f, const Utils::WString &to) const;
	void convertFakePatch(C_File *f) const;
	void convertAutoText(C_File *f) const;
	void renameFile(C_File *f, const Utils::WString &baseName) const;

	size_t countFiles(FileType filetype) const;
	C_File* findFileAt(FileType filetype, size_t pos) const;
	C_File* findFile(const Utils::WString& file, FileType type, const Utils::WString& dir = Utils::WString::Null(), int game = 0) const;

	void addFileScript(FileType filetype, bool shared, bool packed, const Utils::WString &rest, const Utils::WString &sMainGame, Utils::WStringList &otherGames, Utils::WStringList &gameAddons, CProgressInfo *progress = NULL);
	void AddFile(C_File* file);
	C_File* addFile(const Utils::WString& file, const Utils::WString& dir, FileType type, int game = 0, bool packed = false);
	C_File* appendFile(const Utils::WString& file, int type, int game, bool packed, const Utils::WString& dir = Utils::WString::Null(), CProgressInfo* progress = NULL);
	bool addFileNow(const Utils::WString&, const Utils::WString&, FileType type, CProgressInfo* progress = NULL);
	bool removeFile(size_t pos);
	bool removeFile(C_File* files);
	bool removeFile(const Utils::WString &file, FileType type, const Utils::WString &dir = Utils::WString::Null(), int game = 0);
	void removeAllFiles(FileType type, int game);

	Utils::WString createFilesLine(SSPKHeader2 *header, CProgressInfo* = NULL) const;

	// error handling
	void ClearError () { _sLastError = L""; _iLastError = SPKERR_NONE; }
	int lastError() const { return _iLastError; }
	const Utils::WString lastErrorString() const { return _sLastError; }

	virtual bool writeHeader(CFileIO &file, int iHeader, int iLength) const;
	virtual bool writeData(CFileIO &file, CProgressInfo * = NULL) const;
	virtual bool writeFile(const Utils::WString &filename, CProgressInfo* = NULL) const;
	virtual bool readFile(const Utils::WString &filename, int readType = SPKREAD_ALL, CProgressInfo *progress = NULL);
	bool readFile(CFileIO &File, int readtype, CProgressInfo *progress);

	virtual bool extractFile(int file, const Utils::WString &dir, bool includedir = true, CProgressInfo *progress = NULL);
	virtual bool extractFile(C_File *file, const Utils::WString &dir, bool includedir = true, CProgressInfo *progress = NULL);
	virtual bool extractFile(int filenum, const Utils::WString &dir, unsigned int game, const Utils::WStringList &gameAddons, bool includedir = true, CProgressInfo *progress = NULL);
	virtual bool extractFile(C_File *file, const Utils::WString &dir, unsigned int game, const Utils::WStringList &gameAddons, bool includedir = true, CProgressInfo *progress = NULL);
	virtual bool extractAll(const Utils::WString &dir, int game, const Utils::WStringList &gameAddons, bool includedir = true, CProgressInfo *progress = NULL);

	virtual bool saveToArchive(const Utils::WString &filename, int game, const CGameExe *exes, CProgressInfo *progress = NULL);
	virtual void addGeneratedFiles(HZIP &hz) {};

	void ClearFileData();

	// reading files
	void ReadAllFilesToMemory ();
	void ReadIconFileToMemory ();
	bool ReadFileToMemory(C_File *f);

	// compression
	void RecompressAllFiles ( int compresstype, CProgressInfo *progress );
	void CompressAllFiles ( int compresstype, CProgressInfo *progress = NULL, CProgressInfo *overallProgress = NULL, int level = DEFAULT_COMPRESSION_LEVEL );
	bool UncompressAllFiles ( CProgressInfo * = NULL );

	bool IsFileAdded(C_File *f) { return m_lFiles.FindData(f); }

	// installing
	void SwitchFilePointer(C_File *oldFile, C_File *newFile);
	bool installFiles(const Utils::WString &destdir, CProgressInfo *progress, CLinkList<C_File> *spklist, Utils::WStringList *errorStr, bool enabled = true, CPackages *packages = NULL );
	virtual bool IsPatch () { return false; }

	// installer functions
	bool IsProfileEnabled () { return m_bProfile; }
	bool isEnabled() const { return m_bEnable; }
	bool IsEnabled() { return m_bEnable; }
	bool IsModifiedEnabled () { return m_bModifiedEnabled; }
	bool IsGlobalEnabled () { return m_bGlobal; }

	void SetProfileEnabled ( bool en ) { m_bProfile = en; }
	void SetEnabled ( bool en ) { m_bEnable = en; }
	void SetModifiedEnabled ( bool en ) { m_bModifiedEnabled = en; }
	void SetGlobalEnabled ( bool en ) { m_bGlobal = en; }
	virtual void completeFile() { }

	int  GetLoadError() { return m_iLoadError; }
	void SetLoadError(int i) { m_iLoadError = i; }
	Utils::WString createUpdateFile(const Utils::WString &dir) const;

	virtual bool parseValueLine(const Utils::WString &line);

	CLinkList<SGameCompat>  *GetGameCompatabilityList() { return &m_lGames; }

	Utils::WString fileSizeString() const;

	CLinkList<SNeededLibrary> *GetNeededLibraries() { return &m_lNeededLibrarys; }
	void addNeededLibrary(const Utils::WString &scriptName, const Utils::WString &author, const Utils::WString &minVersion);
	bool isPackageNeeded(const Utils::WString &scriptName, const Utils::WString &author);
	SNeededLibrary *findPackageNeeded(const Utils::WString &scriptName, const Utils::WString &author);
	void removePackageNeeded(const Utils::WString &scriptName, const Utils::WString &author);
	void ClearNeededPackages();
	bool AnyDependacies() { return (m_lNeededLibrarys.size()) ? true : false; }
	bool anyDependacies() const { return (m_lNeededLibrarys.size()) ? true : false; }
	bool AutoGenerateUpdateFile() { return m_bAutoGenerateUpdateFile; }
	void removeFakePatchOrder(bool after, const Utils::WString &scriptName, const Utils::WString &author);
	void removeFakePatchOrder(const Utils::WString &scriptName, const Utils::WString &author);
	void addFakePatchOrder(bool after, const Utils::WString &scriptName, const Utils::WString &author);
	bool anyFakePatchOrder() const { if ( !_lFakePatchBefore.empty() || !_lFakePatchAfter.empty() ) return true; return false; }
	const Utils::WStringList &getFakePatchBeforeOrder() const { return _lFakePatchBefore; }
	const Utils::WStringList &getFakePatchAfterOrder() const { return _lFakePatchAfter; }
	void updateTextDB() { this->_resetTextDB(); }

	virtual bool readWares(int iLang, CLinkList<SWareEntry> &list, const Utils::WString &empWares);
	virtual bool readCommands(int iLang, CLinkList<SCommandSlot> &list);
	virtual bool readWingCommands(int iLang, CLinkList<SCommandSlot> &list);

	int  FindFirstGameInPackage();
	bool IsAnyGameInPackage();
	bool IsMultipleGamesInPackage();
	bool IsGameInPackage(int game);

	virtual BaseFileType type () const { return BaseFileType::TYPE_BASE; }
	virtual int GetType() { return type(); }
	bool AnyFileType ( int type );
	CBaseFile *GetParent () { return m_pParent; }
	void SetParent ( CBaseFile *file ) { m_pParent = file; }
	int parseLanguage(const Utils::WString &lang) const;

	virtual bool computeSigned(bool updateFiles) const;
	bool updateSigned (bool updateFiles);
	int GetNum() { return m_iNum; }
	void SetNum(int i) { m_iNum = i; }

	bool	IsFullyLoaded() { return m_bFullyLoaded; }
	virtual bool   IsSigned ()		{ return m_bSigned;}
	void SetOverrideFiles(bool b) { m_bOverrideFiles = b; }
	bool IsUpdateChecked () { return m_bUpdate; }
	void SetUpdateChecked ( bool en ) { m_bUpdate = en; }

	unsigned char *createData(size_t *size, CProgressInfo *progress = NULL);

protected:
	virtual void Delete ();
	virtual void SetDefaults ();

	// reading of files
	virtual bool _checkHeader(const Utils::WString &header) const;
	bool _parseHeader(const Utils::WString &header);
	bool _parseFileHeader(const Utils::WString& header);
	bool _parseFilesLine(const Utils::WString& line);
	void _readValues(const Utils::WString& values);
	void _readFiles(const Utils::WString& values);

	void _install_adjustFakePatches(CPackages *pPackages);
	void _install_renameText(CPackages *pPackages);
	bool _install_uncompress(C_File *fit, CProgressInfo *progress, Utils::WStringList *errorStr, bool *uncomprToFile);
	bool _install_setEnabled(bool bEnable, C_File *fit);
	bool _install_checkVersion(C_File *pFile, const Utils::WString &sDestination);
	Utils::WString _install_adjustFilepointer(C_File *pFile, bool bEnabled, const Utils::WString &sDestination);
	C_File *_install_checkFile(C_File *pFile, Utils::WStringList *errorStr, bool *bDoFile, CLinkList<C_File> *pFileList);
	bool _install_checkFileEnable(C_File *pCheckFile, C_File *fit, const Utils::WString &sDestination, bool bEnabled, Utils::WStringList *errorStr);
	bool _install_createDirectory(CDirIO &Dir, const Utils::WString &sTo, C_File *pFile, Utils::WStringList *errorStr);
	void _install_writeFile(C_File *pFile, const Utils::WString &sDestination, Utils::WStringList *errorStr);

	int _read_FileHeader(CFileIO &File, int iReadType, int iMaxProgress, int iDoneLen, CProgressInfo *pProgress);
	int _read_Header(CFileIO &File, int iReadType, int iMaxProgress, CProgressInfo *pProgress);
	CFileIO *_startRead();

	void _addFile(C_File *file, bool dontChange = false);
	void _updateTextDB(C_File *file);
	void _resetTextDB();
	void _addWaresToList(int iLang, CLinkList<SWareEntry> &list, const Utils::WString &wares, enum WareTypes eType);
	bool _readCommands(int iLang, int iStartID, CLinkList<SCommandSlot> &list);
	Utils::WString _replaceFilename(const Utils::WString &fname);

protected:
	SSPKHeader m_SHeader;
	SSPKHeader2 m_SHeader2;

	C_File *m_pIconFile;
	Utils::WString _sIconExt;
	Utils::WString _sLastError;
	int _iLastError;

	CLinkList<C_File>  m_lFiles;
	CLinkList<C_File>  m_lTempFiles;
	Utils::WStringList _lFakePatchBefore;
	Utils::WStringList _lFakePatchAfter;

	CTextDB	*_pTextDB;

	CLinkList<SGameCompat> m_lGames;
	Utils::WStringList _lGlobals;

	bool m_bSigned;
	bool m_bFullyLoaded;

	//installer varibles
	bool m_bEnable;
	bool m_bModifiedEnabled;
	bool m_bGlobal;
	bool m_bProfile;
	int m_iLoadError;

	CBaseFile *m_pParent;

	int m_iNum;

	CLinkList<SNeededLibrary> m_lNeededLibrarys;

	bool	_bCombineFiles;
	bool	m_bOverrideFiles;
	Utils::WString	_sFtpAddr;
	bool			m_bAutoGenerateUpdateFile;
	bool			m_bUpdate;
};


class SPKEXPORT CArchiveFile : public CBaseFile
{
public:
	CArchiveFile();
	virtual ~CArchiveFile();
	virtual Utils::WString getFullPackageName(const Utils::WString& format, int lang) const override { return L"Archive(" + name() + L")"; }
	virtual Utils::WString getFullPackageName(int language, const Utils::WString& byString) const override { return L"Archive(" + name() + L")"; }
	virtual Utils::WString getFullPackageName(int language, bool includeVersion = true, const Utils::WString& byString = L"by") const override { return L"Archive(" + name() + L")"; }

	virtual BaseFileType type() const override { return BaseFileType::TYPE_ARCHIVE; }
};

#endif //__BASEFILE_H__

