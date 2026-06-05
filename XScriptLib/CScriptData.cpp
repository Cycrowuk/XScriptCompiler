#include "pch.h"
#include "CScriptData.h"

#include "ScriptDataReader.h"
#include "../XLib/XLib.h"

using namespace XScript;

ConstantData::ConstantData() : _type(DataTypes::Constant), _id(-1), _subtype(DataTypes::Unknown), _pGroup(NULL)
{

}

ConstantData::ConstantData(DataTypes type, const std::wstring& name, unsigned int id, DataTypes subtype) : _type(type), _name(name), _id(id), _subtype(subtype), _pGroup(NULL)
{

}

ConstantData::ConstantData(DataTypes type, const std::wstring& name, unsigned int id, ConstGroup* group, DataTypes subtype) : _type(type), _name(name), _id(id), _subtype(subtype), _pGroup(group)
{

}

void ConstantData::setGroup(ConstGroup* group)
{
	_pGroup = group;
}

void ConstantData::setStrData(const std::wstring& data)
{
	_strData = data;
}

const std::wstring& ConstantData::strData() const
{
	return _strData;
}

ConstGroup* ConstantData::group() const
{
	return _pGroup;
}

DataTypes ConstantData::type() const
{
	return _type;
}

DataTypes ConstantData::subtype() const
{
	return _subtype;
}

unsigned int ConstantData::id() const
{
	return _id;
}


//####///////////////////////////////////////////////////////////////////////////////////////////////////////////////

Properties::Properties() :
	_getter(0),
	_setter(0)
{

}

Properties::Properties(unsigned int getter, unsigned int setter, const std::wstring& desc) :
	_getter(getter),
	_setter(setter),
	_description(desc)
{

}
unsigned int Properties::getter() const
{
	return _getter;
}
unsigned int Properties::setter() const
{
	return _setter;
}

const std::wstring& Properties::description() const
{
	return _description;
}


//####///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CScriptData::CScriptData() : _elseCommand(5),
_endCommand(4),
_returnCommand(103),
_continueCommand(6),
_expressionCommand(104),
_hiddenGotoCommand(112),
_breakCommand(7),
_defineLabelCommand(101),
_gotoCommand(100),
_gosubCommand(1167)
{

	// register all internal functions
	_internalFunctions[L"SetArgument"] = InternalFunctions::SetArguments;
	_internalFunctions[L"SetDescription"] = InternalFunctions::SetDescription;
	_internalFunctions[L"SetVersion"] = InternalFunctions::SetVersion;
	_internalFunctions[L"SetCommand"] = InternalFunctions::SetCommand;

	_internalFunctionLookup.resize(static_cast<unsigned int>(InternalFunctions::Max));
	_internalFunctionLookup[static_cast<unsigned int>(InternalFunctions::SetArguments)] = L"SetArgument";
	_internalFunctionLookup[static_cast<unsigned int>(InternalFunctions::SetDescription)] = L"SetDescription";
	_internalFunctionLookup[static_cast<unsigned int>(InternalFunctions::SetVersion)] = L"SetVersion";
	_internalFunctionLookup[static_cast<unsigned int>(InternalFunctions::SetCommand)] = L"SetCommand";
}

CScriptData::~CScriptData()
{

}

const GameData& CScriptData::gameData() const
{
	return _gameData;
}

const Function* CScriptData::getFunction(unsigned int id) const
{
	if (id >= _functionData.size())
		return NULL;
	return &_functionData[id];
}

InternalFunctions CScriptData::findInternalFunction(const std::wstring& function) const
{
	auto itr = _internalFunctions.find(function);
	if (itr != _internalFunctions.end())
		return itr->second;

	return InternalFunctions::Unknown;
}

const Function* CScriptData::findGlobalFunction(const std::wstring& function) const
{
	auto itr = _globalFunctions.find(function);
	if (itr != _globalFunctions.end())
		return &_functionData[itr->second];

	return NULL;
}

const Function* CScriptData::findBestGlobalFunction(const std::wstring& function, int argCount) const
{
	// If there are aliases (overloads) for this name, pick the best match by argument count
	auto aliasItr = _functionAliases.find(function);
	if (aliasItr != _functionAliases.end())
	{
		const Function* best = nullptr;
		int bestDiff = INT_MAX;

		// Check all alias overloads
		for (unsigned int id : aliasItr->second)
		{
			if (id < _functionData.size())
			{
				const Function* f = &_functionData[id];
				int fArgCount = 0;
				for (const auto& a : f->arguments)
					if (a.pardef != ParDef::RetVar)
						fArgCount++;
				int diff = std::abs(fArgCount - argCount);
				if (diff < bestDiff)
				{
					bestDiff = diff;
					best = f;
				}
			}
		}

		// Also consider the primary function registered under this exact name
		auto primaryItr = _globalFunctions.find(function);
		if (primaryItr != _globalFunctions.end())
		{
			const Function* f = &_functionData[primaryItr->second];
			int fArgCount = 0;
			for (const auto& a : f->arguments)
				if (a.pardef != ParDef::RetVar)
					fArgCount++;
			int diff = std::abs(fArgCount - argCount);
			if (diff < bestDiff)
				best = f;
		}

		if (best)
			return best;
	}

	// No aliases — fall back to exact name lookup
	return findGlobalFunction(function);
}

const Function* CScriptData::getSpecialGlobalFunction(SpecialFunction func) const
{
	auto itr = _specialFunctions.find(func);
	if (itr != _specialFunctions.end())
		return &_functionData[itr->second];

	return NULL;
}

const Function* CScriptData::findObjectFunction(const std::wstring& function) const
{
	auto itr = _objectFunctions.find(function);
	if (itr != _objectFunctions.end())
		return &_functionData[itr->second];

	return NULL;
}

const Properties* CScriptData::findObjectProperty(const std::wstring& prop) const
{
	auto itr = _objectProperties.find(prop);
	if (itr != _objectProperties.end())
		return &itr->second;

	return NULL;
}

const Function* CScriptData::findObjectPropertySetter(const std::wstring& prop) const
{
	auto itr = _objectProperties.find(prop);
	if (itr != _objectProperties.end())
		return &_functionData[itr->second.setter()];

	return NULL;
}

const Function* CScriptData::findObjectPropertyGetter(const std::wstring& prop) const
{
	auto itr = _objectProperties.find(prop);
	if (itr != _objectProperties.end())
		return &_functionData[itr->second.getter()];

	return NULL;
}

