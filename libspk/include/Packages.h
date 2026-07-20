#ifndef __PACKAGES_H__
#define __PACKAGES_H__

#include "SpkFile.h"
#include "GameExe.h"
#include "XspFile.h"
#include "archive/unzip.h"

#include "MultiSpkFile.h"
#include "VirtualFileSystem.h"

#include "spkdefines.h"

class CFileIO;

namespace SPK {
	class COriginalFiles;
}

using namespace SPK;

class SPKEXPORT CPackages
{
public:
	static Utils::WString ConvertLanguage(int lang);
	static SAvailablePackage *CreateAvailablePackageData(CBaseFile *package);
	static Utils::WString FormatAvailablePackageData(CBaseFile *package);
	static Utils::WString FormatAvailablePackageData(SAvailablePackage *package);
	static Utils::WString CreateFromPackagerScript(CPackages *packages, const Utils::WString &filename);
	static int GeneratePackageUpdateData(const Utils::WString &dir, bool includeSingle = true);

	CPackages ();
	~CPackages ();

	bool isOldDir(const Utils::WString &dir);
	bool read(const Utils::WString &dir, CProgressInfo *progress = NULL);
	void startup(const Utils::WString &dir, const Utils::WString &tempDir, const Utils::WString &myDoc);
	void startup(const Utils::WString &dir, const Utils::WString &tempDir, const Utils::WString &myDoc, const Utils::WString &mod);

	bool extractAll(CBaseFile *baseFile, const Utils::WString &dir, int game, bool includedir = true, CProgressInfo *progress = NULL) const;
	bool generatePackagerScript(CBaseFile *baseFile, bool wildcard, Utils::WStringList *list, int game, bool datafile = false) const;
	CBaseFile *loadPackagerScript(const Utils::WString &filename, int compression, Utils::WString (*askFunc)(const Utils::WString &), Utils::WStringList *malformedLines = NULL, Utils::WStringList *unknownCommands = NULL, Utils::WStringList *variables = NULL, CProgressInfo *progress = NULL);
	Utils::WStringList &getGlobals() { return _lGlobals; }
	Utils::WStringList &getFakePatchOrder() { return _lFakePatchOrder; }
	Utils::WString getCurrentDirectory() const;
	static const Utils::WString &tempDirectory() { return m_sTempDir; }
	const Utils::WString &myDocuments() { return m_sMyDoc; }
	CGameExe* GetGameExe() { return &m_gameExe; }
	const CGameExe* gameExe() const { return &m_gameExe; }
	CLinkList<CBaseFile> *GetInstallPackageList() { return &m_lInstallList; }

	// installing/uninstalling
	bool		installPackage(CBaseFile *package, Utils::WStringList *errors, CProgressInfo *progress = NULL, bool disabled = false);
	CBaseFile		*openPackage(const Utils::WString &file, int *error, CProgressInfo *progress = NULL, int readtype = SPKREAD_ALL, int flags = READFLAG_NONE) const;
	CMultiSpkFile	*openMultiPackage(const Utils::WString &file, int *error, CProgressInfo *progress = NULL);
	bool			 openMultiPackage(const Utils::WString &file, CLinkList<CBaseFile> *packageList, int *error, CProgressInfo *progress = NULL );
	int			CheckInstallPackage(CBaseFile *package, int check = IC_ALL);
	bool		removeFile(C_File *file, Utils::WStringList *errors = NULL);
	void		PrepareUninstallPackage(CBaseFile *package);
	bool		uninstallPreparedPackages(Utils::WStringList *errors, CProgressInfo *progress, CLinkList<CBaseFile> *uninstalledPackages = NULL, CLinkList<CBaseFile> *disabledPackages = NULL);
	void		ConvertOldPackage(CBaseFile *p) const;
	void		purgeUninstallScripts(CBaseFile *package, Utils::WStringList *errors);
	int 		PrepareInstallPackage(CBaseFile *package, bool disabled = false, bool force = false, int check = IC_ALL);
	int			installPreparedPackages(Utils::WStringList *errors, CProgressInfo *progress, CLinkList<CBaseFile> *errored, CLinkList<CBaseFile> *installedList = NULL);
	void		RemovePreparedInstall(CBaseFile *package);
	int			GetNumPackagesInQueue() { return m_lInstallList.size(); }
	int			checkOpenPackage(const Utils::WString &file, int* error) const;
	bool		findAllNeededDependacies(CBaseFile *p, const CLinkList<CBaseFile> &packages, CLinkList<CBaseFile> *foundPackages, bool onlyEnabled = false, bool includePrepared = false) const;
	int			getMissingDependacies(CBaseFile *p, Utils::WStringList *list, bool onlyEnabled = false, bool includePrepared = false);
	size_t		getDownloadableDependacies(CBaseFile* p, std::vector<const SAvailablePackage*>& list, bool onlyEnabled = false, bool includePrepared = false) const;
	bool		checkInstalledDependacy(const Utils::WString &name, const Utils::WString &author, const Utils::WString &version, bool onlyEnabled = false, bool includePerpered = false) const;
	bool		CheckEnabledDependacy(CBaseFile *p);
	int			GetDependacyList(CBaseFile *package, CLinkList<CBaseFile> *list);
	int			prepareMultiPackage(const Utils::WString &file, CLinkList<CBaseFile> *errorPackageList, int *error, CProgressInfo *progress = 0);

