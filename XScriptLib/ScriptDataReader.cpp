#include "pch.h"
#include "ScriptDataReader.h"
#include "CScriptData.h"
#include "Utils.h"

#include "XScriptLib.h"
#include "VFSHelper.h"

#include <sstream>
#include <io.h>

using namespace XScript;

////////////////////////////////////////////////////////////////////////////////////////////////

ParDefFlags GetParDefFlag(const std::wstring& flag)
{
	if (flag == L"Constant")
		return ParDefFlags::Constant;
	else if (flag == L"String")
		return ParDefFlags::String;
	else if (flag == L"Integer" || flag == L"Number")
		return ParDefFlags::Integer;
	else if (flag == L"Object")
		return ParDefFlags::Object;
	else if (flag == L"Variable")
		return ParDefFlags::Variable;

	return ParDefFlags::None;
}

RetVarType GetRetVarType(const std::wstring& type)
{
	if (type == L"noif")
		return RetVarType::NoIf;
	else if (type == L"no_if")
		return RetVarType::NoIf;
	else if (type == L"if")
		return RetVarType::If;
	else if (type == L"noifstart")
		return RetVarType::NoIfStart;
	else if (type == L"start")
		return RetVarType::NoIfStart;

	return RetVarType::Return;
}

////////////////////////////////////////////////////////////////////////////////////////////////

ScriptDataReader::ScriptDataReader(CScriptData* data) : _pData(data)
{

}

bool ScriptDataReader::readData(const std::wstring& filename)
{
	if (!_pData)
		throw std::exception("XML Read Error, invalid data input");

	_pData->resetData();

	// resize a certain amount (doesn't matter if its more or less than we need)
	// reduces the number of times we need to resize the arrays to save on memory allocation
	_pData->_pardefData.resize(100);
	_pData->_functionData.resize(4000);

	std::wifstream inFile(filename);
	if (!inFile.good())
		throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, unable to read file: ", filename)).c_str());

	std::vector<wchar_t>* buffer = new std::vector<wchar_t>((std::istreambuf_iterator<wchar_t>(inFile)), std::istreambuf_iterator<wchar_t>());
	buffer->push_back('\0');

	rapidxml::xml_document<wchar_t>* doc = new rapidxml::xml_document<wchar_t>();
	doc->parse<0>(&(*buffer)[0]);

	rapidxml::xml_node<wchar_t>* root_node = doc->first_node(L"XScript");

	if (!root_node)
	{
		delete buffer;
		delete doc;
		throw std::exception("XML Read Error, unable to find root node <XScript>");
	}

	// read the game data
	_readGameData(root_node);

	// Build language suffix for local fallback — e.g. language=44 -> "L044"
	std::wstring langSuffix = L"L";
	{
		std::wstring langStr = std::to_wstring(_pData->_gameData.language);
		while (langStr.size() < 3) langStr = L"0" + langStr;
		langSuffix += langStr;
	}

	int language = static_cast<int>(_pData->_gameData.language);

	// When VFS is loaded, text lookups go through vfsFindText() — the VFS
	// already loaded all text files when LoadFilesystem() was called, so no
	// manual text file scanning is needed. When VFS is not loaded, fall back
	// to loading local Data\t\ files into an XLib::TextDB as before.
	// Text lookup uses VFS when loaded; empty strings in local mode (no TextDB)

	// read all the data types
	_readDataTypes(root_node);

	// read all the pardef
	_readParDefs(root_node);

	// read all the constant data
	_readConstants(root_node);

	// read all the function data
	_readFunctions(root_node);

	// read all object commands
	_readCommands(root_node, language, nullptr);

	// read all the ware types (wares, ships, stations, etc)
	_readWareTypes(root_node, language, nullptr);

	// read all the races
	_readRaces(root_node);

	// read all the object properties
	_readProperties(root_node);

	// read any of the custom entries
	_readCustomEntries(root_node);

	// read macro definitions
	_readMacros(root_node);

	// finished, clear the data
	delete buffer;
	delete doc;
	return true;

}

////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptDataReader::_readGameData(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* node = root_node->first_node(L"Game");
	if (node)
	{
		{
			rapidxml::xml_node<wchar_t>* n = node->first_node(L"EngineVersion");
			if (n)
			{
				for (rapidxml::xml_attribute<wchar_t>* attr = n->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::wstring name = attr->name();
					if (name == L"min")
						_pData->_gameData.engineMin = std::stoi(attr->value());
					if (name == L"max")
						_pData->_gameData.engineMax = std::stoi(attr->value());

				}
			}
		}
		for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::wstring name = attr->name();
			if (name == L"id")
				_pData->_gameData.id = attr->value();
			else if (name == L"name")
				_pData->_gameData.name = attr->value();
			else if (name == L"directory")
				_pData->_gameData.dir = attr->value();
			else if (name == L"language")
				_pData->_gameData.language = std::stoi(attr->value());
		}

		for (rapidxml::xml_node<wchar_t>* childNode = node->first_node(L"TextPrefix"); childNode; childNode = childNode->next_sibling(L"TextPrefix"))
		{
			for (rapidxml::xml_attribute<wchar_t>* attr = childNode->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring name = attr->name();
				if (name == L"value")
					_pData->_gameData.textPrefixes.push_back(static_cast<unsigned int>(std::stoi(attr->value())));
			}
		}
	}
}

