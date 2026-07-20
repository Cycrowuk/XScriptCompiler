#ifndef _XSPFILE_H__
#define _XSPFILE_H__

#include "BaseFile.h"
#include "ShipData.h"
#include "VirtualFileSystem.h"

class CCatFile;

enum class ShipyardRace
{
	None			= 0,
	Argon			= 1,
	Boron			= 2,
	Paranid			= 4,
	Split			= 8,
	Teladi			= 16,
	Pirates			= 32,
	Friend			= 64,
	Xenon			= 128,
	Terran			= 256,
	ATF				= 512,
	Yaki			= 1024,
	Goner			= 2048,
	Max				= 2048,
};

enum {TAT_NONE, TAT_SINGLE, TAT_3};

#define TSHIPPOS_CLASS		53

enum {IMPORTSHIP_NONE, IMPORTSHIP_SCENE, IMPORTSHIP_EXTRACTSCENE, IMPORTSHIP_DUMMIES, IMPORTSHIP_COMPONANT, IMPORTSHIP_MODELS, IMPORTSHIP_TEXTURES, IMPORTSHIP_TEXTS, IMPORTSHIP_BODIES, IMPORTSHIP_COCKPITS, IMPORTSHIP_PACK, IMPORTSHIP_DONE};

unsigned long  SPKEXPORT GetMaxShipyards ();
Utils::WString SPKEXPORT GetShipyardName (ShipyardRace s);

typedef struct STypesSection {
	Utils::WString		sSection;
	Utils::WStringList	lEntries;
} STypesSection2;

typedef struct SComponentEntry {
	Utils::WString		sSection;
	Utils::CList<STypesSection> lEntries;
} SComponentEntry;

typedef struct SWeaponMask {
	int		iMask;
	int		iGame;
} SWeaponMask;

typedef struct SText
{
	int		iId;
	Utils::WString sName;
	Utils::WString sDesc;
} SText;

typedef struct SDummy
{
	Utils::WString	sSection;
	Utils::WString sData;
} SDummy;

typedef struct SComponent
{
	Utils::WString sSection;
	Utils::WString sSection2;
	Utils::WString sData;
} SComponent;

typedef struct SCockpit
{
	Utils::WString	   sCockpit;
	CLinkList<SWeaponMask> lWeaponMask;
	int				iIndex;
} SCockpit;

using namespace SPK;

class SPKEXPORT CXspFile : public CBaseFile
{
public:
	CXspFile ();
	~CXspFile () { this->Delete(); }

	static bool ReadAnimations(const Utils::WStringList& lIn, Utils::WStringList& lOut, int startRecord);
	static int GetAnimationType(const Utils::WString &sType);
	static Utils::WString TypesListToString(Utils::CList<STypesSection> &list, bool deleteAfter = false);

	bool writeHeader(CFileIO &file, int iHeader, int iLength) const override;

	bool IsLanguageText () { return m_bLanguageText; }
	bool IsExistingShip () { return m_bExistingShip; }

	int  GetOriginalDescription () { return m_iOrgDesc; }
	unsigned long shipyards () const { return _iShipyard; }

	const Utils::WString &shipID () const { return m_sID; }
	const Utils::WString &shipData () const { return m_sData; }
	Utils::WString getX3ShipData() const;
	Utils::WString getTCShipData() const;
	Utils::WString shipFilename() const
	{ 
		Utils::WString s = this->name() + L"-" + this->author();
		s = s.remove(L' ');
		s = s.remove(L'\\');
		s = s.remove(L'/');
		return s;
	}

	SText *FindShipText(int lang);
	Utils::WString textName(int lang);
	Utils::WString textDescription(int lang);

	void SetLanguageText		( bool b )   { m_bLanguageText = b; _changed(); }
	void SetExistingShip		( bool b )   { m_bExistingShip = b; _changed(); }
	void SetOriginalDescription	( int i )	 { m_iOrgDesc = i; _changed();; }
	void setShipID				(const Utils::WString &sId ) { m_sID = sId; _changed();; }
	void setShipData			(const Utils::WString &sData ) { m_sData = sData; _changed(); }

	void SetSceneFile ( C_File *f ) { m_pSceneFile = f; }
	void SetCockpitFile ( C_File *f ) { m_pCockpitFile = f; }