	bool		IsOldPluginManager() { return m_bOldPlugin; }
	int			findPackageFiles(CLinkList<CBaseFile> &packages, const Utils::WString &dir);
	int			findPackageDirectories(CLinkList<CBaseFile> &packages, const Utils::WString &dir);
	int			findAllPackages(CLinkList<CBaseFile> &packages, const Utils::WString &dir);
	size_t		updateFoundPackages(const Utils::WString& dir);
	size_t		addFoundPackages(const Utils::WString& dir);

	// enable/disable
	bool enablePackage(CBaseFile *package, Utils::WStringList *errors, CProgressInfo *progress = NULL );
	bool disablePackage(CBaseFile *package, Utils::WStringList *errors, CProgressInfo *progress = NULL );
	bool disablePreparedPackages(Utils::WStringList *errors, CProgressInfo *progress, CLinkList<CBaseFile> *disabledPackages = NULL );
	bool enablePreparedPackages(Utils::WStringList* errors, CProgressInfo* progress, CLinkList<CBaseFile>* enabledPackages = NULL);
	bool PrepareEnablePackage(CBaseFile *package);
	bool PrepareDisablePackage(CBaseFile *package);
	int  GetNumPackagesInEnabledQueue() { return m_lEnableList.size(); }
	int  GetNumPackagesInDisabledQueue() { return m_lDisableList.size(); }
	bool PrepareDisableForVanilla();
	bool PrepareEnableLibrarys();
	bool PrepareEnableFromVanilla();

	void WriteData();
	bool closeDir(Utils::WStringList *errors = 0, CProgressInfo *progress = NULL, bool removedir = false);
	bool RestoreFakePatch();
	bool ReadyFakePatch();
	bool checkValidPluginManagerFile(const Utils::WString &filename) const;
	bool checkIfPluginManagerFile(const Utils::WString &filename) const;