void ScriptDataReader::_readDataTypes(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* node = root_node->first_node(L"DataTypes");
	if (node)
	{
		for (rapidxml::xml_node<wchar_t>* childNode = node->first_node(L"DataType"); childNode; childNode = childNode->next_sibling(L"DataType"))
		{
			DataTypeData data;
			data.id = DataTypes::Invalid;
			for (rapidxml::xml_attribute<wchar_t>* attr = childNode->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring value = attr->value();
				std::wstring name = attr->name();

				try {
					if (name == L"id")
						data.id = static_cast<DataTypes>(std::stoi(value));
					else if (name == L"keyword")
						data.code = value;
					else if (name == L"description")
						data.desc = value;
					else if (name == L"isobject")
						data.isObject = (value == L"true");
					else if (name == L"name")
						data.name = value;
					else if (name == L"prefix")
						data.prefix = value;
				}
				catch (std::exception)
				{
					if (name == L"id")
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, DataTypes, Invalid datatype id value, '", value, L"'")).c_str());
					else
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, DataTypes, Invalid datatype value for: ", name, L" = '", value, L"'")).c_str());
				}
			}

			// check for errors
			if (data.id == DataTypes::Invalid)
				throw std::exception(Utils::ws2s(L"XML Read Error, DataTypes, Invalid datatype id value, missing?").c_str());
			else if (_pData->_dataTypesData.find(static_cast<unsigned int>(data.id)) != _pData->_dataTypesData.end())
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, DataTypes, Duplicate datatype id value found '", std::to_wstring(static_cast<int>(data.id)), L"'")).c_str());

			if (data.name.empty())
				data.name = data.code;
			if (data.desc.empty())
				data.desc = data.name;

			_checkConstant(data.code, L"DataTypes");

			if (data.id != DataTypes::Invalid)
			{
				unsigned int iId = static_cast<unsigned int>(data.id);
				_pData->_dataTypesData[iId] = data;
				if (!data.code.empty())
				{
					_pData->_dataTypes[data.code] = iId;
					_pData->_constData[data.code] = ConstantData(DataTypes::DataType, data.code, iId, data.id);
				}
			}
		}
	}
}

void ScriptDataReader::_readParDefs(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* node = root_node->first_node(L"ParDefs");
	if (node)
	{
		for (rapidxml::xml_node<wchar_t>* childNode = node->first_node(L"ParDef"); childNode; childNode = childNode->next_sibling(L"ParDef"))
		{
			ParDefData data;
			data.id = ParDef::Unknown;
			data.flags = ParDefFlags::None;
			for (rapidxml::xml_attribute<wchar_t>* attr = childNode->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring value = attr->value();
				std::wstring name = attr->name();
				if (name == L"id")
					data.id = static_cast<ParDef>(std::stoi(value));
				else if (name == L"keyword")
					data.code = value;
				else if (name == L"description")
					data.desc = value;
				else if (name == L"name")
					data.name = value;
			}
			if (data.id == ParDef::Unknown)
				throw std::exception(Utils::ws2s(L"XML Read Error, ParDefs, Invalid ParDef id").c_str());
			else if (data.code.empty())
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, ParDefs, Code entry missing, id=", std::to_wstring(static_cast<int>(data.id)))).c_str());
			else
			{
				_checkConstant(data.code, L"ParDefs");

				unsigned int id = static_cast<unsigned int>(data.id);
				if (id >= _pData->_pardefData.size())
					_pData->_pardefData.resize(id + 1);
				_pData->_pardefData[id] = data;
				if (!data.code.empty())
				{
					_pData->_pardefs[data.code] = data.id;
					_pData->_constData[data.code] = ConstantData(DataTypes::ParDef, data.code, id);
				}

				ParDefData* createdData = &_pData->_pardefData[static_cast<unsigned int>(data.id)];

				rapidxml::xml_node<wchar_t>* flags_node = childNode->first_node(L"Flags");
				if (flags_node)
				{
					for (rapidxml::xml_node<wchar_t>* childNodeFlags = flags_node->first_node(L"Flag"); childNodeFlags; childNodeFlags = childNodeFlags->next_sibling(L"Flag"))
					{
						for (rapidxml::xml_attribute<wchar_t>* attr = childNodeFlags->first_attribute(); attr; attr = attr->next_attribute())
						{
							std::wstring name = attr->name();
							if (name == L"value")
							{
								createdData->flags = createdData->flags | GetParDefFlag(attr->value());
								break;
							}
						}
					}
				}

				rapidxml::xml_node<wchar_t>* datatype_node = childNode->first_node(L"DataTypes");
				if (datatype_node)
				{
					for (rapidxml::xml_node<wchar_t>* childNodeDT = datatype_node->first_node(L"DataType"); childNodeDT; childNodeDT = childNodeDT->next_sibling(L"DataType"))
					{
						for (rapidxml::xml_attribute<wchar_t>* attr = childNodeDT->first_attribute(); attr; attr = attr->next_attribute())
						{
							std::wstring name = attr->name();
							if (name == L"id")
							{
								createdData->datatypes.insert(_convertDataType(attr->value(), Utils::CombineStrings(L"ParDef: ", data.code)));
								break;
							}
						}
					}
				}
			}
		}
	}
}

