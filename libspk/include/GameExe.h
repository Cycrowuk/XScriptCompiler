#ifndef __GAME_EXE_H__
#define __GAME_EXE_H__

#include "lists.h"
#include "File.h"

enum {
	EXEFLAG_NONE			= 0,
	EXEFLAG_TCTEXT			= 1,
	EXEFLAG_MYDOCLOG		= 2,
	EXEFLAG_NOXOR			= 4,
	EXEFLAG_NOSAVESUBDIR	= 8,
	EXEFLAG_ADDON			= 16,
	EXEFLAG_SETTINGFILE		= 32,
};

#define EXE_VERSIONPOS			8
#define EXE_VERSION_NAMESTART	(EXE_VERSIONPOS + 2)
#define EXE_VERSION_SIZESTART	(EXE_VERSIONPOS + 1)

tstruct SGameExeVersion {
	Utils::WString	sName;
	int				iName;
	Utils::WString	fVersion;
	CLinkList<int> lSize;
} SGameExeVersion;

tstruct SGameExe {
	struct SGameExe() : iFlags(0), iMaxPatch(0), iName(0), iAudioStream(1), iAddonTo(0), iTextNum(0) { }
	Utils::WString sExe;
	Utils::WString sName;
	Utils::WString sModKey;
	int		 iFlags;
	int		 iMaxPatch;
	int		 iName;
	int		 iAudioStream;
	Utils::WString sMyDoc;
	Utils::WString sAddon;
	int		 iAddonTo;
	int		 iTextNum;
	CLinkList<SGameExeVersion> lVersions;
} SGameExe;

tstruct GameDirectory
{
	Utils::WString dir;
	Utils::WString name;
	Utils::WString version;
	int			  id;
	int			  langid;
	Utils::WString langname;
	Utils::WString logdir;
	Utils::WString addon;
	Utils::WString exe;
	Utils::WString basename;
} GameDirectory;

class SPKEXPORT CGameExe 
{
private:
	CLinkList<SGameExe> m_lExe;

public:
	void Reset();

	void parseExe(const Utils::WString &line);
	int  parseFlags(const Utils::WString &flags);
	bool readFile(const Utils::WString &file);

	int addExe(const Utils::WString &exe);
	SGameExe *gameExe(const Utils::WString &exe) const;

	int findVersion(const Utils::WString &exe, int size, Utils::WString *fVersion);

	int findAddonType(const Utils::WString &gameExe) const;
	int getGameType(const Utils::WString &gameExe) const;
	int getGameVersion(const Utils::WString &gameExe, Utils::WString *fVersion) const;
	bool getGameVersionName(const Utils::WString &gameExe, Utils::WString *versionName) const;
	Utils::WString extractGameName(const Utils::WString &dir, Utils::WString *extra = nullptr) const;
	Utils::WString gameRunExe(const Utils::WString &dir) const;
	Utils::WString gameName(const Utils::WString &gameExe) const;
	Utils::WString gameBaseName(const Utils::WString &gameExe) const;
	Utils::WString gameNameFromType(int type) const;
	Utils::WString gameVersionFromType(int game, int gameVersion, const Utils::WString &fGameVersion) const;
	Utils::WString gameDir(const Utils::WString &dir) const;
	Utils::WString properDir(const Utils::WString &dir) const;
	Utils::WString addonDir(const Utils::WString &dir) const;
	bool isAddon(const Utils::WString &ext) const;
	int getTextID(const Utils::WString &dir) const;
	int getGameAddons(const Utils::WString &dir, Utils::WStringList &exe) const;
	int gameFlags(int game);
	int maxPatch(int game);
	Utils::WString getModKey(int game) const;
	void getDirectoryData(GameDirectory *gameDir) const;

	int convertGameType(int gametype, int *version) const;

	int numGames() { return m_lExe.size(); }
	SGameExe *game(int game) const;
	unsigned int gameCount() const;

private:
	int _findExe(const Utils::WString &exe) const;
	int _findVersion(int exe, int size, Utils::WString *fVersion) const;

	void _setExeName(Utils::WString *sName, int *iName, const Utils::WString &n);
	Utils::WString _textFileName(const Utils::WString &sGameDir) const;
	Utils::WString _readTextFile(const Utils::WString &sGameDir) const;
	Utils::WString _extractTextData(const Utils::WString &sData, long iPage, long iID, int iGameID) const;
};


#endif //__GAME_EXE_H__
