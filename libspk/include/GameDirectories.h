#pragma once

#include <vector>
#include "spkdefines.h"
#include "Utils/WStringList.h"

class CPackages;
class CGameExe;

namespace SPK {

	class CVirtualFileSystem;

	typedef struct SGameDir {
		bool			bLoad;
		Utils::WString	dir;
		Utils::WString	name;
		int				iGame;
		CVirtualFileSystem *pVfs;
//		SGameExe		*pExe;
	} SGameDir;

	tclass CGameDirectories
	{
	private:
		std::vector<SGameDir *> *_pDirs;
		Utils::WStringList	_lControlledDirs;
		SGameDir	*_pCurrentItr;
		SGameDir	*_pSelected;
		SGameDir	*_pTemporary;
		int			 _iLanguage;

	public:
		CGameDirectories(const Utils::WString &mydoc);
		~CGameDirectories(void);

		bool add(const Utils::WString &dir, const Utils::WString &name, int iGame, const Utils::WString &addon, bool bLoad);
		bool remove(const Utils::WString &dir);

		bool parse(const Utils::WString &data, CPackages *pPackages);
		bool findDir(const Utils::WString &dir);
		bool writeData(Utils::WStringList* lines);
		bool writeData(std::vector<Utils::WString> &lines);
		void updateCurrentVFS(bool bReload);

		void clear();

		bool setSelectedGameDirectory(int iGame, bool temp = false);
		bool setSelectedDirectory(const Utils::WString &dir, bool temp = false);
		void reselectTemporaryDirectory();
		void setLanguage(int iLang);

		void setLoad(bool bLoad);

		bool isAllTextLoaded() const;
		bool isDir(const Utils::WString &dir) const;
		bool hasSelected() const;
		bool isEmpty() const;
		int highestGame() const;

		Utils::WString currentName() const;
		int currentGame() const;
		bool currentLoad() const;
		CVirtualFileSystem *selectedVFS() const;

		void syncWithControlled(CGameExe *exe);

		Utils::WString findText(int iGame, int iLanguage, int iPage, int iID);
		Utils::WString findText(int iLanguage, int iPage, int iID, Utils::WString missing = Utils::WString::Null());

		Utils::WString first(int iGame = -1);
		Utils::WString next(int iGame = -1);

		Utils::WString firstShield();
		Utils::WString nextShield();
		Utils::WString firstComponentSection();
		Utils::WString nextComponentSection();
		Utils::WString firstDummySection();
		Utils::WString nextDummySection();
		Utils::WString firstBodiesSection();
		Utils::WString nextBodiesSection();
		Utils::WString firstCockpit();
		Utils::WString nextCockpit();
		std::pair<Utils::WString, Utils::WString> firstLaser() const;
		std::pair<Utils::WString, Utils::WString> nextLaser() const;
		std::pair<Utils::WString, Utils::WString> firstMissile() const;
		std::pair<Utils::WString, Utils::WString> nextMissile() const;

	private:
		SGameDir *_findGameDir(int iGame) const;	
		SGameDir *_findGameDir(const Utils::WString &dir) const;	
		void _add(const Utils::WString &dir, const Utils::WString &name, int iGame, const Utils::WString &addon, bool bLoad);

		void _updateControlledDirs(const Utils::WString &mydoc);
	};

}