void ScriptDataReader::_readConstants(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* node = root_node->first_node(L"Constants");
	if (node)
	{
		ConstantData data;
		for (rapidxml::xml_node<wchar_t>* childNode = node->first_node(); childNode; childNode = childNode->next_sibling())
		{
			std::wstring childName = childNode->name();
			if (childName == L"Constant")
				_readConstant(childNode, NULL);
			else if (childName == L"ConstantGroup")
			{
				std::wstring code, desc;
				bool ns = false;
				for (rapidxml::xml_attribute<wchar_t>* attr = childNode->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::wstring name = attr->name();
					if (name == L"code")
						code = attr->value();
					else if (name == L"description")
						desc = attr->value();
					else if (name == L"namespace")
						ns = _parseBoolean(attr->value());
				}

				if (code.empty())
					throw std::exception(Utils::ws2s(L"XML Read Error, Constants, missing code entry for constant group").c_str());

				if (_pData->_constantGroups.find(code) != _pData->_constantGroups.end())
					throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Constants, Duplicate constant group found '", code, L"'")).c_str());

				_pData->_constantGroups[code] = { code, desc, ns };
				ConstGroup* group = &_pData->_constantGroups[code];
				for (rapidxml::xml_node<wchar_t>* node = childNode->first_node(L"Constant"); node; node = node->next_sibling(L"Constant"))
					_readConstant(node, group);
			}
		}
	}
}

void ScriptDataReader::_readConstant(rapidxml::xml_node<wchar_t>* root_node, ConstGroup* group)
{
	if (std::wstring(root_node->name()) != L"Constant")
		return;

	std::wstring id, code, desc, type = L"1", global;
	for (rapidxml::xml_attribute<wchar_t>* attr = root_node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::wstring name = attr->name();
		if (name == L"id")
			id = attr->value();
		else if (name == L"code")
			code = attr->value();
		else if (name == L"description")
			desc = attr->value();
		else if (name == L"type")
			type = attr->value();
		else if (name == L"globalcode")
			global = attr->value();
	}

	if (id.empty())
		throw std::exception(Utils::ws2s(L"XML Read Error, Constants, Missing id entry").c_str());
	else if (code.empty())
		throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Constants, Missing code entry, id=", id)).c_str());
	else
	{
		if (group && group->ns)
		{
			_checkConstant(code, L"Constants", false);
			auto& map = _pData->_constantNamespaces[group->name];
			if (map.find(code) != map.end())
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Constant Duplicate constant found '", code, L"' for namespace: ", group->name)).c_str());
			map[code] = { DataTypes::Constant, code, static_cast<unsigned int>(stoi(id)), group, _convertDataType(type, Utils::CombineStrings(L"Constant: ", id)) };

			if (!global.empty())
			{
				_checkConstant(global, L"Constants");

				_pData->_constData[global] = { DataTypes::Constant, global, static_cast<unsigned int>(stoi(id)), group, _convertDataType(type, Utils::CombineStrings(L"Constant: ", id)) };
			}

			_pData->_constants[static_cast<unsigned int>(stoi(id))] = std::wstring(group->name) + L"::" + code;
		}
		else
		{
			_checkConstant(code, L"Constants");

			_pData->_constData[code] = { DataTypes::Constant, code, static_cast<unsigned int>(stoi(id)), group, _convertDataType(type, Utils::CombineStrings(L"Constant: ", id)) };
			_pData->_constants[static_cast<unsigned int>(stoi(id))] = code;
		}
	}
}