	// package control
	void UpdateUsedFiles(CLinkList<CBaseFile> *ignoreList = NULL, bool = true);
	void UpdateSigned();
	bool UpdatePackages(int doStatus = -1, bool individual = false);
	CBaseFile *findXspPackage(const Utils::WString &id) const;
	CBaseFile *findSpkPackage(const Utils::WString &name, const Utils::WString &author) const;
	CArchiveFile *findArchivePackage(const Utils::WString &name) const;
	CBaseFile* findPackage(const Utils::WString &name, const Utils::WString &author) const;
	CBaseFile *findFirstPackageWithFile(C_File *f) const;
	CBaseFile *findNextPackageWithFile(CBaseFile *p, C_File *f) const;
	CBaseFile *findPackage(CBaseFile *package) const;
	C_File *findFile(FileType filetype, const Utils::WString &filename, const Utils::WString &dir = Utils::WString::Null()) const;
	CBaseFile *FirstPackage() { return m_lPackages.First(); }
	CBaseFile *NextPackage() { return m_lPackages.Next(); }
	CBaseFile *GetFirstPackage() { if ( m_lPackages.Front() ) return m_lPackages.Front()->Data(); return NULL; }
	CBaseFile *GetNextPackage(CBaseFile *from) 
	{ 
		bool found = false;
		for ( CListNode<CBaseFile> *node = m_lPackages.Front(); node; node = node->next() )
		{
			if ( !node->Data() ) continue;
			if ( node->Data() == from )
				found = true;
			else if ( found )
				return node->Data();
		}
		return NULL; 
	}
	void Reset();
	CBaseFile *GetPackageAt(int i) { return m_lPackages[i]; }
	Utils::WString selectedModName() const;
	Utils::WString getModKey() const;
	void SetupWares();
	void SetupShips();
	void PurgeGameObjects();
	void PurgeWares();
	void PurgeShips();
	void StartPackage() { m_pPackageNode = m_lPackages.Front(); }
	void UpdatePackage(CBaseFile *p);
	CBaseFile *GetCurrentPackage() { return (m_pPackageNode) ? m_pPackageNode->Data() : NULL; }
	bool RemoveCurrentDirectory();
	void RemoveCreatedFiles();
	bool AnyUnusedShared();

	// util functions
	void removeUnusedDirectories(const Utils::WStringList &dirs, Utils::WStringList* errors = NULL);
	int  removeUninstallScripts(Utils::WStringList *errors = 0, CProgressInfo *progress = NULL);
	bool removeUninstallFile(C_File *file, Utils::WStringList *errors = 0);
	int  removeUnusedSharedFiles(Utils::WStringList *errors = 0, CProgressInfo *progress = NULL);
	bool removeSharedFile(C_File *file, Utils::WStringList *errors = NULL);
	void shufflePatchTo(C_File *file, int to, Utils::WStringList *errors);
	void shuffleFakePatches(Utils::WStringList *errors);
	void shuffleTextFiles(Utils::WStringList *errors);
	int  findNextFakePatch(int start = 0, const Utils::WString &dir = Utils::WString::Null()) const;
	unsigned int  findNextTextFile(unsigned int start = 4) const;
	unsigned int  findNextTextFile(const Utils::WString &dir, unsigned int start = 4) const;
	int  findLastFakePatch(int start = 99, const Utils::WString &dir = Utils::WString::Null()) const;
	int  findLastTextFile(int start = 9999, const Utils::WString &dir = Utils::WString::Null()) const;
	int  FindLowestFakePatchInstalled();
	void ReadGameLanguage(bool force = true);
	int	 removeAllPackages(Utils::WStringList *errors = NULL, CProgressInfo *progress = NULL);
	void AssignPackageNumbers();
	int  GetChildPackages(CBaseFile *package, CLinkList<CBaseFile> *children, bool recursive = false);
	int  GetAllPackageFiles(CBaseFile *package, CLinkList<C_File> *fileList, bool includeChild);
	int	 GetAllPackageFiles(CLinkList<CBaseFile> *list, CLinkList<C_File> *fileList, bool includeChild);
	void addLogEntry(int type, const Utils::WString &args, Utils::WStringList* errors);
	Utils::WString findDataDir(const Utils::WString &dir, const Utils::WString &file);
	int  countPackages(int type, bool onlyEnabled) const;
	int  CountBuiltInPackages(bool onlyEnabled);
	bool isCurrentDir(const Utils::WString &dir) const;
	bool CheckOtherPackage(CBaseFile *package);
	int CheckPreparedInstallRequired(CLinkList<CBaseFile> *list);
	static int ConvertWareType(wchar_t w);
	static wchar_t ConvertWareTypeBack(int w);
	Utils::WString logDirectory();
	Utils::WString logDirectory(const Utils::WString &gameExe);
	Utils::WString saveDirectory();
	void backupSaves() { backupSaves(m_bVanilla); }
	void backupSaves(bool vanilla);
	void restoreSaves() { restoreSaves(m_bVanilla); }
	void restoreSaves(bool vanilla);
	bool unPackFile(const Utils::WString &filename, bool checkxml = true) const;
	bool packFile(const Utils::WString &filename) const;
	bool packFile(CFileIO* File, const Utils::WString &filename) const;
	Utils::WString convertTextString(const Utils::WString &text);
	void LoadVirtualFileSystem();
	bool checkAccessRights(const Utils::WString &dir) const;
	bool readTextPage(const Utils::WString &file, Utils::WStringList &list, bool search, int page) const;
	size_t loadShipData(const Utils::WString& file, Utils::WStringList& list) const;
	FileType adjustFileType(const Utils::WString &name, FileType filetype) const;
	CXspFile *extractShip(const Utils::WString &sCatFile, const Utils::WString &sId, CProgressInfo *progress = NULL);
	Utils::WString readShipData(const Utils::WString &file, const Utils::WString &entry) const;
	bool isSamePackage(CBaseFile *p1, CBaseFile *p2) const;
	void applyFakePatchOrder(const Utils::WStringList &list);
	CBaseFile *createFromArchive(const Utils::WString &filename, bool toInstall = false) const;
	void readArchiveData(const char *buf, size_t len, CBaseFile *archive) const;
	void readArchiveData(const Utils::WString &filename, CBaseFile *archive) const;
	size_t verifyInstalledFiles(Utils::WStringList *missingFiles = nullptr, bool getPackages = true) const;
	Utils::WString empWaresForGame(size_t *maxsize = NULL);
	void addEMPPriceOverride(int empId, int price);
	void addEMPNotoOverride(int empId, int noto);
	void addBuiltInWarePriceOverride(int empId, int price);
	void addBuiltInWareNotoOverride(int empId, int noto);
	void addCustomWarePriceOverride(const Utils::WString &id, int price);
	void addCustomWareNotoOverride(const Utils::WString &id, int noto);

