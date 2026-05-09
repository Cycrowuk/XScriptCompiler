#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <string>
#include <vector>
#include <unordered_set>
#include <map>

#define _SCRIPT_VARIDX_SHIFT_JUMPIDX 8
#define _SCRIPT_VARIDX_MASK_JUMPIDX  0x00ffff00
#define _SCRIPT_VARIDX_PREFIX        0x80000000
#define _SCRIPT_VARIDX_FLAG_NOT      0x40000000
#define _SCRIPT_VARIDX_FLAG_ISIF     0x20000000
#define _SCRIPT_VARIDX_MASK_OP       0x000000ff

#define SCRIPT_VARIDX_NORETVAR  (_SCRIPT_VARIDX_PREFIX|1)
#define SCRIPT_VARIDX_START	    (_SCRIPT_VARIDX_PREFIX|2)
#define SCRIPT_VARIDX_IF        (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|3)
#define SCRIPT_VARIDX_IFNOT     (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|_SCRIPT_VARIDX_FLAG_NOT|4)
#define SCRIPT_VARIDX_ELSEIF    (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|5)
#define SCRIPT_VARIDX_ELSEIFNOT (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|_SCRIPT_VARIDX_FLAG_NOT|6)
#define SCRIPT_VARIDX_SKIPIF    (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|_SCRIPT_VARIDX_FLAG_NOT|7)
#define SCRIPT_VARIDX_SKIPIFNOT (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|8)
#define SCRIPT_VARIDX_WHILE     (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|9)
#define SCRIPT_VARIDX_WHILENOT  (_SCRIPT_VARIDX_PREFIX|_SCRIPT_VARIDX_FLAG_ISIF|_SCRIPT_VARIDX_FLAG_NOT|10)

enum class ParDef 
{
	Unknown				= -1,
	Var					= 0,
	Label				= 1,
	String				= 2,
	Int					= 3,	
	Op					= 4,
	Args				= 5,
	LabelDef			= 6,
	CallName			= 7,
	Comment				= 8,
	Value				= 9,
	Number				= 10,
	VarString			= 11,
	Station				= 12,
	Ware				= 13,
	Sector				= 14,
	RetVar				= 15,
	Race				= 16,
	StationType			= 17,
	StationSerial		= 18,
	ShipType			= 19,
	RefObj				= 20,
	Ship				= 21,
	ShipStation			= 22,
	ObjClass			= 23,
	TC					= 24,
	RetVarNoIf			= 25,
	RetVarIf			= 26,
	ShipStationType		= 27,
	RetvarStart			= 28,
	Relation			= 29,
	Expression			= 30,
	ObjCmd				= 31,
	ObjSignal			= 32,
	ObjCmdSignal		= 33,
	FlightReturn		= 34,
	PlayerShips			= 35,
	PlayerStation		= 36,
	PlayerHomebase		= 37,
	PlayerShipStation	= 38,
	WareGate			= 39,
	HomeBase			= 40,
	DockAt				= 41,
	AllWares			= 42,
	Environment			= 43,
	DataType			= 44,
	Array				= 45,
	StationResource		= 46,
	StationProduct		= 47,
	StationWare			= 48,
	HomebaseResource	= 49,
	HomebaseProduct		= 50,
	HomebaseWare		= 51,
	SectorPosition		= 52,
	Constant			= 53,
	Asteroid			= 54,
	FlyingWare			= 55,
	JumpToGate			= 56,
	ShipWare			= 57,
	WareOfShip			= 58,
	Quest				= 59,
	Custom				= 60,
	Wing				= 61,
	ScriptDef			= 62,
	VarBoolean			= 63,
	Boolean				= 64,
	WingCommand			= 65,
	Passenger			= 66,
	ShipPassenger		= 67,
	PassengerOfShip		= 68,
	SectorJumpdrive		= 69,	
	Fleet				= 70,
	GlobalValue			= 71,
	AgentCommand		= 72,
	Agent				= 73,
	Table				= 74,
	ArrayTable			= 75,
	LoadoutRet			= 76,
	Merchant			= 77,
	Dealer				= 78,
	MerchantDealer		= 79,
	CarrierRole			= 80,
	CommandMenu			= 81,
	GSubType			= 82,
	MainType			= 83,
	AvailableWare		= 84,
	SpaceObject			= 85,
	PlayerStationType	= 86,
	PropertyGroup		= 87,
	TurretCommand		= 88,
	AllObjCmds			= 89,
	HSAP				= 90,
	Time				= 91,
	RetiredAgent		= 92,
	Laser				= 93,
	Shield				= 94,
	Missile				= 95,
	Goods				= 96,
	Equipment			= 97,
	XMLData				= 98,
	ShipCommandName		= 99,
	CustomWing			= 100,
};

#define _SCRIPT_DATATYPE_FLAG_OBJ		0x00010000
#define _SCRIPT_DATATYPE_FLAG_DECODE	0x00020000