void ScriptDataReader::_readFunctions(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* node = root_node->first_node(L"Functions");
	if (node)
	{
		for (rapidxml::xml_node<wchar_t>* childNode = node->first_node(L"Function"); childNode; childNode = childNode->next_sibling(L"Function"))
		{
			std::wstring id, desc, code, object, specialType;
			bool allowKeyword = false;
			for (rapidxml::xml_attribute<wchar_t>* attr = childNode->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring name = attr->name();
				if (name == L"id")
					id = attr->value();
				else if (name == L"description")
					desc = attr->value();
				else if (name == L"name")
					code = attr->value();
				else if (name == L"object")
					object = attr->value();
				else if (name == L"specialtype")
					specialType = attr->value();
				else if (name == L"allowkeyword")
					allowKeyword = _parseBoolean(attr->value());
			}

			if (id.empty())
				throw std::exception(Utils::ws2s(L"XML Read Error, Function, missing id entry").c_str());
			else
			{
				unsigned int iID = static_cast<unsigned int>(std::stoi(id));
				if (iID >= _pData->_functionData.size())
					_pData->_functionData.resize(iID + 1);

				if (allowKeyword)
				{
					if (_pData->_constData.find(code) != _pData->_constData.end())
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, Keyword matches existing constant '", code, L"'")).c_str());
					if (_pData->_specialKeywords.find(code) != _pData->_specialKeywords.end())
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, Duplicate keyword found '", code, L"'")).c_str());

					_pData->_specialKeywords[code] = iID;
				}

				if (!_pData->_functionData[iID].name.empty())
					throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, Duplicate command entry found ", id, L" '", code, L"'")).c_str());

				_pData->_functionData[iID].name = code;
				_pData->_functionData[iID].id = iID;
				_pData->_functionData[iID].description = desc;
				_pData->_functionData[iID].returnValueType = RetVarType::None;

				if (!specialType.empty())
				{
					if (specialType == L"continue")
						_pData->_continueCommand = iID;
					else if (specialType == L"break")
						_pData->_breakCommand = iID;
					else if (specialType == L"end")
						_pData->_endCommand = iID;
					else if (specialType == L"else")
						_pData->_elseCommand = iID;
					else if (specialType == L"return")
						_pData->_returnCommand = iID;
					else if (specialType == L"goto")
						_pData->_gotoCommand = iID;
					else if (specialType == L"gosub")
						_pData->_gosubCommand = iID;
					else if (specialType == L"definelabel")
						_pData->_defineLabelCommand = iID;
					else if (specialType == L"getarray")
						_pData->_specialFunctions[SpecialFunction::GetArray] = iID;
					else if (specialType == L"setarray")
						_pData->_specialFunctions[SpecialFunction::SetArray] = iID;
					else if (specialType == L"arrayassign")
						_pData->_specialFunctions[SpecialFunction::SetArrayFromArray] = iID;
					else if (specialType == L"getdoublearray")
						_pData->_specialFunctions[SpecialFunction::GetArrayDouble] = iID;
					else if (specialType == L"setdoublearray")
						_pData->_specialFunctions[SpecialFunction::SetArrayDouble] = iID;
					else
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, unknown special type '", specialType, L"'")).c_str());
				}

				rapidxml::xml_node<wchar_t>* example_node = childNode->first_node(L"Example");
				if (example_node)
					_pData->_functionData[iID].example = example_node->value();

				{
					rapidxml::xml_node<wchar_t>* n = childNode->first_node(L"ObjectTypes");
					if (n)
					{
						for (rapidxml::xml_node<wchar_t>* node = n->first_node(L"ObjectType"); node; node = node->next_sibling(L"ObjectType"))
						{
							std::wstring code, type;
							for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
							{
								std::wstring name = attr->name();
								if (name == L"name")
									code = attr->value();
								else if (name == L"datatype")
									type = attr->value();
							}

							if (!code.empty() && !type.empty())
								_pData->_objectTypeFunctions[_convertDataType(type, Utils::CombineStrings(L"Function: ", code))][code] = iID;
						}
					}

				}
				bool hasRefObj = false;
				rapidxml::xml_node<wchar_t>* arg_node = childNode->first_node(L"Arguments");
				if (arg_node)
				{
					for (rapidxml::xml_attribute<wchar_t>* attr = arg_node->first_attribute(); attr; attr = attr->next_attribute())
					{
						std::wstring name = attr->name();
						if (name == L"allownull")
							_pData->_functionData[iID].allowNull = _parseBoolean(attr->value());
						else if (name == L"allownullobject")
							_pData->_functionData[iID].allowNullObject = _parseBoolean(attr->value());
						else if (name == L"undefinedcount")
							_pData->_functionData[iID].undefinedCount = std::stoi(attr->value());
					}

					int arg = 0;
					for (rapidxml::xml_node<wchar_t>* argChildNode = arg_node->first_node(); argChildNode; argChildNode = argChildNode->next_sibling())
					{
						std::wstring name = argChildNode->name();
						if (name == L"ReturnValue")
						{
							_pData->_functionData[iID].returnValueType = RetVarType::Return;
							_pData->_functionData[iID].order.push_back(L"RetVar");
							for (rapidxml::xml_attribute<wchar_t>* attr = argChildNode->first_attribute(); attr; attr = attr->next_attribute())
							{
								std::wstring name = attr->name();
								if (name == L"type")
									_pData->_functionData[iID].returnValueType = GetRetVarType(attr->value());
							}

							for (rapidxml::xml_node<wchar_t>* retVarChildNode = argChildNode->first_node(L"ReturnValue"); retVarChildNode; retVarChildNode = retVarChildNode->next_sibling(L"ReturnValue"))
								_pData->_functionData[iID].returnValue.insert(_convertDataType(retVarChildNode->value(), Utils::CombineStrings(L"Function: ", code)));
						}
						else if (name == L"Argument" || name == L"ReturnArgument")
						{
							bool isReturnArg = (name == L"ReturnArgument");
							std::wstringstream strm;
							strm << L"Arg" << arg++;

							if (isReturnArg)
							{
								_pData->_functionData[iID].order.push_back(L"RetVar");
								_pData->_functionData[iID].returnValueType = RetVarType::Return;
							}
							else
								_pData->_functionData[iID].order.push_back(strm.str());

							std::wstring pardef, pardefDesc, constantGroup;
							bool scriptCheck = false;
							for (rapidxml::xml_attribute<wchar_t>* attr = argChildNode->first_attribute(); attr; attr = attr->next_attribute())
							{
								std::wstring name = attr->name();
								if (name == L"pardef")
									pardef = attr->value();
								else if (name == L"description")
									pardefDesc = attr->value();
								else if (name == L"constantgroup")
									constantGroup = attr->value();
								else if (name == L"scriptcheck")
									scriptCheck = (std::wstring(attr->value()) == L"true");
							}

							if (pardef.empty())
								throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, missing pardef value for argument ", std::to_wstring(_pData->_functionData[iID].arguments.size()), L" - Function: ", _pData->_functionData[iID].name)).c_str());
							else
							{
								ConstGroup* constGroup = NULL;
								if (!constantGroup.empty())
								{
									auto fItr = _pData->_constantGroups.find(constantGroup);
									if (fItr != _pData->_constantGroups.end())
										constGroup = &fItr->second;
									else
										throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, invalid constant group '", constantGroup, L"' for argument ", std::to_wstring(_pData->_functionData[iID].arguments.size()), L" - Function: ", _pData->_functionData[iID].name)).c_str());
								}

								ParDef pd = _convertParDef(pardef);
								if (pd == ParDef::Unknown)
									throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, invalid pardef '", pardef, L"' for argument ", std::to_wstring(_pData->_functionData[iID].arguments.size()), L" - Function: ", _pData->_functionData[iID].name)).c_str());

								_pData->_functionData[iID].arguments.push_back({ pd, pardefDesc, constGroup, scriptCheck });
								if (isReturnArg)
								{
									_pData->_functionData[iID].returnArgument = static_cast<unsigned int>(_pData->_functionData[iID].arguments.size());
									for (rapidxml::xml_node<wchar_t>* retVarChildNode = argChildNode->first_node(L"ReturnValue"); retVarChildNode; retVarChildNode = retVarChildNode->next_sibling(L"ReturnValue"))
										_pData->_functionData[iID].returnValue.insert(_convertDataType(retVarChildNode->value(), Utils::CombineStrings(L"Function: ", code)));
								}
							}
						}
						else if (name == L"RefObject")
						{
							hasRefObj = true;
							_pData->_functionData[iID].order.push_back(L"RefObj");
							for (rapidxml::xml_node<wchar_t>* refObjChildNode = argChildNode->first_node(L"RefObjectType"); refObjChildNode; refObjChildNode = refObjChildNode->next_sibling(L"RefObjectType"))
								_pData->_functionData[iID].refObjType.insert(_convertDataType(refObjChildNode->value(), Utils::CombineStrings(L"Function: ", code)));
							if (_pData->_functionData[iID].refObjType.empty())
								_pData->_functionData[iID].refObjType.insert(DataTypes::Unknown);
						}
					}
				}

				_checkConstant(code, L"Functions");

				if (object == L"true" || hasRefObj)
				{
					if (_pData->_objectFunctions.find(code) != _pData->_objectFunctions.end())
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, duplicate object function name '", code, L"'")).c_str());

					_pData->_objectFunctions[code] = iID;
					// if they have a nul
					if (_pData->_functionData[iID].refObjType.find(DataTypes::Null) != _pData->_functionData[iID].refObjType.end())
					{
						if (_pData->_globalFunctions.find(code) != _pData->_globalFunctions.end())
							throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, duplicate global function name '", code, L"'")).c_str());
						_pData->_globalFunctions[code] = iID;
					}
				}
				else
				{
					if (_pData->_globalFunctions.find(code) != _pData->_globalFunctions.end())
						throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Functions, duplicate global function name '", code, L"'")).c_str());
					_pData->_globalFunctions[code] = iID;
				}
			}
		}
	}
}

