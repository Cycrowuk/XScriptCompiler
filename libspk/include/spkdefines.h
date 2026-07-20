#pragma once

#include "spkdll.h"
#include "lists.h"
#include "Utils/WStringList.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Class declarations

class CXspFile;
class CBaseFile;

namespace SPK {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enumerations

enum {INSTALLERR_NONE, INSTALLERR_VERSION, INSTALLERR_INVALID, INSTALLERR_NOMULTI, INSTALLERR_NOSHIP, INSTALLERR_UNKNOWN, INSTALLERR_NOEXIST, INSTALLERR_OLD};
enum {INSTALLCHECK_OK, INSTALLCHECK_OLDVERSION, INSTALLCHECK_NOOTHERMOD, INSTALLCHECK_ALREADYQUEUED, INSTALLCHECK_WRONGGAME, INSTALLCHECK_WRONGVERSION, INSTALLCHECK_MODIFIED, INSTALLCHECK_NOSHIP};
enum {PROGRESS_ENABLEFILE, PROGRESS_SHUFFLEFAKE, PROGRESS_DISABLEFILE};
enum {PKERR_NONE, PKERR_NOPARENT, PKERR_MODIFIED, PKERR_MISSINGDEP, PKERR_NOOUTPUT, PKERR_DONTEXIST, PKERR_UNABLETOOPEN};
enum {WARETYPE_NONE, WARETYPE_DELETED, WARETYPE_ADDED, WARETYPE_DISABLED};
enum {WARES_BIO, WARES_ENERGY, WARES_FOOD, WARES_MINERAL, WARES_TECH, WARES_NATURAL, WAREBUFFERS};
enum WareTypes {
	Ware_BuiltIn,
	Ware_EMP,
	Ware_Custom
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defines

#define IC_WRONGGAME		1
#define IC_WRONGVERSION		2
#define IC_OLDVERSION		4
#define IC_MODIFIED			8
#define IC_ALL				15

#define PMTEXTFILE 901

#define SHIPSTARTTEXT		500000
#define WARETEXTSTART		400000

#define SafeDelete(a) if (a) delete a; a = NULL;
#define tstruct typedef struct SPKEXPORT
#define tclass class SPKEXPORT 

// text pages
#define TEXTPAGE_RACE		1266
#define TEXTPAGE_OBJECTS	17
#define TEXTPAGE_CLASS		2006
#define TEXTPAGE_CARGOCLASS	1999

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structures

tstruct SWareEntry {
	Utils::WString name;
	Utils::WString description;
	enum WareTypes type;
	Utils::WString id;
	int			  relval;
	int			  notority;
	int			  position;
	CBaseFile	 *package;
} SWareEntry;

tstruct SCommandSlot {
	Utils::WString name;
	Utils::WString id;
	Utils::WString info;
	Utils::WString shortName;
	int			  slot;
	CBaseFile    *package;
} SCommandSlot;


tstruct SWaresText {
	int		iLang;
	Utils::WString  sName;
	Utils::WString  sDesc;
} SWaresText;

tstruct SWares {
	Utils::WString  sID;
	wchar_t    cType;
	long	iPrice;
	int		iSize;
	int		iVolumn;
	int		iNotority;
	bool	bEnabled;
	int		iPosID;
	int		iDescID;
	int		iTextID;
	int		iTextPage;
	CLinkList<SWaresText> lText;
	int		iUsed;
} SWares;

tstruct SSettingType {
	Utils::WString	sKey;
	int		iType;
} SSettingType;

tstruct SGameWare {
	wchar_t cType;
	int iType;
	int iText;
	Utils::WString sWareName;
	int iPos;
	SWares *pWare;
} SGameWare;

tstruct SGameShip {
	int		  iType;
	int		  iText;
	int		  iPos;
	Utils::WString  sShipID;
	Utils::WString  sShipClass;
	CXspFile *pPackage;
} SGameShip;

tstruct SBodies {
	Utils::WString	 sNumbers;
	Utils::WString	 sSection;
	Utils::WStringList lEntries;
} SBodes;

tstruct SNeededLibrary {
	Utils::WString	sName;
	Utils::WString	sAuthor;
	Utils::WString	sMinVersion;
} SNeededLibrary;

tstruct SGameCompat {
	int			iGame;
	Utils::WString	sVersion;
	int			iVersion;
} SGameCompat;

typedef struct SNames {
	int iLanguage;
	Utils::WString sName;
} SNames;

tstruct SAvailablePackage {
	CLinkList<SGameCompat> lGames;
	int			iType;
	int			iPluginType;
	Utils::WString	sName;
	Utils::WString	sAuthor;
	Utils::WString	sVersion;
	Utils::WString	sDesc;
	Utils::WString	sUpdated;
	int			iEase;
	int			iChanging;
	int			iRec;
	Utils::WString	sFilename;
	int			iScriptType;
	bool		bSigned;
} SAvailablePackage;

tstruct SWarePriceOverride {
	enum WareTypes type;
	int			   pos;
	Utils::WString  id;
	int			   relval;
	int			   notority;
	bool		   bNotority;
} SWarePriceOverride;


}