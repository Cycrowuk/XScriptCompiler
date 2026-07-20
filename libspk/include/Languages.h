#ifndef __LANGUAGES_H__
#define __LANGUAGES_H__

#include "Utils\WString.h"
#include <map>
#include <vector>

enum Lang_Section 
{ 
	LS_STARTUP			= 1, 
	LS_DIRECTORY,
	LS_FILEDIALOG,
	LS_SCRIPTTYPE,
};

enum Lang_Startup
{ 
	LANGSTARTUP_ANOTHERINSTANCE_TITLE	= 1,
	LANGSTARTUP_ANOTHERINSTANCE,
	LANGSTARTUP_PROTECTEDDIR,
	LANGSTARTUP_LOCKEDDIR,
	LANGSTARTUP_LOCKEDDIR_TITLE,
};

enum Lang_Directory
{
	LANGDIR_TITLE	 = 1,
};

typedef std::map<int, Utils::WString> LangPage;
typedef std::map<int, LangPage> LangPages;
typedef std::map<int, LangPages> LangTexts;

class SPKEXPORT CLanguages
{
public:
	static CLanguages *Instance();
	static void Release();

	void setLanguage(int lang);
	void pushLanguage(int lang);
	void popLanguage();

	Utils::WString findText(int section, int id);

protected:
	CLanguages();
	~CLanguages();

private:
	void DEBUG_AddDefaultTexts();

	LangPages *_findLanguageText(int id = -1) const;
	Utils::WString _error(int section, int id) const;
	bool _findText(LangPages *texts, int section, int id, Utils::WString *out) const;

private:
	static CLanguages *_pInstance;

	int			_iLanguage;
	LangTexts	*_lTexts;
	LangPages	*_pDefaultLang;
	LangPages	*_pCurrentLang;

	std::vector<int> *_langStack;
};

#endif //__LANGUAGES_H__