void ScriptDataReader::_readCommands(rapidxml::xml_node<wchar_t>* root_node, int language, void* textDB)
{
	// Unified text lookup — uses VFS when loaded, otherwise falls back to TextDB
	auto getText = [&](int page, int id) -> std::wstring {
		if (vfsIsLoaded()) return vfsFindText(language, page, id);
		return L""; // fallback without VFS: text not available
		};
	rapidxml::xml_node<wchar_t>* mainNode = root_node->first_node(L"Commands");
	if (mainNode)
	{
		for (rapidxml::xml_node<wchar_t>* node = mainNode->first_node(L"CommandList"); node; node = node->next_sibling(L"CommandList"))
		{
			std::wstring id, code, text, sShort, desc, sDT;
			for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring name = attr->name();
				if (name == L"id")
					id = attr->value();
				else if (name == L"code")
					code = attr->value();
				else if (name == L"datatype")
					sDT = attr->value();
				else if (name == L"name")
					text = attr->value();
				else if (name == L"description")
					desc = attr->value();
				else if (name == L"short")
					sShort = attr->value();
			}

			if (id.empty())
				throw std::exception(Utils::ws2s(L"XML Read Error, Commands, missing id entry").c_str());

			DataTypes dt = sDT.empty() ? DataTypes::Unknown : _convertDataType(sDT, Utils::CombineStrings(L"Commands: ", id));

			_pData->_commands[dt] = { id, dt, L"" };

			rapidxml::xml_node<wchar_t>* rangeNode = node->first_node(L"Ranges");
			if (rangeNode)
			{
				for (rapidxml::xml_node<wchar_t>* node = rangeNode->first_node(L"Range"); node; node = node->next_sibling(L"Range"))
				{
					std::wstring start, end;
					for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
					{
						std::wstring name = attr->name();
						if (name == L"start")
							start = attr->value();
						else if (name == L"end")
							end = attr->value();
					}

					try
					{
						unsigned int iStart = std::stoi(start);
						unsigned int iEnd = std::stoi(end);
						unsigned int iCode = std::stoi(code);
						unsigned int iText = std::stoi(text);
						unsigned int iShort = std::stoi(sShort);
						unsigned int iDesc = std::stoi(desc);

						for (unsigned int i = iStart; i <= iEnd; i++)
						{
							std::wstring str = getText(iCode, i);
							if (!str.empty())
							{
								_pData->_commands[dt].data[i] = { str, getText(iText, i), getText(iDesc, i), getText(iShort, i), i };
								_pData->_commands[dt].list[str] = i;
								_pData->_constData[str] = ConstantData(dt, str, i);
							}
						}
					}
					catch (std::exception)
					{

					}
				}
			}
		}
	}
}

