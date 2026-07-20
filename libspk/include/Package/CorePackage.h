#pragma once

#include "spkdefines.h"
#include "../Utils/List.h"
#include <map>

enum {PLUGIN_NORMAL, PLUGIN_STABLE, PLUGIN_EXPERIMENTAL, PLUGIN_CHEAT, PLUGIN_MOD}; // filters for browsing packages

namespace SPK {
namespace Package {


class CInstallText;

class SPKEXPORT CCorePackage
{
private:
	Utils::WString _sName;
	Utils::WString _sVersion;
	Utils::WString _sAuthor;
	Utils::WString _sWebSite;
	Utils::WString _sWebAddress;
	Utils::WString _sEmail;
	Utils::WString _sDescription;
	Utils::WString _sCreationDate;
	Utils::WString _sForumLink;

	Utils::WString _sFilename;
	Utils::WString _sExportFilename;

	std::map<unsigned int, Utils::WString> *_pAutoExtract;
	std::map<unsigned int, Utils::WString> *_pAutoExport;
	Utils::CList<SNames> *_lNames;

	CInstallText *_pInstallText;
	CInstallText *_pUninstallText;
	Utils::WStringList _lMirrors;

	int _iPluginType;

	// package stats
	int _iRecommended;
	int _iEaseOfUse;
	int _iGameChanging;

	bool _bChanged;

public:
	CCorePackage(void);
	virtual ~CCorePackage(void);

	//getters
	const Utils::WString &name				() const { return _sName; }
	const Utils::WString &version	 		() const { return _sVersion; }
	const Utils::WString &author			() const { return _sAuthor; }
	const Utils::WString &webSite			() const { return _sWebSite; }
	const Utils::WString &webAddress		() const { return _sWebAddress; }
	const Utils::WString &email				() const { return _sEmail; }
	const Utils::WString &creationDate		() const { return _sCreationDate; }
	const Utils::WString &description		() const { return _sDescription; }
	const Utils::WString &forumLink			() const { return _sForumLink; }
	int					 recommended		() const { return _iRecommended; }
	int					 easeOfUse			() const { return _iEaseOfUse; }
	int					 gameChanging		() const { return _iGameChanging; }
	const Utils::WString &filename			() const { return _sFilename; }
	const Utils::WString &exportFilename	() const { return _sExportFilename; }
	const CInstallText  *installText		() const { return _pInstallText; }
	const CInstallText  *uninstallText		() const { return _pUninstallText; }
	int					 pluginType			() const { return _iPluginType; }
	bool				 hasChanged			() const { return _bChanged; }
	const Utils::WString	&name			(int lang) const;
	const std::map<unsigned int, Utils::WString> *autoExtraction() const { return _pAutoExtract; }
	const std::map<unsigned int, Utils::WString> *autoExporter() const { return _pAutoExport; }
	const Utils::CList<SNames> *namesList() const { return _lNames; }

	//setters
	void setName			( const Utils::WString &str ) { _changed(); _sName = str.remove(L'|'); }
	void setVersion			( const Utils::WString &str ) { _changed(); _sVersion = str; }
	void setAuthor			( const Utils::WString &str ) { _changed(); _sAuthor = str.remove(L'|'); }
	void setWebAddress		( const Utils::WString &str ) { _changed(); _sWebAddress = str; }
	void setWebSite			( const Utils::WString &str ) { _changed(); _sWebSite = str; }
	void setEmail			( const Utils::WString &str ) { _changed(); _sEmail = str; }
	void setCreationDate	( const Utils::WString &str ) { _changed(); _sCreationDate = str; }
	void setDescription		( const Utils::WString &str ) { _changed(); _parseDescription(str); }
	void setFilename		( const Utils::WString &str ) { _sFilename = str; }
	void setExportFilename	( const Utils::WString &str ) { _sExportFilename = str; }
	void setForumLink		( const Utils::WString &str ) { _changed(); _sForumLink = str; }
	void setRecommended		( int i )					 { _changed(); _iRecommended = i; }
	void setGameChanging	( int i )					 { _changed(); _iGameChanging = i; }
	void setEaseOfUse		( int i )					 { _changed(); _iEaseOfUse = i; }
	void setPluginType		( int i )					 { _changed(); _iPluginType = i; }
	void adjustChanged		( bool bChanged )			 { _bChanged = bChanged; }

	void addAutoExtract(unsigned int game, const Utils::WString &dir);
	void addAutoExport(unsigned int game, const Utils::WString &file);
	void clearAutoExtract();
	void clearAutoExport();

	void addName(int iLang, const Utils::WString& name);
	void removeName(int iLang);
	void clearNames();

	// install texts
	Utils::WString installText(int iLang, bool bBefore, bool bIncludeDefault = true) const;
	Utils::WString uninstallText(int iLang, bool bBefore, bool bIncludeDefault = true) const;
	void addInstallText(int iLang, bool bBefore, const Utils::WString &sText);
	void addUninstallText(int iLang, bool bBefore, const Utils::WString &sText);
	void removeInstallText(int iLang, bool bInstall);

	// Web Mirrors
	bool anyWebMirrors() { return !_lMirrors.empty(); }
	const Utils::WStringList& webMirrors() const { return _lMirrors; }
	size_t getMaxWebMirrors() { return _lMirrors.size(); }
	Utils::WString getWebMirror(size_t i) { if (i >= 0 && i < _lMirrors.size()) return _lMirrors.get(i)->str; return Utils::WString::Null(); }
	void clearWebMirrors() { _lMirrors.clear(); }
	void addWebMirror(const Utils::WString& str);
	void removeWebMirror(const Utils::WString& str);

protected:
	virtual void _changed() { _bChanged = true; }
	virtual void _setDefaults();
	void _setRatings(int iEase, int iChanging, int iRec);
	void _parseDescription(const Utils::WString &sDesc);

	bool _noRatings() const { return (_iRecommended == -1 && _iEaseOfUse == -1 && _iGameChanging == -1) ? true : false; }
	void _merge(CCorePackage *pPackage);

	Utils::WString _installText(int iLang, bool bBefore, bool bIncludeDefault, const CInstallText *pText) const;
	void _addInstallText(int iLang, bool bBefore, const Utils::WString &sText, CInstallText *pText);
};


}}//NAMESPACE