	void addAnimation(const Utils::WStringList &list);
	void addAnimation(const Utils::WString& data);
	void addBodies(const Utils::WString &data);
	void addBody(const Utils::WString &section, const Utils::WString &data);
	void addCutData(const Utils::WString& data);
	void addText(int id, const Utils::WString &name, const Utils::WString &desc);
	void addCockpit(const Utils::WString &cockpit, int game, int mask = -1, int index = -1 );
	void addCockpitWeapon(const Utils::WString &cockpit, int game, int mask);
	void newCockpit(const Utils::WString &id, const Utils::WString &scene, int mask );
	void editCockpit(const Utils::WString &id, const Utils::WString &cockpit);
	void editCockpit(const Utils::WString &id, const Utils::WString &scene, int mask);
	Utils::WString getCockpitData(const Utils::WString &id);
	SCockpit *findCockpit(const Utils::WString &id);
	void addDummy(const Utils::WString &section, const Utils::WString &data );
	void addComponent(const Utils::WString &section, const Utils::WString &section2, const Utils::WString &data);
	void AddWeaponMask(int game, int mask );
	void AddMissileMask(int game, int mask );
	void RemoveText(int id);
	void ClearText() { m_lText.MemoryClear(); _changed(); }

	void addShipyard(ShipyardRace s) { _iShipyard |= static_cast<unsigned long>(s); _changed(); }
	void removeShipyard(ShipyardRace s) { _iShipyard &= ~(static_cast<unsigned long>(s)); _changed(); }
	void setShipyard(ShipyardRace s, bool b) { if ( b ) addShipyard(s); else removeShipyard(s); }
	void toggleShipyard(ShipyardRace s) { if ( this->isShipyard(s) ) removeShipyard(s); else addShipyard(s); }

	bool removeCutData(const Utils::WString &cut);
	bool removeCockpit(const Utils::WString &cockpitid);
	bool removeComponent(const Utils::WString &section1, const Utils::WString &section2, const Utils::WString &data);
	bool removeDummy(const Utils::WString &section, const Utils::WString &data);
	bool removeBodies(const Utils::WString &body);
	bool removeAnimation(const Utils::WString &ani);

	bool IsSigned() { return false; }
	bool IsValid ();
	bool isShipyard(ShipyardRace s) const { return (_iShipyard & static_cast<unsigned long>(s)) ? true : false; }
	bool anyShipyards() const { return (_iShipyard > 0) ? true : false; }
	void AdjustCockpits();
	virtual Utils::WString createValuesLine() const override;
	virtual bool parseValueLine(const Utils::WString &line) override;

	virtual int GetType () { return TYPE_XSP; }
	virtual BaseFileType type() const override { return BaseFileType::TYPE_XSP; }

	Utils::WString shipName(int lang);
	bool convertOld(const Utils::WString &file);

	const Utils::WStringList &getAnimations() const { return _lAnimations; }
	const Utils::WStringList &getCutData() const { return _lCutData; }
	const Utils::WStringList &getBodies() const { return _lBodies; }
	CLinkList<SWeaponMask> *GetLaserMasks() { return &m_lWeaponMasks; }
	CLinkList<SWeaponMask> *GetMissileMasks() { return &m_lMissileMasks; }
	CLinkList<SText> *GetTexts() { return &m_lText; }
	CLinkList<SDummy> *GetDummies() { return &m_lDummy; }
	CLinkList<SComponent> *GetComponents() { return &m_lComponent; }
	CLinkList<SCockpit> *GetCockpits() { return &m_lCockpit; }
	int GetLaserMask(int game, bool getOnly = false);
	int GetMissileMask(int game, bool getOnly = false);
	void SetLaserMask(int game, int mask);
	void SetMissileMask(int game, int mask);

	void setCreatedShipData(CShipData *data) { m_sData = data->createData(); }
	bool AnyTexts() { return !m_lText.empty(); }
	bool AnyDummies() { return !m_lDummy.empty(); }
	bool AnyComponents() { return !m_lComponent.empty(); }
	bool AnyCockpits() { return !m_lCockpit.empty(); }
	bool anyCutData() { return !_lCutData.empty(); }
	bool anyBodies() const { return !_lBodies.empty(); }
	bool anyAnimations() const { return !_lAnimations.empty(); }

