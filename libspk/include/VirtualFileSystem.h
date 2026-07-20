#pragma once

#include "File.h"
#include "Utils/WString.h"
#include <map>

class CBaseFile;
class CXspFile;

namespace Utils {
	class WStringList;
}
namespace SPK {
	class CTextDB;

tclass CVirtualFileSystem
{
public:
	typedef std::map<std::wstring, Utils::WString> FileList;
	typedef std::map<std::wstring, Utils::WString>::iterator FileListItr;

private:
	Utils::WStringList *_lAddons;
	Utils::WString	m_sAddon;
	Utils::WString	m_sDir;
	bool		m_bLoaded;

	FileList *m_pModMap;
	FileList *m_pMap;

	CTextDB	 *m_pTexts;
	CTextDB  *m_pModTexts;

	Utils::WStringList	*_lShips;
	Utils::WStringList	*_lShields;
	Utils::WStringList	*_lLasers;
	Utils::WStringList	*_lMissiles;
	Utils::WStringList	*_lCockpits;
	Utils::WStringList	*_lComponentSections;
	Utils::WStringList	*_lDummySections;
	Utils::WStringList	*_lBodiesSections;

	int m_iLang;

public:
	CVirtualFileSystem(void);
	~CVirtualFileSystem(void);

	// setters
	void setAddon(const Utils::WString &addon);
	void setLanguage(int iLang);

	// extractions
	void extractTexts(CXspFile *pPackage, int textId);
	C_File *extractGameFileToPackage(CBaseFile *pPackage, const Utils::WString &sFile, FileType iFileType, const Utils::WString &sTo);
	C_File *extractGameFileToPackage(CBaseFile *pPackage, const Utils::WString &sFile, FileType iFileType);
	Utils::WString extractGameFile(const Utils::WString &file, const Utils::WString &to);

	// loading
	bool LoadFilesystem(const Utils::WString &dir, int maxPatch = 0);
	bool LoadFilesystem(const Utils::WString &dir, const Utils::WString &mod, int maxPatch = 0);
	bool loadMod(const Utils::WString &mod);
	bool addMod(const Utils::WString &mod);

	// getters
	const Utils::WString &directory() const;
	bool isFileAvailable(const Utils::WString &file) const;
	Utils::WString getTShipsEntry(const Utils::WString &sId);
	Utils::WString findText(int iLang, int iPage, int iID) const;
	bool textExists(int iLang, int iPage, int iID) const;
	bool isTextUpdated() const;

	// Updates
	void updateModTexts(int iPage);
	void updateModTexts(int iFromPage, int iToPage);
	void updateTexts(int iPage);
	void updateTexts(int iFromPage, int iToPage);

	void clearMods(bool bIncludeStandard = false);

	// Iterators
	Utils::WStringList *getTShipsEntries();
	Utils::WString firstShield();
	Utils::WString nextShield();
	Utils::WString firstComponentSection();
	Utils::WString nextComponentSection();
	Utils::WString firstDummySection();
	Utils::WString nextDummySection();
	Utils::WString firstBodiesSection();
	Utils::WString nextBodiesSection();
	std::pair<Utils::WString, Utils::WString> firstLaser();
	std::pair<Utils::WString, Utils::WString> nextLaser();
	std::pair<Utils::WString, Utils::WString> firstMissile();
	std::pair<Utils::WString, Utils::WString> nextMissile();
	Utils::WString firstCockpit();
	Utils::WString nextCockpit();

	///////////////////////////////////////////////////////////////
	// Debug Functions
	void DEBUG_LogContents(const Utils::WString &sFile);	

private:
	void _updateTexts(int iFromPage, int iToPage, FileList *pFileList, CTextDB *pTextList);
	Utils::WString _convertExtension(const Utils::WString &sFile) const;
	void _clear();
	void _addFile(const Utils::WString &sFile, const Utils::WString &sDest, FileList *pList);
	void _addModFile(const Utils::WString &sFile, const Utils::WString &sMod, FileList *pList);
	Utils::WString _findFile(const Utils::WString &file) const;
	void _addDir(const Utils::WString &sStart, const Utils::WString &sDir);
	Utils::WString _extractFromCat(const Utils::WString &sCat, const Utils::WString &sFile, const Utils::WString &sTo);
	Utils::WString _extract(const Utils::WString &sFile, const Utils::WString &sTo);

	Utils::WStringList *_updateList(const Utils::WString &typesFile, int iTextPos);
	void _updateShields();
	void _updateLasers();
	void _updateMissiles();
	void _updateCockpits();
	void _updateComponentSections();
	void _updateDummySections();
	void _updateBodiesSections();
	void _updateShips();

	Utils::WString _returnText(Utils::WStringNode *s);
	Utils::WString _returnID(Utils::WStringNode *s);
	Utils::WString _returnLine(Utils::WStringNode *s);
	std::pair<Utils::WString, Utils::WString> _returnPair(Utils::WStringNode *s);
	Utils::WStringList *_updateSectionList(const Utils::WString &sFile, bool singleEntry);

	void _removeSameFile(const Utils::WString &sFile, const Utils::WString &sDest, const Utils::WString &ext, FileList *pList);

};

}