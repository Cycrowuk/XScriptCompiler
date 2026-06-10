#pragma once

#define MAINTYPE(typecode) (((typecode)>>16) & 0xFF)					/**< Get the maintype from a type code. */
#define SUBTYPE(typecode) ((typecode) & 0xFFFF)							/**< Get the subtype of a type code. */
#define TYPECODE(maintype,subtype) (((maintype)<<16) | (subtype))		/**< Combine \a maintype and \a subtype in one integer. */

#define DATAVERSION 2

// Forward declarations
namespace XScript { class ParseArguments; }

namespace XScript
{
	enum class SpecialFunction
	{
		GetArray,
		GetArrayDouble,
		SetArray,
		SetArrayDouble,
		SetArrayFromArray,
	};

	/** A single line in a macro's expansion routine */
	struct MacroRoutineLine
	{
		enum class Type { Expression, StartBlock, EndBlock, BlockCommands };
		Type         type;
		std::wstring text; // for Expression — template with %ARG0%, %ARG1%, $0, $1 etc.

		// Optional function calls whose return values substitute $0, $1 etc.
		struct FuncArg
		{
			unsigned int funcId;   // function to call
			int          argPos;   // which passed macro argument to pass (-1 = none)
		};
		std::vector<FuncArg> funcArgs;
	};

	/** A function macro defined in x3fl.xml — expands to XScript source at compile time */
	struct MacroData
	{
		std::wstring                  name;
		std::vector<std::wstring>     argNames; // positional argument names
		bool                          hasBlock; // true if routine contains <BlockCommands/>
		std::vector<MacroRoutineLine> routine;
	};

	struct GameData
	{
		std::wstring id;
		std::wstring name;
		std::wstring dir;
		unsigned int language;
		std::vector<unsigned int> textPrefixes;
		unsigned int engineMin;
		unsigned int engineMax;
	};

	struct RaceData
	{
		unsigned int id;
		std::wstring code;
		std::wstring name;
		std::wstring desc;
	};

	struct CustomDataEntry
	{
		std::wstring code;
		std::wstring desc;
		std::wstring strID;
		unsigned int intID = 0;
	};

	struct CustomData
	{
		std::wstring name;
		std::wstring desc;
		bool isStringData = false;
		DataTypes datatype = DataTypes::Unknown;
		std::map<std::wstring, CustomDataEntry> entries;
		std::vector<std::wstring> strKeys;
		std::vector<unsigned int> intKeys;
		std::map<std::wstring, unsigned int> keyMap;
		std::map<std::wstring, std::wstring> strLookup;
		std::map<unsigned int, std::wstring> intLookup;
	};
	class ConstantData
	{
	private:
		DataTypes _type;
		DataTypes _subtype;
		std::wstring _name;
		std::wstring _strData;
		unsigned int _id;
		ConstGroup* _pGroup;

	public:
		ConstantData();
		ConstantData(DataTypes type, const std::wstring& name, unsigned int id, ConstGroup* group, DataTypes subtype = DataTypes::Unknown);
		ConstantData(DataTypes type, const std::wstring& name, unsigned int id, DataTypes subtype = DataTypes::Unknown);

		void setGroup(ConstGroup* group);
		void setStrData(const std::wstring& data);

		const std::wstring& strData() const;
		ConstGroup* group() const;

		DataTypes subtype() const;
		DataTypes type() const;
		unsigned int id() const;
	};

	class Properties
	{
	private:
		unsigned int _getter;
		unsigned int _setter;
		std::wstring _description;

	public:
		Properties();
		Properties(unsigned int getter, unsigned int setter, const std::wstring &desc);
		unsigned int getter() const;
		unsigned int setter() const;
		const std::wstring& description() const;
	};

	class CScriptData
	{
		friend class ScriptDataReader;

