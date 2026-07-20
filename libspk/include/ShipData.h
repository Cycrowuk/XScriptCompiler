#pragma once

#include "lists.h"
#include "Utils/WString.h"

/**
 * Ship class defines
 */
typedef enum {
	SG_SH_TL,	
	SG_SH_TS,
	SG_SH_M0,
	SG_SH_M1,
	SG_SH_M2,
	SG_SH_M3,
	SG_SH_M4,
	SG_SH_M5,
	SG_SH_TL_P,
	SG_SH_TS_P,
	SG_SH_GO,
	SG_SH_M6,
	SG_SH_TP,
	SG_SH_M7, 
	SG_SH_TM, 
	SG_SH_M8, 
	SG_SH_MAX
} ShipClasses;

/**
 * Ship Object Classes
 */
typedef enum {
	OBJ_SHIP_M0			= 2020,
	OBJ_SHIP_M1			= 2021,
	OBJ_SHIP_M2			= 2022,
	OBJ_SHIP_M3			= 2023,
	OBJ_SHIP_M4			= 2024,
	OBJ_SHIP_M5			= 2025,
	OBJ_SHIP_M6			= 2026,
	OBJ_SHIP_M7			= 2027,
	OBJ_SHIP_TP			= 2030,
	OBJ_SHIP_TS			= 2031,
	OBJ_SHIP_TL			= 2032,
	OBJ_SHIP_M8			= 2142,
	OBJ_SHIP_TM			= 2141,
	OBJ_SHIP_TS_PIRATE	= 2035,
	OBJ_SHIP_TL_PIRATE	= 2036,
	OBJ_SHIP_GONER		= 2039,
	OBJ_SHIP_MAX		= 16
} ShipObjectClasses;

/**
 * Missle flags
 */
typedef enum {
	SG_MISSILE_LIGHT			= 1,
	SG_MISSILE_MEDIUM			= 2,
	SG_MISSILE_HEAVY			= 4,
	SG_MISSILE_TR_LIGHT			= 8,
	SG_MISSILE_TR_MEDIUM		= 16,
	SG_MISSILE_TR_HEAVY			= 32,
	SG_MISSILE_KHAAK			= 64,
	SG_MISSILE_BOMBER			= 128,
	SG_MISSILE_TORP_CAPTIAL		= 256,
	SG_MISSILE_AF_CAPTIAL		= 512,
	SG_MISSILE_TR_BOMBER		= 1024,
	SG_MISSILE_TR_TORP_CAPTIAL	= 2048,
	SG_MISSILE_TR_AF_CAPTIAL	= 4096,
	SG_MISSILE_BOARDINGPOD		= 8192,
	SG_MISSILE_DMBF				= 16384,
	SG_MISSILE_DISRUPTOR		= 32768,
	SG_MISSILE_MAX				= 16
} MissileFlags;

/**
 * structure for ships individual guns
 */
typedef struct SWeaponGroup {
	int				iGunIndex;
	int				iLaser;
	Utils::WString	sModel1;
	int				iNode1;
	Utils::WString	sModel2;
	int				iNode2;
} SWeaponGroup;

/**
 * structure for each gun placement on the ships
 * Can hold multiple weapons
 */
typedef struct SGunGroup {
	int			iGunIndex;
	int			iLaserCount;
	int			iIndex;
	int			iWeaponCount;
	CLinkList<SWeaponGroup> lWeapons;
} SGunGroup;

/**
 * structure for each turret cockpit on the ship
 */
typedef struct STurretEntry
{
	int				iIndex;
	int				iTurret;
	Utils::WString	sModel;
	int				iSceneNode;
} STurretEntry;

typedef struct STurretData {
	int				iPos;
	int				iCockpit;
	Utils::WString	sCockpit;
} STurretData;

/**
 * Class to split the TShips data into individual values
 */
class SPKEXPORT CShipData
{
public:
	CShipData(void);
	CShipData(const Utils::WString &a_data)
	{
		this->readShipData(a_data);
	}
	virtual ~CShipData(void);

	Utils::WString createData() const;
	void CreateDefault();
	void ClearLists();
	bool readShipData(const Utils::WString &a_data);
	void UpdateGunEntries();
	void AddNewTurret(int dir);
	
	void ClearTurrets();
	void RemoveTurret(int t);

	static Utils::WString ConvertShipSubType(int subtype);
	static Utils::WString ConvertShipClass(int shipclass);
	static int			GetShipClassFromNum(int shipclass);
	static int			ConvertShipSubType(const Utils::WString &subtype);
	static int			ConvertShipClassToNum(int shipclass);
	static int			ConvertShipClass(const Utils::WString &shipclass);
	static int			ConvertMissileGroup(const Utils::WString &type);
	static int			ConvertMissileGroupToID(const Utils::WString &type);
	static int			ConvertMissileGroupToID(int type);
	static Utils::WString	ConvertMissileGroup(int type);

	Utils::WString	sModel;
	int				iPictureID;
	float			fRotX, fRotY, fRotZ;
	int				iSubType;
	int				iDesc;
	int				iRace;
	int				iVariation;
	int				iClass;
	int				iDocking;
	int				iSpeed;
	int				iAccel;
	int				iEngineSound;
	int				iReactionDelay;
	int				iEngineEffect;
	int				iEngineGlow;
	int				iPower;
	int				iSoundMin;
	int				iSoundMax;
	Utils::WString	sModelScene;
	Utils::WString	sCockpitScene;
	int				iLaserMask;
	int				iGunCount;
	int				iLaserEnergy;
	float			fLaserRecharge;
	int				iShieldType;
	int				iMaxShields;
	int				iMissileMask;
	int				iMissileCount;
	int				iSpeedExtension;
	int				iSteeringExtension;
	int				iCargoMin;
	int				iCargoMax;
	int				iWareListID;
	STurretData		cockpits[6];
	int				iCargoClass;
	int				iHull;
	int				iExplosionDef;
	int				iExplosionBody;
	int				iParticle;
	int				iRotationAccel;
	int				iTurretCount;
	CLinkList<STurretEntry> lTurrets;
	int				iGunsCount;
	CLinkList<SGunGroup> lGuns;
	int				iVolumn;
	int				iRelVal;
	int				iPriceMod1;
	int				iPriceMod2;
	int				iSize;
	int				iRelValPlayer;
	int				iMinNoto;
	int				iVideoID;
	int				iSkin;
	Utils::WString	sID;

private:
	void _capValues();
};
