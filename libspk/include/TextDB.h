#pragma once

#include <map>
#include <set>
#include "Utils/WString.h"

namespace SPK {
class CTextDB
{
public:
	typedef std::map<std::wstring, Utils::WString> TextList;
	typedef std::map<std::wstring, Utils::WString>::iterator TextListItr;

private:
	int			 m_iLang;
	int			 m_iInPage;
	int			_iInGame;
	TextList	*m_pTexts;
	TextList	*m_pTextComment;
	std::vector<unsigned int> _lGameOrder;
	std::set<unsigned int> _lGames;
	bool		_bSortedGames;

public:
	CTextDB(void);
	virtual ~CTextDB(void);

	void parseTextFile(int iFromPage, int iToPage, const Utils::WString &sFile, int iLang);
	Utils::WString get(int iLang, int iPage, int iID) const;
	bool exists(int iLang, int iPage, int iID) const;
	Utils::WString get(int iPage, int iID) const;
	bool exists(int iPage, int iID) const;
	bool anyTextLoaded() const;

	void setLanguage(int iLang);

private:
	Utils::WString _parseText(int iLang, const Utils::WString &sText, const Utils::WString &sReplace) const;
	Utils::WString _parseText(int iLang, const Utils::WString &sText) const;
	void _parsePage(int iLang, const Utils::WString &sLine);
	void _parseFileLine(int iFromPage, int iToPage, int iLang, const Utils::WString &sLine);
	void _addText(int iLang, int iID, const Utils::WString &sText);
	Utils::WString _mapID(int iLang, int iPage, int iID) const;
	void _sortGames();
};

} //NAMESPACE