const Function* CScriptData::findObjectTypeFunction(DataTypes type, const std::wstring& function) const
{
	auto itr = _objectTypeFunctions.find(type);
	if (itr != _objectTypeFunctions.end())
	{
		auto findItr = itr->second.find(function);
		if (findItr != itr->second.end())
			return &_functionData[findItr->second];
	}

	return NULL;
}

const ParDefData* CScriptData::findParDefData(const std::wstring& pardef) const
{
	auto itr = _pardefs.find(pardef);
	if (itr != _pardefs.end())
		return &_pardefData[static_cast<unsigned int>(itr->second)];

	return NULL;
}

const ConstantData* CScriptData::findConstant(const std::wstring& constant) const
{
	auto itr = _constData.find(constant);
	if (itr != _constData.end())
		return &itr->second;

	return NULL;
}

const ConstantData* CScriptData::findConstant(const std::wstring& ns, const std::wstring& constant) const
{
	auto findItr = _constantNamespaces.find(ns);
	if (findItr != _constantNamespaces.end())
	{
		auto itr = findItr->second.find(constant);
		if (itr != findItr->second.end())
			return &itr->second;
	}
	return NULL;
}

unsigned int CScriptData::findSpecialKeyword(const std::wstring& keyword) const
{
	auto itr = _specialKeywords.find(keyword);
	if (itr != _specialKeywords.end())
		return itr->second;

	return 0;
}

const DataTypeData* CScriptData::findDatatype(const std::wstring& datatype) const
{
	auto itr = _dataTypes.find(datatype);
	if (itr != _dataTypes.end())
		return &_dataTypesData.at(itr->second);

	return NULL;
}
const DataTypeData* CScriptData::findDatatype(DataTypes datatype) const
{
	auto itr = _dataTypesData.find(static_cast<unsigned int>(datatype));
	if (itr != _dataTypesData.end())
		return &(itr->second);

	return NULL;
}

const DataTypeData* CScriptData::findDatatypeByPrefix(const std::wstring& name, unsigned int& outId) const
{
	// Check whether 'name' matches any DataType's prefix pattern.
	// e.g. DataType with prefix="SHIPCOMMAND_" matches "SHIPCOMMAND_1000",
	// setting outId=1000 (the numeric part after the prefix).
	for (const auto& itr : _dataTypesData)
	{
		const DataTypeData& dt = itr.second;
		if (dt.prefix.empty())
			continue;
		if (name.length() <= dt.prefix.length())
			continue;
		if (name.compare(0, dt.prefix.length(), dt.prefix) != 0)
			continue;

		// Prefix matches — the remainder must be a non-empty integer
		const std::wstring remainder = name.substr(dt.prefix.length());
		if (remainder.empty())
			continue;
		bool allDigits = true;
		for (wchar_t c : remainder)
		{
			if (c < L'0' || c > L'9') { allDigits = false; break; }
		}
		if (!allDigits)
			continue;

		try
		{
			outId = static_cast<unsigned int>(std::stoul(remainder));
			return &dt;
		}
		catch (...) { continue; }
	}
	return NULL;
}

const Commands* CScriptData::findCommandsList(DataTypes data) const
{
	auto findItr = _commands.find(data);
	if (findItr != _commands.end())
		return &findItr->second;
	return NULL;
}

std::wstring CScriptData::getDataTypeName(DataTypes type) const
{
	auto data = findDatatype(type);
	if (data)
		return data->name;

	return L"Unknown";
}

std::wstring CScriptData::getDataTypeCode(DataTypes type) const
{
	auto data = findDatatype(type);
	if (data)
		return data->code;

	return L"";
}

std::wstring CScriptData::getWareTypeCode(unsigned int waretype) const
{
	auto findItr = _wareTypesData.find(waretype);
	if (findItr != _wareTypesData.end())
		return findItr->second.id;
	return L"";
}
std::wstring CScriptData::getConstantCode(unsigned int constID) const
{
	auto findItr = _constants.find(constID);
	if (findItr != _constants.end())
		return findItr->second;
	return L"";
}
std::wstring CScriptData::getRaceCode(unsigned int raceID) const
{
	auto findItr = _raceData.find(raceID);
	if (findItr != _raceData.end())
		return findItr->second.code;
	return L"";
}

const CustomData* CScriptData::getCustomDatatype(DataTypes dt) const
{
	auto findItr = _customData.find(dt);
	if (findItr != _customData.end())
		return &findItr->second;
	return NULL;
}

const ObjectCommand* CScriptData::getCommand(DataTypes dt, unsigned int cmd) const
{
	auto dtItr = _commands.find(dt);
	if (dtItr != _commands.end())
	{
		auto findItr = dtItr->second.data.find(cmd);
		if (findItr != dtItr->second.data.end())
			return &findItr->second;
	}

	return NULL;
}

const std::map<const std::wstring, unsigned int>& CScriptData::races() const
{
	return _races;
}

const std::map<DataTypes, Commands>& CScriptData::commands() const
{
	return _commands;
}

const std::map<const std::wstring, ConstantData>& CScriptData::constantData() const
{
	return _constData;
}

const std::vector<Function>& CScriptData::functionData() const
{
	return _functionData;
}

const ParDefData* CScriptData::getParDefData(ParDef id) const
{
	int i = static_cast<int>(id);
	if (i >= 0 && i < _pardefData.size())
		return &_pardefData[i];

	return NULL;
}

const std::map<const std::wstring, unsigned int>& CScriptData::dataTypes() const
{
	return _dataTypes;
}

unsigned int CScriptData::elseCommand() const
{
	return _elseCommand;
}
unsigned int CScriptData::endCommand() const
{
	return _endCommand;
}
unsigned int CScriptData::returnCommand() const
{
	return _returnCommand;
}
unsigned int CScriptData::continueCommand() const
{
	return _continueCommand;
}
unsigned int CScriptData::breakCommand() const
{
	return _breakCommand;
}
unsigned int CScriptData::expressionCommand() const
{
	return _expressionCommand;
}
unsigned int CScriptData::hiddenGotoCommand() const
{
	return _hiddenGotoCommand;
}
unsigned int CScriptData::defineLabelCommand() const
{
	return _defineLabelCommand;
}
unsigned int CScriptData::gotoCommand() const
{
	return _gotoCommand;
}
unsigned int CScriptData::gosubCommand() const
{
	return _gosubCommand;
}