	void clearCutData();
	void clearAnimations();
	void clearBodies();

	Utils::WString shipClass() const;
	virtual bool computeSigned(bool updateFiles) const override { return false; }

	// ship extraction
	bool startExtractShip(CVirtualFileSystem *pVfs, const Utils::WString &sId, CProgressInfo *pProgress);
	bool extractShip(CVirtualFileSystem *pVfs, const Utils::WString &sId, CProgressInfo *progress);
	bool extractSceneFiles(CVirtualFileSystem *pVfs);
	void extractComponants(CVirtualFileSystem *pVfs, const Utils::WStringList &sceneModels);
	void extractCutData(CVirtualFileSystem *pVfs, Utils::WStringList& sceneModels, bool add);
	void extractDummies(CVirtualFileSystem *pVfs, Utils::WStringList& sceneModels, bool add);
	void extractTextures(CVirtualFileSystem *pVfs);
	bool extractBodies(CVirtualFileSystem *pVfs, const Utils::WStringList& sceneModels);
	bool extractCockpits(CVirtualFileSystem *pVfs);

	void addComponentsToList(CLinkList<SComponentEntry> &componentList);
	void addDummiesToList(Utils::CList<STypesSection> &list);
	void addCutDataToList(Utils::CList<STypesSection> &list);
	void addBodiesToList(Utils::CList<STypesSection> &list);
	void addAnimationsToList(Utils::CList<STypesSection> &list);

	bool processSceneFileSection(int section, CVirtualFileSystem *pVfs, Utils::WStringList &lModels, CProgressInfo *progress);
	bool processSceneFiles(CVirtualFileSystem *pVfs, CProgressInfo *progress);

	void PackAllFiles();
	bool readSceneModels(Utils::WStringList& out);

	void completeFile() override;

	//TODO: convert this
	void ExtractTexts(CCatFile *catFile, CCatFile *secondCatFile, int textId);

	bool getTextureList(Utils::WStringList &list, const unsigned char *olddata, size_t size) const;
	bool addTextFromFile(const Utils::WString &file, int textId = -1);
	bool importBodies(const Utils::WStringList &sceneModels, const Utils::WString &filename);
	bool ImportCockpits(const Utils::WString &filename);
	Utils::WString formatShipData(const Utils::WStringList &cockpits, int *text, int game);

	virtual bool loadPackageData(const Utils::WString &sFirst, const Utils::WString &sRest, const Utils::WString &sMainGame, Utils::WStringList &otherGames, Utils::WStringList &gameAddons, CProgressInfo *progress = NULL) override;
	virtual bool GeneratePackagerScript(bool wildcard, Utils::WStringList *list, int game, const Utils::WStringList &gameAddons, bool datafile = false) override;
	virtual void addGeneratedFiles(HZIP &hz);

protected:
	virtual void Delete ();
	virtual void SetDefaults ();
	virtual bool _checkHeader(const Utils::WString &header) const override;
	SCockpit *_findCockpit(const Utils::WString &sID);
	void _addSection(Utils::CList<STypesSection> &list, const Utils::WString &section, const Utils::WString &data);
	void _addDataSection(Utils::WStringList& list, Utils::CList<STypesSection>& sectionList, bool bUseFirst);
	bool _addTextFromFile(CFileIO &F, int textId = -1);

protected:
	C_File *m_pSceneFile, *m_pCockpitFile;

	bool m_bLanguageText, m_bExistingShip;

	int m_iOrgDesc;

	Utils::WString m_sID;
	Utils::WString m_sData;

	Utils::WStringList		_lCutData;
	Utils::WStringList		_lBodies;
	Utils::WStringList		_lAnimations;
	CLinkList<SCockpit>		m_lCockpit;
	CLinkList<SText>		m_lText;
	CLinkList<SComponent>	m_lComponent;
	CLinkList<SDummy>		m_lDummy;
	CLinkList<SWeaponMask>  m_lWeaponMasks;
	CLinkList<SWeaponMask>  m_lMissileMasks;

	unsigned long _iShipyard;
};

#endif //_XSPFILE_H__