void ScriptDataReader::_readWareTypes(rapidxml::xml_node<wchar_t>* root_node, int language, void* textDB)
{
	auto getText = [&](int page, int id) -> std::wstring {
		if (vfsIsLoaded()) return vfsFindText(language, page, id);
		return L""; // fallback without VFS: text not available
		};
	rapidxml::xml_node<wchar_t>* mainNode = root_node->first_node(L"WareTypes");
	if (mainNode)
	{
		for (rapidxml::xml_node<wchar_t>* node = mainNode->first_node(L"WareType"); node; node = node->next_sibling(L"WareType"))
		{
			std::wstring text, file;
			unsigned int id = 0, textpos = 0;
			for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring name = attr->name();
				if (name == L"name")
					text = attr->value();
				else if (name == L"id")
					id = std::stoi(attr->value());
				else if (name == L"textpos")
					textpos = std::stoi(attr->value());
				else if (name == L"file")
					file = attr->value();
			}

			if (!text.empty() && !file.empty() && id)
			{
				std::vector<std::wstring> list;
				std::vector<unsigned int> textlist;

				std::wstringstream strm;
				strm << "Data\\" << file << L".txt";

				if (_extractTypesFile(strm.str(), list, textpos, textlist))
				{
					for (size_t i = 0; i < list.size(); i++)
					{
						unsigned int wareid = TYPECODE(id, static_cast<unsigned int>(i));
						std::wstring text, desc;

						if (textpos)
						{
							text = getText(17, textlist[i]);
							desc = getText(17, textlist[i] + 1);
						}

						_pData->_wareTypesData[wareid] = { list[i], text, desc, wareid, id, static_cast<unsigned int>(i) };
						_pData->_wareTypes[list[i]] = wareid;
						_pData->_constData[list[i]] = ConstantData(DataTypes::Ware, list[i], wareid);
					}
				}
			}
		}

	}
}

void ScriptDataReader::_readRaces(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* mainNode = root_node->first_node(L"Races");
	if (mainNode)
	{
		for (rapidxml::xml_node<wchar_t>* node = mainNode->first_node(L"Race"); node; node = node->next_sibling(L"Race"))
		{
			std::wstring id, code, name2, desc;
			for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring name = attr->name();
				if (name == L"name")
					name2 = attr->value();
				else if (name == L"code")
					code = attr->value();
				else if (name == L"description")
					desc = attr->value();
				else if (name == L"id")
					id = attr->value();
			}

			if (id.empty())
				throw std::exception(Utils::ws2s(L"XML Read Error, Races, Missing id entry").c_str());
			else if (code.empty())
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Races, Missing code entry, id=", id)).c_str());

			_checkConstant(code, L"Races");

			unsigned int iID = std::stoi(id);
			_pData->_raceData[iID] = RaceData();
			_pData->_raceData[iID].code = code;
			_pData->_raceData[iID].desc = desc;
			_pData->_raceData[iID].name = name2;
			_pData->_raceData[iID].id = iID;
			_pData->_races[code] = iID;
			_pData->_constData[code] = ConstantData(DataTypes::Race, code, iID);
		}
	}
}

void ScriptDataReader::_readProperties(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* mainNode = root_node->first_node(L"Properties");
	if (mainNode)
	{
		for (rapidxml::xml_node<wchar_t>* node = mainNode->first_node(L"Property"); node; node = node->next_sibling(L"Property"))
		{
			std::wstring name, desc;
			unsigned int getter = 0, setter = 0;
			for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring attName = attr->name();
				if (attName == L"name")
					name = attr->value();
				else if (attName == L"description")
					desc = attr->value();
				else if (attName == L"getter")
				{
					try
					{
						getter = std::stoi(attr->value());
					}
					catch (std::exception)
					{
						getter = 0;
					}
				}
				else if (attName == L"setter")
				{
					try
					{
						setter = std::stoi(attr->value());
					}
					catch (std::exception)
					{
						setter = 0;
					}
				}
			}

			if (name.empty())
				throw std::exception(Utils::ws2s(L"XML Read Error, Properties, Missing name entry").c_str());
			if (!getter && !setter)
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Properties, ", name, L": Invalid setter/getter entry")).c_str());

			_pData->_objectProperties[name] = { getter, setter, desc };
		}

	}
}
void ScriptDataReader::_readCustomEntries(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* mainNode = root_node->first_node(L"CustomEntries");
	if (mainNode)
	{
		for (rapidxml::xml_node<wchar_t>* node = mainNode->first_node(L"CustomEntry"); node; node = node->next_sibling(L"CustomEntry"))
		{
			CustomData data;
			std::wstring dt;
			for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				std::wstring name = attr->name();
				if (name == L"name")
					data.name = attr->value();
				else if (name == L"description")
					data.desc = attr->value();
				else if (name == L"datatype")
					dt = attr->value();
				else if (name == L"isstring")
					data.isStringData = _parseBoolean(attr->value());
			}
			if (!dt.empty())
				data.datatype = _convertDataType(dt, Utils::CombineStrings(L"CustomEntry: ", data.name));
			_pData->_customData[data.datatype] = data;
			_readCustomEntry(node, &_pData->_customData[data.datatype]);
		}
	}
}

void ScriptDataReader::_readCustomEntry(rapidxml::xml_node<wchar_t>* root_node, CustomData* customData)
{
	for (rapidxml::xml_node<wchar_t>* node = root_node->first_node(L"Entry"); node; node = node->next_sibling(L"Entry"))
	{
		CustomDataEntry entry;
		for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::wstring name = attr->name();
			if (name == L"code")
				entry.code = attr->value();
			else if (name == L"description")
				entry.desc = attr->value();
			else if (name == L"id")
			{
				if (customData->isStringData)
				{
					entry.intID = 0;
					entry.strID = attr->value();
				}
				else
				{
					entry.intID = std::stoi(attr->value());
					entry.strID = attr->value();
				}
			}
		}

		if (_pData->_constData.find(entry.code) != _pData->_constData.end())
			throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Duplicate constant found '", entry.code, L"' - Custom Group: ", customData->name)).c_str());

		customData->entries[entry.code] = entry;
		unsigned int keyID = 0;
		if (customData->isStringData)
		{
			if (customData->strLookup.find(entry.strID) != customData->strLookup.end())
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Duplicate custom entry id found '", entry.strID, L"' - Custom Entry: ", customData->name, L" - ", entry.code)).c_str());

			keyID = static_cast<unsigned int>(customData->strKeys.size());
			customData->strKeys.push_back(entry.strID);
			customData->strLookup[entry.strID] = entry.code;
		}
		else
		{
			if (customData->intLookup.find(entry.intID) != customData->intLookup.end())
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Duplicate custom entry id found '", std::to_wstring(entry.intID), L"' - Custom Entry: ", customData->name, L" - ", entry.code)).c_str());

			keyID = static_cast<unsigned int>(customData->intKeys.size());
			customData->intKeys.push_back(entry.intID);
			customData->intLookup[entry.intID] = entry.code;
		}
		customData->keyMap[entry.code] = keyID;
		_pData->_constData[entry.code] = ConstantData(DataTypes::Custom, entry.code, keyID, customData->datatype);
		_pData->_constData[entry.code].setStrData(entry.strID);
	}
}

