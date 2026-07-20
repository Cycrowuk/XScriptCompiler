#pragma once
#include "BaseFile.h"
#include "spkdefines.h"

class SPKEXPORT CSpkFile : public CBaseFile
{
public:
	enum {
		SCRIPTTYPE_CUSTOM,
		SCRIPTTYPE_OTHER,
		SCRIPTTYPE_NAVIGATION,
		SCRIPTTYPE_COMBAT,
		SCRIPTTYPE_MISSION,
		SCRIPTTYPE_ALPLUGIN,
		SCRIPTTYPE_HOTKEY,
		SCRIPTTYPE_SHIPUPGRADE,
		SCRIPTTYPE_SHIPCOMMAND,
		SCRIPTTYPE_STATIONCOMMAND,
		SCRIPTTYPE_FLEET,
		SCRIPTTYPE_TRADE,
		SCRIPTTYPE_PIRACY,
		SCRIPTTYPE_CHEAT,
		SCRIPTTYPE_EXTENSION,
		SCRIPTTYPE_REBALANCE,
		SCRIPTTYPE_FIX,
		SCRIPTTYPE_GENERALMOD,
		SCRIPTTYPE_TOTAL,
		SCRIPTTYPE_WINGCOMMAND,
		SCRIPTTYPE_MAX,
		SCRIPTTYPE_UNKNOWN,
		SCRIPTTYPE_LIBRARY,
		SCRIPTTYPE_UPDATE,
		SCRIPTTYPE_CUSTOMSTART,
		SCRIPTTYPE_PATCH,
		SCRIPTTYPE_FAKEPATCH,
	};

private:
	enum {
		SETTING_INTEGER,
		SETTING_STRING,
		SETTING_CHECK
	};

	typedef struct SSettingString : public SSettingType {
		SSettingString () { iType = SETTING_STRING; }
		Utils::WString sValue;
	} SSettingString;

	typedef struct SSettingInteger : public SSettingType {
		SSettingInteger () : iValue(0) { iType = SETTING_INTEGER; }
		int iValue;
	} SSettingInteger;

	typedef struct SSettingCheck : public SSettingType {
		SSettingCheck () : bValue(false) { iType = SETTING_CHECK; }
		bool bValue ;
	} SSettingCheck;


public:
	static int ConvertScriptType(const Utils::WString &sType);
	static Utils::WString GetScriptTypeStringStatic(int type, int iLang);
	static Utils::WString GetWareText(SWares *w, int lang);
	static Utils::WString GetWareDesc(SWares *w, int lang);

	// get functions
	Utils::WString scriptTypeString(int lang) const;
	Utils::WString customScriptType () const { return m_sScriptType; }
	Utils::WString customScriptType (int lang) const;
	const Utils::WString &otherName() const { return m_sOtherName; }
	const Utils::WString &otherAuthor()	const { return m_sOtherAuthor; }
	CLinkList<SWares> *GetWaresList() { return &m_lWares; }
	CLinkList<SSettingType> *settingsList() { return &m_lSettings; }

	bool CheckValidReadmes () const;

	bool   IsCustomStart () const	{ return (m_iPackageType == PACKAGETYPE_CUSTOMSTART) ? true : false;  }
	bool   IsPackageUpdate () const { return (m_iPackageType == PACKAGETYPE_UPDATE) ? true : false; }
	bool   IsPatch () const			{ return (m_iPackageType == PACKAGETYPE_PATCH) ? true: false; }
	bool   IsLibrary ()	const		{ return (m_iPackageType == PACKAGETYPE_LIBRARY) ? true: false; }
	bool   IsAnotherMod() const { if ((m_sOtherName.empty()) || (m_sOtherAuthor.empty())) return false; return true; }
	bool   isAnotherMod() const { if ((m_sOtherName.empty()) || (m_sOtherAuthor.empty())) return false; return true; }
	bool   IsForceProfile() const	{ return m_bForceProfile; }
	int		GetScriptType() const	{ return m_iScriptType; }
	int		GetPackageType() const	{ return m_iPackageType; }

	virtual bool readWares(int iLang, CLinkList<SWareEntry> &list, const Utils::WString &empWares);

	// Custom Start
	Utils::WString customStartName() const;

	// set functions
	void setScriptType	 ( const Utils::WString &str ) { m_sScriptType = str; _changed(); }
	void setAnotherMod   ( const Utils::WString &name, const Utils::WString &author ) { m_sOtherName = name; m_sOtherAuthor = author; _changed(); }
	void SetForceProfile ( bool p ) { m_bForceProfile = p; _changed(); }