	int GetCurrentGameFlags() { return m_iGameFlags; }

	void setCurrentDir(const Utils::WString &dir);

	// merge mods
	void getMergedFiles(Utils::WStringList &list, CCatFile *cat1, CCatFile *cat2) const;
	bool canWeMerge(const Utils::WString &file) const;
	bool mergeMods(CCatFile *mod1, CCatFile *mod2, const Utils::WString &outFile, Utils::WStringList *cantMerge) const;
	bool needToMerge(const Utils::WString &file) const;
	bool getModCompatabilityList(C_File *file, Utils::WStringList *list = NULL) const;
	bool checkCompatabilityBetweenModFiles(C_File *from, C_File *to, Utils::WStringList *list = NULL) const;
	bool checkCompatabilityBetweenMods(CBaseFile *from, CBaseFile *to, Utils::WStringList *list = NULL) const;
	int checkCompatabilityAgainstPackages(CBaseFile *newFile, Utils::WStringList *list = NULL, CLinkList<CBaseFile> *packages = NULL) const;

	Utils::WString getLanguageName() const;
	int			  getGameLanguage() const;
	int			  getGameLanguage(const Utils::WString &dir) const;
	int			  getGameAddons(Utils::WStringList &exes) const;
	int			  getGameAddons(Utils::WStringList &exes, const Utils::WString &dir) const;
	Utils::WString getGameName() const;
	Utils::WString getGameName(const Utils::WString &dir) const;
	Utils::WString getGameVersionFromType(int game, int version, const Utils::WString &sVersion) const;
	Utils::WString getGameNameFromType(int game) const;
	Utils::WString getGameTypesString(CBaseFile *package, bool includeVersion) const;
	Utils::WString getGameVersionString(CBaseFile *package) const;
	Utils::WString getGameRunExe(const Utils::WString &dir) const;
	Utils::WString getGameRunExe() const;
	Utils::WString getProperDir() const;
	Utils::WString getProperDir(const Utils::WString &dir) const;
	Utils::WString getAddonDir() const;
	Utils::WString getAddonDir(const Utils::WString &dir) const;

	// text files
	void CreateLanguageTextFiles(Utils::WStringList *errors = 0);
	bool renameTextFile(const Utils::WString &textid, int languageid, Utils::WStringList *errors);
	void addTextFileToScripts(C_File *file, const Utils::WString &textid);
	void CreatePluginManagerText();
	void createPluginManagerOpenText();

