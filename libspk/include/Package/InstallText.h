#pragma once

#include "lists.h"
#include <Utils/WString.h>

namespace SPK {
namespace Package {

class SPKEXPORT CInstallText
{
private:
	typedef struct SInstallText {
		int				iLanguage;
		Utils::WString	sBefore;
		Utils::WString	sAfter;
	} SInstallText;

public:
	typedef CLinkList<SInstallText> TextList;
	typedef CListNode<SInstallText>* TextNode;

	friend class CInstallText;

private:
	TextList _lTexts;

public:
	CInstallText(void);
	virtual ~CInstallText(void);

	void add(int iLang, const Utils::WString &sBefore, const Utils::WString &sAfter);
	void addBefore(int iLang, const Utils::WString &sBefore);
	void addAfter(int iLang, const Utils::WString &sAfter);

	void remove(int iLang);
	void removeBefore(int iLang);
	void removeAfter(int iLang);

	Utils::WString getBefore(int iLang, bool bIncludeDefault = true) const;
	Utils::WString getAfter(int iLang, bool bIncludeDefault = true) const;
	bool any() const;

	void merge(const CInstallText *pText);
	unsigned int count() const;
	int language(unsigned int iPos) const;

private:
	SInstallText *_find(int iLang) const;
	SInstallText *_new(int iLang);

	void _purge();
	SInstallText *_default() const;
	SInstallText *_getAt(int iIdx) const;
};

}}//NAMESPACE