	private:
		struct DataFileHeaderRaw
		{
			short version;
			short count;
			char header[14];
		};
		struct DataFileHeader
		{
			bool success;
			std::string header;
			unsigned int version;
			unsigned int count;
		};
		struct GameDataFileData
		{
			unsigned short engineMin;
			unsigned short engineMax;
			unsigned short texts;
			unsigned short language;
			unsigned short idSize;
			unsigned short nameSize;
			unsigned short dirSize;
		};
		struct DataTypeFileData
		{
			unsigned long id;
			unsigned char isObject;
			unsigned short idSize;
			unsigned short nameSize;
			unsigned short descSize;
			unsigned short prefixSize;
		};
		struct ParDefFileData
		{
			unsigned long id;
			unsigned long flags;
			unsigned short idSize;
			unsigned short nameSize;
			unsigned short descSize;
			unsigned short datatypes;
		};
		struct WareTypeFileData
		{
			unsigned long id;
			unsigned short idSize;
			unsigned short nameSize;
			unsigned short descSize;
		};
		struct ConstantFileData
		{
			unsigned long id;
			unsigned long subtype;
			unsigned short idSize;
			unsigned short groupSize;
		};
		struct ConstantNamespaceFileData
		{
			unsigned short idSize;
			unsigned short entries;
		};
		struct ConstGroupFileData
		{
			unsigned short idSize;
			unsigned short descSize;
		};
		struct CommandFileData
		{
			unsigned long datatype;
			unsigned short entries;
			unsigned short nameSize;
			unsigned short descSize;
		};
		struct ObjectCommandFileData
		{
			unsigned long id;
			unsigned short idSize;
			unsigned short nameSize;
			unsigned short descSize;
			unsigned short shortSize;
		};
		struct FunctionFileData
		{
			unsigned long id;
			unsigned short idSize;
			unsigned short descSize;
			unsigned short argCount;
			unsigned char allowNull;
			unsigned short refObj;
			unsigned short orderCount;
			unsigned short exampleSize;
			unsigned short returnArg;
			unsigned short returnType;
			unsigned short returnCount;
			unsigned char undefinedArgs;
		};
		struct FunctionArgFileData
		{
			unsigned long id;
			unsigned short descSize;
			unsigned short groupSize;
		};
		struct SpecialFuncFileData
		{
			unsigned short id;
			unsigned short funcId;
		};
		struct SpecialKeyFileData
		{
			unsigned short id;
			unsigned short idSize;
		};
		struct ObjectTypeFileData
		{
			unsigned short dt;
			unsigned short size;
		};
		struct CustomFileData
		{
			unsigned short entries;
			unsigned long datatype;
			unsigned short nameSize;
			unsigned short descSize;
			unsigned short isString;
		};

		struct CustomEntryFileData
		{
			unsigned short codeSize;
			unsigned short descSize;
			unsigned short idSize;
			unsigned short id;
		};
		struct RaceFileData
		{
			unsigned short codeSize;
			unsigned short descSize;
			unsigned short nameSize;
			unsigned short id;
		};
		struct PropertiesData
		{
			unsigned short nameSize;
			unsigned short descSize;
			unsigned short getterID;
			unsigned short setterID;
		};

	private:
		std::map<const std::wstring, InternalFunctions> _internalFunctions;
		std::map<const std::wstring, unsigned int> _globalFunctions;
		std::map<const std::wstring, std::vector<unsigned int>> _functionAliases; // alias name → list of overload function IDs
		std::map<const std::wstring, std::map<const std::wstring, unsigned int>> _namespaceFunctions; // namespace → (alias → functionId)
		std::map<const std::wstring, MacroData> _macros; // function macros
		std::map<DataTypes, std::map<const std::wstring, unsigned int>> _objectTypeFunctions;
		std::map<const std::wstring, unsigned int> _objectFunctions;
		std::map<const std::wstring, Properties> _objectProperties;
		std::map<const std::wstring, ConstantData> _constData;
		std::map<const std::wstring, unsigned int> _wareTypes;
		std::map<const std::wstring, unsigned int> _dataTypes;
		std::map<SpecialFunction, unsigned int> _specialFunctions;
		std::map<const std::wstring, unsigned int> _specialKeywords;
		std::map<unsigned int, std::wstring> _constants;
		std::map<const std::wstring, ConstGroup> _constantGroups;
		std::map<const std::wstring, std::map<const std::wstring, ConstantData>> _constantNamespaces;
		std::map<const std::wstring, unsigned int> _races;

		std::vector<std::wstring> _internalFunctionLookup;
		std::vector<Function> _functionData;
		std::map<unsigned int, WareTypeData> _wareTypesData;
		std::map<unsigned int, RaceData> _raceData;
		std::map<DataTypes, Commands> _commands;