	void SetPackageType(int i)
	{
		m_iPackageType = i;
		if ( i == PACKAGETYPE_UPDATE )
		{
			if ( !this->findPackageNeeded(L"<package>", L"<author>") )
				this->addNeededLibrary(L"<package>", L"<author>", L"1.00");
		}
		else
			this->removePackageNeeded(L"<package>", L"<author>");
		_changed();
	}
	void SetPatch() { this->SetPackageType(PACKAGETYPE_PATCH); }
	void SetLibrary() { this->SetPackageType(PACKAGETYPE_LIBRARY); }
	void SetCustomStart() { this->SetPackageType(PACKAGETYPE_CUSTOMSTART); }
	void SetPackageUpdate() { this->SetPackageType(PACKAGETYPE_UPDATE); }
	void SetNormalPackage() { this->SetPackageType(PACKAGETYPE_NORMAL); }
	void setScriptType(int i) { SetNormalPackage(); m_iScriptType = i; _changed(); }

	virtual bool loadPackageData(const Utils::WString &sFirst, const Utils::WString &sRest, const Utils::WString &sMainGame, Utils::WStringList &otherGames, Utils::WStringList &gameAddons, CProgressInfo *progress = NULL) override;
	virtual bool GeneratePackagerScript(bool wildcard, Utils::WStringList *list, int game, const Utils::WStringList &gameAddons, bool datafile = false) override;

	CSpkFile();
	virtual ~CSpkFile();

	bool isMatchingMod(const Utils::WString &mod) const;

	/// reading of files
	virtual bool parseValueLine(const Utils::WString &line) override;

	// writing of files
	virtual Utils::WString createValuesLine() const override;
	bool writeHeader(CFileIO &file, int iHeader, int iLength) const override;

	SWares *findWare(const Utils::WString &id) const;
	void addWare(const Utils::WString &sWareLine);
	void addWareText(const Utils::WString &sText);
	void addWare(SWares *pWare);
	void addWareText(SWares *pWare, int iLang, const Utils::WString &sName, const Utils::WString &sDesc);
	void removeWare(const Utils::WString &sWare);
	void removeWareText(const Utils::WString &sName, int iLang);
	void clearWareText(SWares *pWare);
	void clearWareText(const Utils::WString &sWare);
	void clearWares ();
	bool anyWares() const { return !m_lWares.empty(); }

	int CheckValidCustomStart () const;

	virtual bool computeSigned(bool updateFiles) const override;

	SSettingType *addSetting(const Utils::WString &key, int type);
	void clearSettings ();
	void convertSetting(SSettingType *t, const Utils::WString &set) const;
	Utils::WString getSetting(SSettingType *t) const;
	bool anySettings() const { return !m_lSettings.empty(); }

	// installer usage
	const Utils::WString &lastReadme () const { return m_sLastReadme; }
	const Utils::WString &customMap () const { return m_sCustomMap; }

	void setCustomMap ( const Utils::WString &map ) { m_sCustomMap = map; }
	void setLastReadme ( const Utils::WString &r ) { m_sLastReadme = r; }

	virtual int GetType () { return TYPE_SPK; }
	virtual BaseFileType type() const override { return BaseFileType::TYPE_SPK; }

	void MergePackage(CBaseFile *p);

	// old conversion functions
	bool convertOld(const Utils::WString &sOldFilename);
	static CSpkFile *convertFromOld(const Utils::WString &sOldFilename);

protected:
	virtual void Delete ();
	virtual void SetDefaults ();
	virtual bool _checkHeader(const Utils::WString &header) const override;

	// old conversion functions
	unsigned char *_convert_uncompressFile(const Utils::WString &sOldFilename, int *pLen);
	void _convert_parse(const Utils::WString &sCmd, const Utils::WString &sRest);
	Utils::WString _convert_fileEndString(const Utils::WString &sFile);
	FileType _convert_fileType(const Utils::WString &sFile);
	Utils::WString _convert_parseFilename(const Utils::WString &sRest, float fVersion, Utils::WString *pDir);
	unsigned char *_convert_parseFile(const Utils::WString &s_Cmd, const Utils::WString &s_Rest, float fVersion, unsigned char *d);

	// varibles
	Utils::WString m_sOtherAuthor;
	Utils::WString m_sOtherName;
	Utils::WString m_sScriptType;

	bool m_bForceProfile;
	int m_iPackageType;
	int m_iScriptType;

	CLinkList<SWares> m_lWares;
	CLinkList<SSettingType> m_lSettings;

	SWares *m_pLastWare;

	// installer varibles
	Utils::WString m_sLastReadme;
	Utils::WString m_sCustomMap;
};