	// game control
	void createEMPFile(const Utils::WString &progDir = Utils::WString::Null());
	void CreateWareFiles();
	int  extractGameFile(const Utils::WString &aFilename, const Utils::WString &aTo, const Utils::WString &dir = Utils::WString::Null(), const Utils::WString &addon = Utils::WString::Null()) const;
	void CreateDummies();
	void CreateComponants();
	void CreateCutData();
	void CreateBodies();
	void CreateGlobals();
	void CreateAnimations();
	void CreateTShips();
	void CreateCustomStarts();
	void addCreatedFile(const Utils::WString &file);
	
	bool readGlobals(Utils::WStringList &globals) const;
	bool readWares(int iLang, CLinkList<SWareEntry> &list);
	bool readCommands(int iLang, CLinkList<SCommandSlot> &list);
	bool readWingCommands(int iLang, CLinkList<SCommandSlot> &list);
	int  empOveridePrice(int id);
	bool empOverideNoto(int id, int *noto);
	int  builtInWareOveridePrice(int id);
	bool builtInWareOverideNoto(int id, int *noto);
	int  customWareOveridePrice(const Utils::WString &id);
	bool customWareOverideNoto(const Utils::WString &id, int *noto);
	void removeEmpOverride(int pos);
	void removeBuiltinWareOverride(int pos);
	void removeCustomWareOverride(const Utils::WString &id);

	// install text
	Utils::WString getInstallBeforeText(CBaseFile *package) const;
	Utils::WString getInstallAfterText(CBaseFile *package) const;
	Utils::WString getUninstallBeforeText(CBaseFile *package) const;
	Utils::WString getUninstallAfterText(CBaseFile *package) const;

	//setting functions
	void SetRenameText(bool b) { m_bRenameText = b; }
	void SetLanguage(int i) { m_iLanguage = i; }
	void SetAutoEnable(bool b) { m_bAutoEnableChild = b; }
	void SetForceModInstall(bool b) { m_bForceModInstall = b; }
	void setTempDirectory(const Utils::WString &s) { m_sTempDir = s; }
	void setMyDocuments(const Utils::WString &s) { m_sMyDoc = s; }
	void SetForceEMP(bool b) { m_bForceEMP = b; }
	void SetSurpressProtectedWarning(bool b) { m_bSurpressProtectedWarning = b; }
	void SurpressProtectedWarning() { m_bSurpressProtectedWarning = true; }
	bool IsSupressProtectedWarning() { return m_bSurpressProtectedWarning; }

	int language() const { return m_iLanguage; }
	int GetLanguage() { return m_iLanguage; }
	int GetGame() { return m_iGame; }
	bool IsLoaded() { return m_bLoaded; }
	bool IsVanilla() { return m_bVanilla; }
	void SetVanilla(bool b);

	const Utils::WString &getMod() const { return m_sSetMod; }
	void setMod(const Utils::WString& mod);
	void setSaveGameManager(bool managed);

	CBaseFile *GetEnabledMod() { return m_pEnabledMod; }

	//errors
	void ClearError() { m_iError = PKERR_NONE; }
	int  GetError() { return m_iError; }

	CLinkList<CBaseFile>* PackageList() { return &m_lPackages; }
	const CLinkList<CBaseFile>* packageList() const { return &m_lPackages; }
	CLinkList<SAvailablePackage> *getAvailablePackageList() { return &m_lAvailablePackages; }
	CLinkList<CBaseFile>* getFoundPackageList() { return &m_lFoundPackages; }
	bool AnyAvailablePackages(int type = -1);
	bool addAvailablePackage(SAvailablePackage *package);
	void saveAvailablePackages();
	void readAvailablePackages();
	void parseAvailablePackage(const Utils::WString &str, const Utils::WString &webaddress = Utils::WString::Null());
	const SAvailablePackage* findAvailablePackage(const Utils::WString& filename) const;
	const SAvailablePackage* findAvailablePackage(const Utils::WString& name, const Utils::WString& author) const;
	CBaseFile* findFoundPackage(const Utils::WString& name, const Utils::WString& author) const;
	int findAllServers(Utils::WStringList *list) const;

	CBaseFile *findScriptByAuthor(const Utils::WString &author, CBaseFile *prev = NULL);
	void RemoveFailedFiles();

private:
	void _processAddPackage(CBaseFile* p);
	size_t _createCockpits(Utils::WStringList &list);