		std::map<const std::wstring, ParDef> _pardefs;
		std::vector<ParDefData> _pardefData;
		std::map<unsigned int, DataTypeData> _dataTypesData;

		std::map<DataTypes, CustomData> _customData;

		GameData	_gameData;

		unsigned int	_elseCommand;
		unsigned int	_endCommand;
		unsigned int	_returnCommand;
		unsigned int	_continueCommand;
		unsigned int	_expressionCommand;
		unsigned int	_hiddenGotoCommand;
		unsigned int	_breakCommand;
		unsigned int	_defineLabelCommand;
		unsigned int	_gotoCommand;
		unsigned int	_gosubCommand;

	public:
		CScriptData();
		virtual ~CScriptData();

		const GameData& gameData() const;

		InternalFunctions findInternalFunction(const std::wstring& function) const;
		const Function* findGlobalFunction(const std::wstring& function) const;
		const Function* findBestGlobalFunction(const std::wstring& function, int argCount, const ParseArguments* args = nullptr) const;
		const Function* findNamespaceFunction(const std::wstring& ns, const std::wstring& alias) const;
		/** Returns {namespace, alias} for a function ID if it belongs to a namespace, or {"",""} */
		std::pair<std::wstring, std::wstring> findNamespaceForFunction(unsigned int funcId) const;
		/** Returns the macro with the given name, or nullptr */
		const MacroData* findMacro(const std::wstring& name) const;
		const Function* getSpecialGlobalFunction(SpecialFunction func) const;
		const Function* findObjectFunction(const std::wstring& function) const;
		const Properties* findObjectProperty(const std::wstring& prop) const;
		const Function* findObjectPropertySetter(const std::wstring& prop) const;
		const Function* findObjectPropertyGetter(const std::wstring& prop) const;
		const Function* findObjectTypeFunction(DataTypes type, const std::wstring& function) const;
		const ParDefData* findParDefData(const std::wstring& pardef) const;
		const ConstantData* findConstant(const std::wstring& constant) const;
		const ConstantData* findConstant(const std::wstring& ns, const std::wstring& constant) const;
		const DataTypeData* findDatatypeByPrefix(const std::wstring& name, unsigned int& outId) const;
		unsigned int findSpecialKeyword(const std::wstring& keyword) const;
		const DataTypeData* findDatatype(const std::wstring& datatype) const;
		const DataTypeData* findDatatype(DataTypes datatype) const;
		const Commands* findCommandsList(DataTypes data) const;

		std::wstring getDataTypeName(DataTypes type) const;
		std::wstring getDataTypeCode(DataTypes type) const;
		std::wstring getWareTypeCode(unsigned int waretype) const;
		std::wstring getConstantCode(unsigned int constID) const;
		std::wstring getRaceCode(unsigned int raceID) const;
		const CustomData* getCustomDatatype(DataTypes dt) const;
		const ObjectCommand* getCommand(DataTypes dt, unsigned int cmd) const;
		const ParDefData* getParDefData(ParDef id) const;
		const Function* getFunction(unsigned int id) const;

		const std::map<const std::wstring, unsigned int>& races() const;
		const std::map<DataTypes, Commands>& commands() const;
		const std::map<const std::wstring, ConstantData>& constantData() const;
		const std::vector<Function>& functionData() const;
		const std::map<const std::wstring, unsigned int>& dataTypes() const;

		unsigned int elseCommand() const;
		unsigned int endCommand() const;
		unsigned int returnCommand() const;
		unsigned int continueCommand() const;
		unsigned int breakCommand() const;
		unsigned int expressionCommand() const;
		unsigned int hiddenGotoCommand() const;
		unsigned int defineLabelCommand() const;
		unsigned int gotoCommand() const;
		unsigned int gosubCommand() const;

		bool readXMLData(const std::wstring& filename);
		bool saveData(const std::wstring& filename);
		bool loadData(const std::wstring& filename);
		void resetData();

	private:
		bool _writeHeader(std::ofstream &out, const std::string& name, unsigned int version, unsigned int count);
		DataFileHeader _readHeader(std::ifstream& in, short version);
		bool _writeFunction(std::ofstream& out, const std::wstring& name, unsigned int id);
		bool _readFunction(std::ifstream& in, std::map<const std::wstring, unsigned int> &list);
	};

}