DataTypes ScriptDataReader::_convertDataType(const std::wstring& type, const std::wstring& extraData)
{
	if (type == L"0")
		return DataTypes::Null;

	bool isDigit = false;
	for (size_t i = 0; i < type.length(); i++)
	{
		if (std::isdigit(type[i]))
			isDigit = true;
		else
		{
			isDigit = false;
			break;
		}
	}

	if (isDigit)
	{
		int num = std::stoi(type);
		if (num)
		{
			if (_pData->_dataTypesData.find(num) == _pData->_dataTypesData.end())
			{
				if (extraData.empty())
					throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Invalid datatype id '", type, L"'")).c_str());
				throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Invalid datatype id '", type, L"' (", extraData, L")")).c_str());
			}
			return static_cast<DataTypes>(num);
		}
	}

	auto findItr = _pData->_dataTypes.find(type);
	if (findItr != _pData->_dataTypes.end())
		return static_cast<DataTypes>(findItr->second);

	if (extraData.empty())
		throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Invalid datatype '", type, L"'")).c_str());
	throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, Invalid datatype '", type, L"' (", extraData, L")")).c_str());
}

ParDef ScriptDataReader::_convertParDef(const std::wstring& type)
{
	if (type == L"0")
		return ParDef::Var;

	bool isDigit = false;
	for (size_t i = 0; i < type.length(); i++)
	{
		if (std::isdigit(type[i]))
			isDigit = true;
		else
		{
			isDigit = false;
			break;
		}
	}

	if (isDigit)
	{
		int num = std::stoi(type);
		if (num)
			return static_cast<ParDef>(num);
	}

	auto findItr = _pData->_pardefs.find(type);
	if (findItr != _pData->_pardefs.end())
		return static_cast<ParDef>(findItr->second);

	return ParDef::Unknown;
}

bool ScriptDataReader::_extractTypesFile(const std::wstring& file, std::vector<std::wstring>& list, unsigned int textpos, std::vector<unsigned int>& textList)
{
	// When VFS is loaded, extract the types file directly from the game archives.
	// Game types files live at types\<name>.txt in the VFS.
	// The local path passed in is Data\<name>.txt — extract the filename part.
	std::wstring actualPath = file;
	std::wstring tempFile;

	if (vfsIsLoaded())
	{
		// Extract the base filename from the local path (e.g. "Data\TShips.txt" -> "TShips.txt")
		std::wstring baseName = file;
		size_t slash = file.find_last_of(L"\\/");
		if (slash != std::wstring::npos)
			baseName = file.substr(slash + 1);

		std::wstring vfsPath = L"types\\" + baseName;
		tempFile = L"_temp_types_" + baseName;
		std::wstring extracted = vfsExtractFile(vfsPath, tempFile);
		if (!extracted.empty())
			actualPath = extracted;
		else
			return false; // file not in VFS
	}

	std::wifstream inFile(actualPath);
	if (!inFile.good())
	{
		if (!tempFile.empty()) _wremove(tempFile.c_str());
		return false;
	}

	size_t count = 0;
	bool firstLine = true;
	std::wstring line;
	while (std::getline(inFile, line))
	{
		if (line.empty())
			continue;

		auto pos = line.find_first_not_of(' ', 0);
		if (pos == std::wstring::npos)
			continue;

		line = line.substr(pos, line.length() - pos);

		// ignore comments
		if (line.substr(0, 2) == L"//")
			continue;

		std::vector<std::wstring> split = Utils::splitString(line, L";");
		if (split.size() > 1)
		{
			if (firstLine)
			{
				firstLine = false;
				try
				{
					unsigned int count = std::stoi(split[1]);
					if (count)
					{
						list.resize(count);
						if (textpos)
							textList.resize(count);
					}
				}
				catch (std::exception)
				{

				}
			}
			else
			{
				if (count >= list.size())
					list.resize(count + 1);
				list[count] = split.back();

				if (textpos)
				{
					try
					{
						if (split.size() > textpos)
						{
							unsigned int textid = std::stoi(split[textpos]);
							if (count >= textList.size())
								textList.resize(count + 1);
							textList[count] = textid;
						}
					}
					catch (std::exception)
					{

					}
				}

				++count;
			}
		}
	}

	inFile.close();
	if (!tempFile.empty())
		_wremove(tempFile.c_str());

	return true;
}

bool ScriptDataReader::_parseBoolean(const std::wstring& str) const
{
	if (str == L"true")
		return true;
	if (str == L"yes")
		return true;
	return false;
}