enum class DataTypes
{
	Invalid				= -1, //NOTE: not a valid datatype for X3 (Used internally)
	Null				= 0,
	Unknown				= 1,
	Variable			= (_SCRIPT_DATATYPE_FLAG_DECODE | 2),
	Constant			= (_SCRIPT_DATATYPE_FLAG_DECODE | 3),
	Number				= 4,
	String				= 5,
	Ship				= (_SCRIPT_DATATYPE_FLAG_OBJ | 6),
	Station				= (_SCRIPT_DATATYPE_FLAG_OBJ | 7),
	Sector				= (_SCRIPT_DATATYPE_FLAG_OBJ | 8),
	Ware				= 9,
	Race				= 10,
	StationSerial		= 11,
	ObjectClass			= 12,
	TransportClass		= 13,
	Relation			= 14,
	Operator			= 15,
	Expression			= 16,
	Object				= (_SCRIPT_DATATYPE_FLAG_OBJ | 17),
	ObjectCommand		= 18,
	FlightReturn		= 19,
	DataType			= 20,
	Array				= 21,
	Quest				= 22,
	Wing				= (_SCRIPT_DATATYPE_FLAG_OBJ | 23),
	ParDef				= 24,
	WingCommand			= 25,
	Passenger			= (_SCRIPT_DATATYPE_FLAG_OBJ | 26),
	Agent				= (_SCRIPT_DATATYPE_FLAG_OBJ | 27),
	AgentCommand		= 28,	
	Table				= 29,
	LoadoutReturn		= 30,
	Merchant			= 31,
	Dealer				= 32,
	CarrierRole			= 33,
	CommandMenu			= 34,
	GalaxySubType		= 35,
	MainType			= 36,
	Time				= 37,
	XMLData				= 38,
	Custom				= 2000,
};

struct ScriptArguments 
{
	std::wstring variable;
	std::wstring description;
	ParDef parDef;
	std::wstring parDefName;
};

enum class ParDefFlags
{
	None		= 0,
	Constant	= 1,
	Variable	= 2,
	String		= 4,
	Integer		= 8,
	Object		= 16,
};

inline ParDefFlags operator|(ParDefFlags a, ParDefFlags b)
{
	return static_cast<ParDefFlags>(static_cast<int>(a) | static_cast<int>(b));
}
inline ParDefFlags operator&(ParDefFlags a, ParDefFlags b)
{
	return static_cast<ParDefFlags>(static_cast<int>(a) & static_cast<int>(b));
}
struct ParDefData
{
	ParDef id;
	std::wstring code;
	std::wstring name;
	std::wstring desc;
	std::unordered_set<DataTypes> datatypes;
	ParDefFlags flags;
};

struct DataTypeData
{
	DataTypes id;
	std::wstring code;
	bool isObject;
	std::wstring name;
	std::wstring desc;
};

struct GameVersion 
{
	unsigned int engine;
};

enum class InternalFunctions
{
	Unknown		= -1,

	SetArguments,
	SetDescription,
	SetVersion,
	SetCommand,

	// always at the bottom
	Max,
};

enum class RetVarType
{
	None,
	Return,
	NoIf,
	If,
	NoIfStart,
};

struct ConstGroup
{
	std::wstring name;
	std::wstring desc;
	bool ns;
};


struct FunctionArgument
{
	ParDef pardef;
	std::wstring description;
	ConstGroup *constGroup;
};
struct Function
{
	struct Function() : id(0), returnArgument(0), returnValueType(RetVarType::None), allowNull(false), undefinedCount(false) {}
	std::wstring name;
	std::wstring description;
	std::wstring example;
	unsigned int id;
	std::vector<FunctionArgument> arguments;
	std::unordered_set<DataTypes> returnValue;
	RetVarType returnValueType;
	std::vector<std::wstring> order;
	unsigned int returnArgument;
	std::unordered_set<DataTypes> refObjType;
	bool allowNull;
	unsigned int undefinedCount;
};


enum class ArgumentType
{
	Normal,
	Variable,
	Constant,
	Lookup,
	Internal,
	Unknown,
};
struct Argument
{
	DataTypes dataType;
	std::wstring data;
	ArgumentType type;

	// additional data
	int		idata;
};

struct ObjectCommand
{
	std::wstring id;
	std::wstring name;
	std::wstring description;
	std::wstring shortName;
	unsigned int num;
};

struct Commands
{
	std::wstring name;
	DataTypes dt;
	std::wstring desc;
	std::map<unsigned int, ObjectCommand> data;
	std::map<std::wstring, unsigned int> list;
};

struct WareTypeData
{
	std::wstring id;
	std::wstring name;
	std::wstring description;
	unsigned int type;
	unsigned int main;
	unsigned int sub;
};

enum class SpecialKeywords
{
	Continue,
	Break,
	Goto,
	GoSub,
};