void CScriptData::resetData()
{
	// clear existing data
	_dataTypes.clear();
	_dataTypesData.clear();
	_pardefData.clear();
	_pardefs.clear();
	_constData.clear();
	_functionData.clear();
	_globalFunctions.clear();
	_functionAliases.clear();
	_objectFunctions.clear();
	_objectTypeFunctions.clear();
	_wareTypes.clear();
	_wareTypesData.clear();
	_specialFunctions.clear();
	_specialKeywords.clear();
	_commands.clear();
	_constants.clear();
	_constantGroups.clear();
	_customData.clear();
	_races.clear();
	_raceData.clear();
	_constantNamespaces.clear();
	_objectProperties.clear();

	// clear the game data
	_gameData.language = 0;
	_gameData.dir.clear();
	_gameData.id.clear();
	_gameData.name.clear();
	_gameData.engineMin = 0;
	_gameData.engineMax = 60;
	_gameData.textPrefixes.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CScriptData::readXMLData(const std::wstring& filename)
{
	resetData();

	ScriptDataReader reader(this);
	bool success = reader.readData(filename);

	if (success && _elseCommand)
	{
		_functionData[_elseCommand].order.clear();
		_functionData[_elseCommand].order.push_back(L"RetVar");
	}

	return success;
}

bool WriteWideString(std::ofstream& out, const std::wstring& str)
{
	std::string write;
	std::transform(str.begin(), str.end(), std::back_inserter(write), [](wchar_t c) {
		return (char)c;
		});

	out.write(write.c_str(), write.size());
	if (!out.good())
		return false;

	char c[1];
	c[0] = '\0';
	out.write(c, 1);
	return out.good();
}
std::wstring ReadWideString(std::ifstream& in, short size)
{
	char* data = new char[size + 1];
	in.read(data, size + 1);

	std::wstring read;
	if (in.good())
	{
		std::string str(data);
		std::transform(str.begin(), str.end(), std::back_inserter(read), [](char c) {
			return (wchar_t)c;
			});
	}

	delete[] data;
	return read;
}

bool CScriptData::saveData(const std::wstring& filename)
{
	std::ofstream outfile(filename, std::ios::out | std::ios::binary);
	if (!outfile || !outfile.is_open())
		return false;

	// write the header
	unsigned int dataCount = 17;
	if (!_writeHeader(outfile, "XSCRIPTDATA", DATAVERSION, dataCount))
		return false;

	// write the game data
	{
		if (!_writeHeader(outfile, "GAMEDATA", 1, static_cast<unsigned int>(1)))
			return false;

		GameDataFileData data;
		data.engineMin = _gameData.engineMin;
		data.engineMax = _gameData.engineMax;
		data.language = _gameData.language;
		data.texts = static_cast<unsigned short>(_gameData.textPrefixes.size());
		data.idSize = static_cast<unsigned short>(_gameData.id.size());
		data.dirSize = static_cast<unsigned short>(_gameData.dir.size());
		data.nameSize = static_cast<unsigned short>(_gameData.name.size());

		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;

		for (auto itr = _gameData.textPrefixes.begin(); itr != _gameData.textPrefixes.end(); itr++)
		{
			unsigned short write = static_cast<unsigned short>(*itr);
			outfile.write(reinterpret_cast<char*>(&write), sizeof(write));
			if (outfile.bad())
				return false;
		}

		if (!WriteWideString(outfile, _gameData.id))
			return false;
		if (!WriteWideString(outfile, _gameData.dir))
			return false;
		if (!WriteWideString(outfile, _gameData.name))
			return false;
	}


	// write the data types
	if (!_writeHeader(outfile, "DATATYPE", 1, static_cast<unsigned int>(_dataTypes.size())))
		return false;

	for (auto itr = _dataTypes.begin(); itr != _dataTypes.end(); itr++)
	{
		auto findItr = _dataTypesData.find(itr->second);
		if (findItr != _dataTypesData.end())
		{
			DataTypeFileData data;
			data.id = itr->second;
			data.isObject = findItr->second.isObject;
			data.descSize = static_cast<short>(findItr->second.desc.size());
			data.idSize = static_cast<short>(findItr->second.code.size());
			data.nameSize = static_cast<short>(findItr->second.name.size());
			data.prefixSize = static_cast<unsigned short>(findItr->second.prefix.size());

			outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
			if (outfile.bad())
				return false;

			if (!WriteWideString(outfile, findItr->second.code))
				return false;
			if (!WriteWideString(outfile, findItr->second.name))
				return false;
			if (!WriteWideString(outfile, findItr->second.desc))
				return false;
			if (!WriteWideString(outfile, findItr->second.prefix))
				return false;
		}
		else
			return false;
	}

	// write the pardefs
	if (!_writeHeader(outfile, "PARDEF", 1, static_cast<unsigned int>(_pardefs.size())))
		return false;

	for (auto itr = _pardefs.begin(); itr != _pardefs.end(); itr++)
	{
		ParDefData* d = &_pardefData[static_cast<size_t>(itr->second)];
		ParDefFileData data;
		data.id = static_cast<unsigned long>(d->id);
		data.flags = static_cast<unsigned long>(d->flags);
		data.descSize = static_cast<short>(d->desc.size());
		data.idSize = static_cast<short>(d->code.size());
		data.nameSize = static_cast<short>(d->name.size());
		data.datatypes = static_cast<unsigned short>(d->datatypes.size());

		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;

		if (!WriteWideString(outfile, d->code))
			return false;
		if (!WriteWideString(outfile, d->name))
			return false;
		if (!WriteWideString(outfile, d->desc))
			return false;

		for (auto dItr = d->datatypes.begin(); dItr != d->datatypes.end(); dItr++)
		{
			unsigned long write = static_cast<unsigned long>(*dItr);
			outfile.write(reinterpret_cast<char*>(&write), sizeof(write));
			if (outfile.bad())
				return false;
		}
	}

	if (!_writeHeader(outfile, "WARES", 1, static_cast<unsigned int>(_wareTypes.size())))
		return false;

	for (auto itr = _wareTypes.begin(); itr != _wareTypes.end(); itr++)
	{
		auto findItr = _wareTypesData.find(itr->second);
		if (findItr != _wareTypesData.end())
		{
			WareTypeFileData data;
			data.id = itr->second;
			data.idSize = static_cast<unsigned short>(findItr->second.id.size());
			data.descSize = static_cast<unsigned short>(findItr->second.description.size());
			data.nameSize = static_cast<unsigned short>(findItr->second.name.size());

			outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
			if (outfile.bad())
				return false;

			if (!WriteWideString(outfile, findItr->second.id))
				return false;
			if (!WriteWideString(outfile, findItr->second.name))
				return false;
			if (!WriteWideString(outfile, findItr->second.description))
				return false;
		}
		else
			return false;
	}

	if (!_writeHeader(outfile, "CONSTGROUP", 1, static_cast<unsigned int>(_constantGroups.size())))
		return false;

	for (auto itr = _constantGroups.begin(); itr != _constantGroups.end(); itr++)
	{
		ConstGroupFileData data;
		data.idSize = static_cast<unsigned short>(itr->second.name.size());
		data.descSize = static_cast<unsigned short>(itr->second.desc.size());
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;
		if (!WriteWideString(outfile, itr->second.name))
			return false;
		if (!WriteWideString(outfile, itr->second.desc))
			return false;
	}

	if (!_writeHeader(outfile, "CONSTANTNS", 1, static_cast<unsigned int>(_constantNamespaces.size())))
		return false;

	for (auto itr = _constantNamespaces.begin(); itr != _constantNamespaces.end(); itr++)
	{
		ConstantNamespaceFileData data;
		data.idSize = static_cast<unsigned short>(itr->first.size());
		data.entries = static_cast<unsigned short>(itr->second.size());
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;
		if (!WriteWideString(outfile, itr->first))
			return false;

		for (auto cItr = itr->second.begin(); cItr != itr->second.end(); cItr++)
		{
			ConstantFileData data;
			data.id = static_cast<unsigned long>(cItr->second.id());
			data.subtype = static_cast<unsigned long>(cItr->second.subtype());
			data.idSize = static_cast<unsigned short>(cItr->first.size());
			data.groupSize = static_cast<unsigned short>(cItr->second.group() ? cItr->second.group()->name.size() : 0);
			outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
			if (outfile.bad())
				return false;
			if (!WriteWideString(outfile, cItr->first))
				return false;
			if (cItr->second.group())
			{
				if (!WriteWideString(outfile, cItr->second.group()->name))
					return false;
			}
		}
	}

	if (!_writeHeader(outfile, "CONSTANTS", 1, static_cast<unsigned int>(_constants.size())))
		return false;

	for (auto itr = _constants.begin(); itr != _constants.end(); itr++)
	{
		auto findItr = _constData.find(itr->second);
		if (findItr != _constData.end())
		{
			ConstantFileData data;
			data.id = static_cast<unsigned long>(findItr->second.id());
			data.subtype = static_cast<unsigned long>(findItr->second.subtype());
			data.idSize = static_cast<unsigned short>(itr->second.size());
			data.groupSize = static_cast<unsigned short>(findItr->second.group() ? findItr->second.group()->name.size() : 0);
			outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
			if (outfile.bad())
				return false;
			if (!WriteWideString(outfile, itr->second))
				return false;
			if (findItr->second.group())
			{
				if (!WriteWideString(outfile, findItr->second.group()->name))
					return false;
			}
		}
		else
		{
			XLib::String str(itr->second);
			if (str.contains("::"))
			{
				auto findItr = _constantNamespaces.find(str.token("::", 1));
				if (findItr != _constantNamespaces.end())
				{
					auto findItr2 = findItr->second.find(str.token("::", 2));
					if (findItr2 != findItr->second.end())
					{
						ConstantFileData data;
						data.id = static_cast<unsigned long>(findItr2->second.id());
						data.subtype = static_cast<unsigned long>(findItr2->second.subtype());
						data.idSize = static_cast<unsigned short>(str.size());
						data.groupSize = static_cast<unsigned short>(findItr2->second.group() ? findItr2->second.group()->name.size() : 0);
						outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
						if (outfile.bad())
							return false;
						if (!WriteWideString(outfile, str))
							return false;
						if (findItr2->second.group())
						{
							if (!WriteWideString(outfile, findItr2->second.group()->name))
								return false;
						}
					}
				}
			}
			else
				return false;
		}
	}

	if (!_writeHeader(outfile, "OBJCMDS", 1, static_cast<unsigned int>(_commands.size())))
		return false;

	for (auto itr = _commands.begin(); itr != _commands.end(); itr++)
	{
		CommandFileData data;
		data.datatype = static_cast<unsigned long>(itr->second.dt);
		data.entries = static_cast<unsigned short>(itr->second.list.size());
		data.nameSize = static_cast<unsigned short>(itr->second.name.size());
		data.descSize = static_cast<unsigned short>(itr->second.desc.size());
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;

		if (!WriteWideString(outfile, itr->second.name))
			return false;
		if (!WriteWideString(outfile, itr->second.desc))
			return false;

		for (auto cItr = itr->second.list.begin(); cItr != itr->second.list.end(); cItr++)
		{
			auto findItr = itr->second.data.find(cItr->second);
			if (findItr != itr->second.data.end())
			{
				ObjectCommandFileData data;
				data.id = static_cast<unsigned long>(findItr->second.num);
				data.idSize = static_cast<unsigned short>(findItr->second.id.size());
				data.nameSize = static_cast<unsigned short>(findItr->second.name.size());
				data.descSize = static_cast<unsigned short>(findItr->second.description.size());
				data.shortSize = static_cast<unsigned short>(findItr->second.shortName.size());
				outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
				if (outfile.bad())
					return false;
				if (!WriteWideString(outfile, findItr->second.id))
					return false;
				if (!WriteWideString(outfile, findItr->second.name))
					return false;
				if (!WriteWideString(outfile, findItr->second.shortName))
					return false;
				if (!WriteWideString(outfile, findItr->second.description))
					return false;
			}
			else
				return false;
		}
	}

	if (!_writeHeader(outfile, "GFUNC", 1, static_cast<unsigned int>(_globalFunctions.size())))
		return false;

	for (auto itr = _globalFunctions.begin(); itr != _globalFunctions.end(); itr++)
	{
		if (!_writeFunction(outfile, itr->first, itr->second))
			return false;
	}

	// Write function aliases (overloads) — simple format: funcId (ulong) + nameSize (ushort) + name
	{
		unsigned int totalAliasEntries = 0;
		for (const auto& a : _functionAliases)
			totalAliasEntries += static_cast<unsigned int>(a.second.size());

		if (!_writeHeader(outfile, "GALIAS", 1, totalAliasEntries))
			return false;

		for (const auto& a : _functionAliases)
		{
			for (unsigned int id : a.second)
			{
				unsigned long funcId = static_cast<unsigned long>(id);
				unsigned short nameSize = static_cast<unsigned short>(a.first.size());
				outfile.write(reinterpret_cast<char*>(&funcId), sizeof(funcId));
				outfile.write(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
				if (outfile.bad())
					return false;
				for (wchar_t c : a.first)
					outfile.write(reinterpret_cast<char*>(&c), sizeof(wchar_t));
			}
		}
	}

	if (!_writeHeader(outfile, "OFUNC", 1, static_cast<unsigned int>(_objectFunctions.size())))
		return false;

	for (auto itr = _objectFunctions.begin(); itr != _objectFunctions.end(); itr++)
	{
		if (!_writeFunction(outfile, itr->first, itr->second))
			return false;
	}

	if (!_writeHeader(outfile, "SFUNC", 1, static_cast<unsigned int>(_specialFunctions.size())))
		return false;

	for (auto itr = _specialFunctions.begin(); itr != _specialFunctions.end(); itr++)
	{
		SpecialFuncFileData data;
		data.id = static_cast<unsigned short>(itr->first);
		data.funcId = static_cast<unsigned short>(itr->second);
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;
	}

	if (!_writeHeader(outfile, "SPECIALKEY", 1, static_cast<unsigned int>(_specialKeywords.size())))
		return false;

	for (auto itr = _specialKeywords.begin(); itr != _specialKeywords.end(); itr++)
	{
		SpecialKeyFileData data;
		data.id = static_cast<unsigned short>(itr->second);
		data.idSize = static_cast<unsigned short>(itr->first.size());
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;
		if (!WriteWideString(outfile, itr->first))
			return false;
	}

	if (!_writeHeader(outfile, "OTFUNC", 1, static_cast<unsigned int>(_objectTypeFunctions.size())))
		return false;

	for (auto itr = _objectTypeFunctions.begin(); itr != _objectTypeFunctions.end(); itr++)
	{
		ObjectTypeFileData data;
		data.dt = static_cast<unsigned short>(itr->first);
		data.size = static_cast<unsigned short>(itr->second.size());
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;

		for (auto dItr = itr->second.begin(); dItr != itr->second.end(); dItr++)
		{
			ObjectTypeFileData dData;
			dData.dt = static_cast<unsigned short>(dItr->second);
			dData.size = static_cast<unsigned short>(dItr->first.size());
			outfile.write(reinterpret_cast<char*>(&dData), sizeof(dData));
			if (outfile.bad())
				return false;
			if (!WriteWideString(outfile, dItr->first))
				return false;
		}
	}

	if (!_writeHeader(outfile, "RACES", 1, static_cast<unsigned int>(_raceData.size())))
		return false;

	for (auto itr = _raceData.begin(); itr != _raceData.end(); itr++)
	{
		RaceFileData data;
		data.id = itr->first;
		data.codeSize = static_cast<unsigned short>(itr->second.code.size());
		data.descSize = static_cast<unsigned short>(itr->second.desc.size());
		data.nameSize = static_cast<unsigned short>(itr->second.name.size());
		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;

		if (!WriteWideString(outfile, itr->second.code))
			return false;
		if (!WriteWideString(outfile, itr->second.name))
			return false;
		if (!WriteWideString(outfile, itr->second.desc))
			return false;
	}

	if (!_writeHeader(outfile, "PROPERTIES", 1, static_cast<unsigned int>(_objectProperties.size())))
		return false;

	for (auto itr = _objectProperties.begin(); itr != _objectProperties.end(); itr++)
	{
		PropertiesData data;
		data.nameSize = static_cast<unsigned short>(itr->first.size());
		data.descSize = static_cast<unsigned short>(itr->second.description().size());
		data.getterID = static_cast<unsigned short>(itr->second.getter());
		data.setterID = static_cast<unsigned short>(itr->second.setter());

		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;
		if (!WriteWideString(outfile, itr->first))
			return false;
		if (!WriteWideString(outfile, itr->second.description()))
			return false;
	}

	if (!_writeHeader(outfile, "CUSTOM", 1, static_cast<unsigned int>(_customData.size())))
		return false;

	for (auto itr = _customData.begin(); itr != _customData.end(); itr++)
	{
		CustomFileData data;
		data.entries = static_cast<unsigned short>(itr->second.entries.size());
		data.datatype = static_cast<unsigned long>(itr->first);
		data.nameSize = static_cast<unsigned short>(itr->second.name.size());
		data.descSize = static_cast<unsigned short>(itr->second.desc.size());
		data.isString = itr->second.isStringData;

		outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
		if (outfile.bad())
			return false;

		if (!WriteWideString(outfile, itr->second.name))
			return false;
		if (!WriteWideString(outfile, itr->second.desc))
			return false;

		for (auto eItr = itr->second.entries.begin(); eItr != itr->second.entries.end(); eItr++)
		{
			CustomEntryFileData data;
			data.codeSize = static_cast<unsigned short>(eItr->second.code.size());
			data.descSize = static_cast<unsigned short>(eItr->second.desc.size());
			data.idSize = static_cast<unsigned short>(eItr->second.strID.size());
			data.id = eItr->second.intID;
			outfile.write(reinterpret_cast<char*>(&data), sizeof(data));
			if (outfile.bad())
				return false;

			if (!WriteWideString(outfile, eItr->second.code))
				return false;
			if (!WriteWideString(outfile, eItr->second.desc))
				return false;
			if (!WriteWideString(outfile, eItr->second.strID))
				return false;
		}
	}

	outfile.close();
	return true;
}

bool CScriptData::loadData(const std::wstring& filename)
{
	std::ifstream infile(filename, std::ios::binary);
	if (!infile || !infile.is_open())
		return false;

	DataFileHeader mainheader = _readHeader(infile, DATAVERSION);
	if (!mainheader.success || mainheader.header != "XSCRIPTDATA")
		return false;

	for (unsigned int i = 0; i < mainheader.count; i++)
	{
		DataFileHeader header = _readHeader(infile, 0);
		if (!header.success)
			return false;

		if (header.header == "PARDEF")
			_pardefData.resize(header.count);
		else if (header.header == "GFUNC")
			_functionData.resize(_functionData.size() + header.count);
		else if (header.header == "OFUNC")
			_functionData.resize(_functionData.size() + header.count);
		// GALIAS does not resize _functionData — IDs reference already-loaded GFUNC entries

		for (unsigned int j = 0; j < header.count; j++)
		{
			if (header.header == "DATATYPE")
			{
				DataTypeFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				std::wstring id = ReadWideString(infile, data.idSize);
				if (id.empty())
					return false;

				_dataTypes[id] = data.id;
				_dataTypesData[data.id] = { static_cast<DataTypes>(data.id), id, data.isObject ? true : false, L"", L"" };
				_constData[id] = ConstantData(DataTypes::DataType, id, data.id, static_cast<DataTypes>(data.id));

				std::wstring read = ReadWideString(infile, data.nameSize);
				if (read.empty() && data.nameSize > 0)
					return false;
				_dataTypesData[data.id].name = read;

				read = ReadWideString(infile, data.descSize);
				if (read.empty() && data.descSize > 0)
					return false;
				_dataTypesData[data.id].desc = read;

				read = ReadWideString(infile, data.prefixSize);
				if (read.empty() && data.prefixSize > 0)
					return false;
				_dataTypesData[data.id].prefix = read;
			}
			else if (header.header == "GAMEDATA")
			{
				GameDataFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				_gameData.engineMin = data.engineMin;
				_gameData.engineMax = data.engineMax;
				_gameData.language = data.language;

				for (unsigned int i = 0; i < data.texts; i++)
				{
					short text;
					infile.read(reinterpret_cast<char*>(&text), sizeof(text));
					if (infile.bad())
						return false;

					_gameData.textPrefixes.push_back(text);
				}

				_gameData.id = ReadWideString(infile, data.idSize);
				if (_gameData.id.empty() && data.idSize > 0)
					return false;

				_gameData.dir = ReadWideString(infile, data.dirSize);
				if (_gameData.dir.empty() && data.dirSize > 0)
					return false;

				_gameData.name = ReadWideString(infile, data.nameSize);
				if (_gameData.name.empty() && data.nameSize > 0)
					return false;
			}
			else if (header.header == "PARDEF")
			{
				ParDefFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;
				std::wstring id = ReadWideString(infile, data.idSize);
				if (id.empty())
					return false;

				if (data.id >= _pardefData.size())
					_pardefData.resize(data.id + 1);

				_pardefs[id] = static_cast<ParDef>(data.id);
				_pardefData[data.id] = { static_cast<ParDef>(data.id) , id, L"", L"" };
				_constData[id] = ConstantData(DataTypes::ParDef, id, data.id);
				_pardefData[data.id].flags = static_cast<ParDefFlags>(data.flags);

				std::wstring read = ReadWideString(infile, data.nameSize);
				if (read.empty() && data.nameSize > 0)
					return false;
				_pardefData[data.id].name = read;

				read = ReadWideString(infile, data.descSize);
				if (read.empty() && data.descSize > 0)
					return false;
				_pardefData[data.id].desc = read;

				for (unsigned k = 0; k < data.datatypes; k++)
				{
					long dt;
					infile.read(reinterpret_cast<char*>(&dt), sizeof(dt));
					if (infile.bad())
						return false;
					_pardefData[data.id].datatypes.insert(static_cast<DataTypes>(dt));
				}
			}
			else if (header.header == "WARES")
			{
				WareTypeFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				std::wstring id = ReadWideString(infile, data.idSize);
				if (id.empty())
					return false;

				_wareTypes[id] = data.id;
				_wareTypesData[data.id] = { id, L"", L"", data.id, MAINTYPE(data.id), SUBTYPE(data.id) };
				_constData[id] = ConstantData(DataTypes::Ware, id, data.id);

				std::wstring read = ReadWideString(infile, data.nameSize);
				if (read.empty() && data.nameSize > 0)
					return false;
				_wareTypesData[data.id].name = read;

				read = ReadWideString(infile, data.descSize);
				if (read.empty() && data.descSize > 0)
					return false;
				_wareTypesData[data.id].description = read;
			}
			else if (header.header == "CONSTGROUP")
			{
				ConstGroupFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				std::wstring id = ReadWideString(infile, data.idSize);
				if (id.empty())
					return false;
				std::wstring read = ReadWideString(infile, data.descSize);
				if (read.empty() && data.descSize > 0)
					return false;
				_constantGroups[id] = { id, read };
			}
			else if (header.header == "CONSTANTNS")
			{
				ConstantNamespaceFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				std::wstring ns = ReadWideString(infile, data.idSize);
				if (ns.empty())
					return false;

				auto& map = _constantNamespaces[ns];

				for (unsigned int i = 0; i < data.entries; i++)
				{
					ConstantFileData data;
					infile.read(reinterpret_cast<char*>(&data), sizeof(data));
					if (infile.bad())
						return false;

					std::wstring id = ReadWideString(infile, data.idSize);
					if (id.empty())
						return false;

					ConstGroup* group = NULL;
					if (data.groupSize)
					{
						std::wstring read = ReadWideString(infile, data.groupSize);
						if (read.empty() && data.groupSize > 0)
							return false;
						auto findItr = _constantGroups.find(read);
						if (findItr != _constantGroups.end())
							group = &findItr->second;
					}

					map[id] = ConstantData(DataTypes::Constant, id, data.id, group, static_cast<DataTypes>(data.subtype));
				}
			}
			else if (header.header == "CONSTANTS")
			{
				ConstantFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				std::wstring id = ReadWideString(infile, data.idSize);
				if (id.empty())
					return false;

				ConstGroup* group = NULL;
				if (data.groupSize)
				{
					std::wstring read = ReadWideString(infile, data.groupSize);
					if (read.empty() && data.groupSize > 0)
						return false;
					auto findItr = _constantGroups.find(read);
					if (findItr != _constantGroups.end())
						group = &findItr->second;
				}

				_constants[data.id] = id;
				_constData[id] = ConstantData(DataTypes::Constant, id, data.id, group, static_cast<DataTypes>(data.subtype));
			}
			else if (header.header == "OBJCMDS")
			{
				CommandFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				DataTypes dt = static_cast<DataTypes>(data.datatype);
				_commands[dt].dt = dt;

				std::wstring read = ReadWideString(infile, data.nameSize);
				if (read.empty() && data.nameSize > 0)
					return false;
				_commands[dt].name = read;

				read = ReadWideString(infile, data.descSize);
				if (read.empty() && data.descSize > 0)
					return false;
				_commands[dt].desc = read;

				for (unsigned int i = 0; i < data.entries; i++)
				{
					ObjectCommandFileData data;
					infile.read(reinterpret_cast<char*>(&data), sizeof(data));
					if (infile.bad())
						return false;
					std::wstring id = ReadWideString(infile, data.idSize);
					if (id.empty())
						return false;

					_commands[dt].data[data.id] = { id, L"", L"", L"", data.id };
					_commands[dt].list[id] = data.id;
					_constData[id] = ConstantData(dt, id, data.id);

					read = ReadWideString(infile, data.nameSize);
					if (read.empty() && data.nameSize > 0)
						return false;
					_commands[dt].data[data.id].name = read;

					read = ReadWideString(infile, data.shortSize);
					if (read.empty() && data.shortSize > 0)
						return false;
					_commands[dt].data[data.id].shortName = read;

					read = ReadWideString(infile, data.descSize);
					if (read.empty() && data.descSize > 0)
						return false;
					_commands[dt].data[data.id].description = read;
				}
			}
			else if (header.header == "SFUNC")
			{
				SpecialFuncFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;
				_specialFunctions[static_cast<SpecialFunction>(data.id)] = static_cast<unsigned int>(data.funcId);
			}
			else if (header.header == "GFUNC")
			{
				if (!_readFunction(infile, _globalFunctions))
					return false;
			}
			else if (header.header == "GALIAS")
			{
				// Read one alias entry: funcId (ulong) + nameSize (ushort) + name (wchars)
				unsigned long funcId;
				infile.read(reinterpret_cast<char*>(&funcId), sizeof(funcId));
				if (infile.bad())
					return false;
				unsigned short nameSize;
				infile.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
				if (infile.bad())
					return false;
				std::wstring aliasName;
				aliasName.resize(nameSize);
				for (unsigned short ci = 0; ci < nameSize; ci++)
				{
					wchar_t c;
					infile.read(reinterpret_cast<char*>(&c), sizeof(wchar_t));
					aliasName[ci] = c;
				}
				if (!aliasName.empty())
					_functionAliases[aliasName].push_back(static_cast<unsigned int>(funcId));
			}
			else if (header.header == "OFUNC")
			{
				if (!_readFunction(infile, _objectFunctions))
					return false;
			}
			else if (header.header == "SPECIALKEY")
			{
				SpecialKeyFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;
				std::wstring id = ReadWideString(infile, data.idSize);
				if (id.empty())
					return false;
				_specialKeywords[id] = static_cast<unsigned int>(data.id);
			}
			else if (header.header == "OTFUNC")
			{
				ObjectTypeFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				DataTypes dt = static_cast<DataTypes>(data.dt);
				for (auto i = 0; i < data.size; i++)
				{
					ObjectTypeFileData dData;
					infile.read(reinterpret_cast<char*>(&dData), sizeof(dData));
					if (infile.bad())
						return false;
					std::wstring read = ReadWideString(infile, dData.size);
					if (read.empty())
						return false;

					_objectTypeFunctions[dt][read] = static_cast<unsigned int>(dData.dt);
				}
			}
			else if (header.header == "RACES")
			{
				RaceFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				_raceData[data.id] = RaceData();
				_raceData[data.id].id = data.id;

				_raceData[data.id].code = ReadWideString(infile, data.codeSize);
				if (_raceData[data.id].code.empty() && data.codeSize > 0)
					return false;
				_raceData[data.id].name = ReadWideString(infile, data.nameSize);
				if (_raceData[data.id].name.empty() && data.nameSize > 0)
					return false;
				_raceData[data.id].desc = ReadWideString(infile, data.descSize);
				if (_raceData[data.id].desc.empty() && data.descSize > 0)
					return false;
				_races[_raceData[data.id].code] = data.id;
				_constData[_raceData[data.id].code] = ConstantData(DataTypes::Race, _raceData[data.id].code, data.id);
			}
			else if (header.header == "PROPERTIES")
			{
				PropertiesData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				std::wstring name = ReadWideString(infile, data.nameSize);
				std::wstring desc = ReadWideString(infile, data.descSize);
				_objectProperties[name] = { data.getterID, data.setterID, desc };
			}
			else if (header.header == "CUSTOM")
			{
				CustomFileData data;
				infile.read(reinterpret_cast<char*>(&data), sizeof(data));
				if (infile.bad())
					return false;

				DataTypes dt = static_cast<DataTypes>(data.datatype);
				_customData[dt] = CustomData();
				_customData[dt].datatype = dt;
				_customData[dt].isStringData = data.isString;

				std::wstring read = ReadWideString(infile, data.nameSize);
				if (read.empty() && data.nameSize > 0)
					return false;
				_customData[dt].name = read;
				read = ReadWideString(infile, data.descSize);
				if (read.empty() && data.descSize > 0)
					return false;
				_customData[dt].desc = read;

				for (unsigned int i = 0; i < data.entries; i++)
				{
					CustomEntryFileData data;
					infile.read(reinterpret_cast<char*>(&data), sizeof(data));
					if (infile.bad())
						return false;

					CustomDataEntry entry;
					entry.code = ReadWideString(infile, data.codeSize);
					if (entry.code.empty() && data.codeSize > 0)
						return false;
					entry.desc = ReadWideString(infile, data.descSize);
					if (entry.desc.empty() && data.descSize > 0)
						return false;
					entry.strID = ReadWideString(infile, data.idSize);
					if (entry.strID.empty() && data.idSize > 0)
						return false;
					entry.intID = data.id;

					_customData[dt].entries[entry.code] = entry;
					unsigned int keyID = 0;
					if (_customData[dt].isStringData)
					{
						keyID = static_cast<unsigned int>(_customData[dt].strKeys.size());
						_customData[dt].strKeys.push_back(entry.strID);
						_customData[dt].strLookup[entry.strID] = entry.code;
					}
					else
					{
						keyID = static_cast<unsigned int>(_customData[dt].intKeys.size());
						_customData[dt].intKeys.push_back(entry.intID);
						_customData[dt].intLookup[entry.intID] = entry.code;
					}
					_customData[dt].keyMap[entry.code] = keyID;
					_constData[entry.code] = ConstantData(DataTypes::Custom, entry.code, keyID, _customData[dt].datatype);
					_constData[entry.code].setStrData(entry.strID);
				}
			}
		}
	}

	infile.close();
	return true;
}

bool CScriptData::_writeHeader(std::ofstream& out, const std::string& name, unsigned int version, unsigned int count)
{
	DataFileHeaderRaw header;
	header.version = version;
	memset(header.header, 0, sizeof(header.header));
#ifdef _WIN32
	strcpy_s(header.header, name.c_str());
#else
	strcpy(mainheader.header, name.c_str());
#endif
	// set the count to the number of data entries that follows
	header.count = count;
	out.write(reinterpret_cast<char*>(&header), sizeof(header));
	if (out.bad())
		return false;

	return true;
}

CScriptData::DataFileHeader CScriptData::_readHeader(std::ifstream& in, short version)
{
	DataFileHeaderRaw header;
	in.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (!in.good())
		return DataFileHeader({ false });

	// check the version if specifed
	if (version && header.version > version)
		return DataFileHeader({ false });

	return { true, header.header, static_cast<unsigned int>(header.version), static_cast<unsigned int>(header.count) };
}

bool CScriptData::_writeFunction(std::ofstream& out, const std::wstring& name, unsigned int id)
{
	auto* funcData = &_functionData[id];
	FunctionFileData data;
	data.id = static_cast<unsigned long>(id);
	data.idSize = static_cast<unsigned short>(name.size());
	data.descSize = static_cast<unsigned short>(funcData->description.size());
	data.allowNull = funcData->allowNull;
	data.argCount = static_cast<unsigned short>(funcData->arguments.size());
	data.refObj = static_cast<unsigned short>(funcData->refObjType.size());
	data.exampleSize = static_cast<unsigned short>(funcData->example.size());
	data.orderCount = static_cast<unsigned short>(funcData->order.size());
	data.returnArg = static_cast<unsigned short>(funcData->returnArgument);
	data.returnType = static_cast<unsigned short>(funcData->returnValueType);
	data.returnCount = static_cast<unsigned short>(funcData->returnValue.size());
	data.undefinedArgs = funcData->undefinedCount;
	out.write(reinterpret_cast<char*>(&data), sizeof(data));
	if (out.bad())
		return false;
	if (!WriteWideString(out, name))
		return false;
	if (!WriteWideString(out, funcData->description))
		return false;
	if (!WriteWideString(out, funcData->example))
		return false;

	for (auto itr = funcData->order.begin(); itr != funcData->order.end(); itr++)
	{
		unsigned short size = static_cast<unsigned short>(itr->size());
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		if (out.bad())
			return false;
		if (!WriteWideString(out, *itr))
			return false;
	}
	for (auto itr = funcData->refObjType.begin(); itr != funcData->refObjType.end(); itr++)
	{
		unsigned long val = static_cast<unsigned long>(*itr);
		out.write(reinterpret_cast<char*>(&val), sizeof(val));
		if (out.bad())
			return false;
	}
	for (auto itr = funcData->returnValue.begin(); itr != funcData->returnValue.end(); itr++)
	{
		unsigned long val = static_cast<unsigned long>(*itr);   // must match refObjType write above
		out.write(reinterpret_cast<char*>(&val), sizeof(val));
		if (out.bad())
			return false;
	}

	for (auto itr = funcData->arguments.begin(); itr != funcData->arguments.end(); itr++)
	{
		FunctionArgFileData aData;
		aData.id = static_cast<unsigned long>(itr->pardef);
		aData.descSize = static_cast<unsigned short>(itr->description.size());
		aData.groupSize = static_cast<unsigned short>(itr->constGroup ? itr->constGroup->name.size() : 0);
		out.write(reinterpret_cast<char*>(&aData), sizeof(aData));
		if (out.bad())
			return false;
		if (!WriteWideString(out, itr->description))
			return false;
		if (itr->constGroup)
		{
			if (!WriteWideString(out, itr->constGroup->name))
				return false;
		}
	}

	return true;
}

bool CScriptData::_readFunction(std::ifstream& in, std::map<const std::wstring, unsigned int>& list)
{
	FunctionFileData data;
	in.read(reinterpret_cast<char*>(&data), sizeof(data));
	if (in.bad())
		return false;
	std::wstring id = ReadWideString(in, data.idSize);
	if (id.empty())
		return false;

	list[id] = data.id;

	if (data.id >= _functionData.size())
		_functionData.resize(data.id + 1);

	_functionData[data.id].id = data.id;
	_functionData[data.id].allowNull = data.allowNull;
	_functionData[data.id].undefinedCount = data.undefinedArgs;
	_functionData[data.id].returnArgument = data.returnArg;
	_functionData[data.id].returnValueType = static_cast<RetVarType>(data.returnType);
	_functionData[data.id].name = id;
	_functionData[data.id].arguments.clear();
	_functionData[data.id].order.clear();
	_functionData[data.id].refObjType.clear();
	_functionData[data.id].returnValue.clear();

	std::wstring read = ReadWideString(in, data.descSize);
	if (read.empty() && data.descSize > 0)
		return false;
	_functionData[data.id].description = read;

	read = ReadWideString(in, data.exampleSize);
	if (read.empty() && data.exampleSize > 0)
		return false;
	_functionData[data.id].example = read;

	for (auto i = 0; i < data.orderCount; i++)
	{
		unsigned short size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		if (in.bad())
			return false;

		if (size)
		{
			read = ReadWideString(in, size);
			if (read.empty())
				return false;
			_functionData[data.id].order.push_back(read);
		}
	}

	for (auto i = 0; i < data.refObj; i++)
	{
		unsigned long val;
		in.read(reinterpret_cast<char*>(&val), sizeof(val));
		if (in.bad())
			return false;
		_functionData[data.id].refObjType.insert(static_cast<DataTypes>(val));
	}

	for (auto i = 0; i < data.returnCount; i++)
	{
		unsigned long val;
		in.read(reinterpret_cast<char*>(&val), sizeof(val));
		if (in.bad())
			return false;
		_functionData[data.id].returnValue.insert(static_cast<DataTypes>(val));
	}

	for (auto i = 0; i < data.argCount; i++)
	{
		FunctionArgFileData aData;
		in.read(reinterpret_cast<char*>(&aData), sizeof(aData));
		if (in.bad())
			return false;

		read = ReadWideString(in, aData.descSize);
		if (read.empty() && aData.descSize > 0)
			return false;

		ConstGroup* group = NULL;
		if (aData.groupSize)
		{
			std::wstring groupName = ReadWideString(in, aData.groupSize);
			if (groupName.empty() && aData.groupSize > 0)
				return false;
			auto findItr = _constantGroups.find(groupName);
			if (findItr != _constantGroups.end())
				group = &findItr->second;
		}

		_functionData[data.id].arguments.push_back({ static_cast<ParDef>(aData.id), read, group });
	}

	return true;
}