void ScriptDataReader::_checkConstant(const std::wstring& str, const std::wstring& section, bool checkList) const
{
	// check for invalid 
	if (str.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_.") != std::string::npos)
		throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, ", section, L", Invalid characters found in constant '", str, L"'")).c_str());

	if (checkList)
	{
		if (_pData->_constData.find(str) != _pData->_constData.end())
			throw std::exception(Utils::ws2s(Utils::CombineStrings(L"XML Read Error, ", section, L", Duplicate constant found '", str, L"'")).c_str());
	}
}

void ScriptDataReader::_readMacros(rapidxml::xml_node<wchar_t>* root_node)
{
	rapidxml::xml_node<wchar_t>* macrosNode = root_node->first_node(L"Macros");
	if (!macrosNode)
		return;

	for (rapidxml::xml_node<wchar_t>* macroNode = macrosNode->first_node(L"Macro");
		macroNode; macroNode = macroNode->next_sibling(L"Macro"))
	{
		MacroData macro;
		macro.hasBlock = false;

		// Read name attribute
		rapidxml::xml_attribute<wchar_t>* nameAttr = macroNode->first_attribute(L"name");
		if (!nameAttr)
			continue;
		macro.name = nameAttr->value();

		// Read arguments
		rapidxml::xml_node<wchar_t>* argsNode = macroNode->first_node(L"Arguments");
		if (argsNode)
		{
			for (rapidxml::xml_node<wchar_t>* argNode = argsNode->first_node(L"Argument");
				argNode; argNode = argNode->next_sibling(L"Argument"))
			{
				rapidxml::xml_attribute<wchar_t>* nameA = argNode->first_attribute(L"name");
				macro.argNames.push_back(nameA ? std::wstring(nameA->value()) : L"");
			}
		}

		// Read routine
		rapidxml::xml_node<wchar_t>* routineNode = macroNode->first_node(L"Routine");
		if (routineNode)
		{
			for (rapidxml::xml_node<wchar_t>* child = routineNode->first_node();
				child; child = child->next_sibling())
			{
				std::wstring nodeName = child->name();
				MacroRoutineLine line;

				if (nodeName == L"Expression")
				{
					line.type = MacroRoutineLine::Type::Expression;
					rapidxml::xml_attribute<wchar_t>* valAttr = child->first_attribute(L"value");
					line.text = valAttr ? std::wstring(valAttr->value()) : L"";

					for (rapidxml::xml_node<wchar_t>* fa = child->first_node(L"FunctionArgument");
						fa; fa = fa->next_sibling(L"FunctionArgument"))
					{
						MacroRoutineLine::FuncArg funcArg;
						funcArg.funcId = 0;
						funcArg.argPos = -1;
						rapidxml::xml_attribute<wchar_t>* idAttr = fa->first_attribute(L"id");
						if (idAttr)
							funcArg.funcId = static_cast<unsigned int>(std::stoi(std::wstring(idAttr->value())));
						rapidxml::xml_node<wchar_t>* passedNode = fa->first_node(L"PassedArgument");
						if (passedNode)
						{
							rapidxml::xml_attribute<wchar_t>* posAttr = passedNode->first_attribute(L"pos");
							if (posAttr)
								funcArg.argPos = std::stoi(std::wstring(posAttr->value()));
						}
						line.funcArgs.push_back(funcArg);
					}
					macro.routine.push_back(line);
				}
				else if (nodeName == L"Block")
				{
					MacroRoutineLine startLine;
					startLine.type = MacroRoutineLine::Type::StartBlock;
					macro.routine.push_back(startLine);

					for (rapidxml::xml_node<wchar_t>* blockChild = child->first_node();
						blockChild; blockChild = blockChild->next_sibling())
					{
						std::wstring blockNodeName = blockChild->name();
						MacroRoutineLine bline;
						if (blockNodeName == L"Expression")
						{
							bline.type = MacroRoutineLine::Type::Expression;
							rapidxml::xml_attribute<wchar_t>* valAttr = blockChild->first_attribute(L"value");
							bline.text = valAttr ? std::wstring(valAttr->value()) : L"";

							for (rapidxml::xml_node<wchar_t>* fa = blockChild->first_node(L"FunctionArgument");
								fa; fa = fa->next_sibling(L"FunctionArgument"))
							{
								MacroRoutineLine::FuncArg funcArg;
								funcArg.funcId = 0;
								funcArg.argPos = -1;
								rapidxml::xml_attribute<wchar_t>* idAttr = fa->first_attribute(L"id");
								if (idAttr)
									funcArg.funcId = static_cast<unsigned int>(std::stoi(std::wstring(idAttr->value())));
								rapidxml::xml_node<wchar_t>* passedNode = fa->first_node(L"PassedArgument");
								if (passedNode)
								{
									rapidxml::xml_attribute<wchar_t>* posAttr = passedNode->first_attribute(L"pos");
									if (posAttr)
										funcArg.argPos = std::stoi(std::wstring(posAttr->value()));
								}
								bline.funcArgs.push_back(funcArg);
							}
							macro.routine.push_back(bline);
						}
						else if (blockNodeName == L"BlockCommands")
						{
							bline.type = MacroRoutineLine::Type::BlockCommands;
							macro.routine.push_back(bline);
							macro.hasBlock = true;
						}
					}

					MacroRoutineLine endLine;
					endLine.type = MacroRoutineLine::Type::EndBlock;
					macro.routine.push_back(endLine);
				}
				else if (nodeName == L"BlockCommands")
				{
					line.type = MacroRoutineLine::Type::BlockCommands;
					macro.routine.push_back(line);
					macro.hasBlock = true;
				}
			}
		}

		_pData->_macros[macro.name] = macro;
	}
}