	bool _checkForDisable(CBaseFile *package, bool disabled, CBaseFile *oldPackage);
	CBaseFile *_archive_fromRar(const Utils::WString &filename, bool toInstall) const;
	CBaseFile* _archive_fromZip(const Utils::WString& filename, bool toInstall) const;
	bool _check_archive_fromZip(const Utils::WString& filename) const;
	void _addToFakePatch(CBaseFile *pPackage);
	int _gameTextNumber() const;
	void _addWareOverride(enum WareTypes type, int pos, const Utils::WString &id, int value, bool noto);
	int _warePriceOverride(enum WareTypes type, int pos, const Utils::WString &id);
	bool _wareNotoOverride(enum WareTypes type, int pos, const Utils::WString &id, int *noto);
	void _removeWareOverride(enum WareTypes type, int pos, const Utils::WString &id);

private:
	Utils::WString	m_sCurrentDir;
	static Utils::WString	m_sTempDir;
	Utils::WString			m_sMyDoc;
	Utils::WString			m_sSetMod;
	Utils::WString			_sSaveDir;
	GameDirectory *_pCurrentDir;

	// global files list
	CLinkList<C_File>		m_lFiles;
	CLinkList<CBaseFile>	m_lPackages;
	CLinkList<C_File>		m_lUninstallFiles;
	CLinkList<SGameWare>	m_lGameWares[WAREBUFFERS];
	CLinkList<SGameShip>	m_lGameShips;
	CLinkList<SWarePriceOverride>	m_lWarePrices;
	CLinkList<CBaseFile>	m_lFoundPackages;
	COriginalFiles		   *_pOriginalFiles;

	// prepared lists
	CLinkList<CBaseFile>	m_lInstallList; // install/uninstalling packages
	CLinkList<CBaseFile>	m_lEnableList;  // enable packages
	CLinkList<CBaseFile>	m_lDisableList; // disable packages

	Utils::WStringList		_lCreatedFiles; // list of files we've created
	Utils::WStringList		_lNonRemovedFiles; // list of fiels that couldn't be removed for some reason
	Utils::WStringList		_lGlobals; // global settigns that we want changed
	Utils::WStringList		_lFakePatchOrder; // ordered fake patches
	CLinkList<SAvailablePackage> m_lAvailablePackages; // list of available packages online

	CVirtualFileSystem		m_pGameVFS; // Games Virtual File System

	CGameExe				m_gameExe;
	SGameExe				*m_pCurrentGameExe;
	CBaseFile			   *m_pEnabledMod;

	// settings
	bool		m_bRenameText;  // rename text files
	bool		m_bAutoEnableChild;	// auto enable all children
	bool		m_bForceModInstall; // allowsm ultiple mods to be installed
	bool		m_bOldPlugin;		// if its loading data from the old plugin manager
	bool		m_bLoadVFS;			// if we should load the virtual filesystem
	int			m_iLanguage;	// Language id to use
	int			m_iSaveGameManager; // Save game manager
	bool		m_bSurpressProtectedWarning; // prevent checking for protected directory

	int			m_iGame;		// the game the directory is
	int			m_iGameVersion; // the version of the game (version position)
	Utils::WString	_sGameVersion; // the version of the game (full version)
	int			m_iGameFlags;	// the flags for the game from SGameExe
	int			m_iMaxPatch;	// the maximum patch for the game

	int			m_iSaveGame;
	int			m_iError;
	int			m_iLastUpdated;
	int			m_iFakePatch;		// The current fake patch

	bool		m_bVanilla;		// currently in vanilla mode, dont allow unsigned packages to be installed

	bool		m_bLoaded;
	bool		m_bRedo;
	bool		m_bUsedWare;	// if we have used the ware files in the directory, not in a patch
	bool		m_bRemoveDir;	// shall we remove direcotires;

	int			m_iWareBuffer[WAREBUFFERS];	//Ware Buffers
	int			m_iShipBuffer; //Ship Buffers

	bool		m_bDisableVanilla;
	bool		m_bForceEMP;

	CListNode<CBaseFile>	*m_pPackageNode;
};

#endif // __PACKAGES_H__
