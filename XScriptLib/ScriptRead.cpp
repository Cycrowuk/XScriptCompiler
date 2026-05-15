#include "pch.h"
#include "ScriptRead.h"
#include "Utils.h"
#include "CScriptData.h"

#include <sstream>
#include "ParseOperator.h"

#include "../XLib/XLib.h"

using namespace XScript;

ScriptRead::ScriptRead(CScriptData *data) : _pData(data),
	_version(0),
	_engine(0),
	_command(0),
	_inserted(0)
{

}

ScriptRead::~ScriptRead()
{

}

bool ScriptRead::read(const std::wstring& filename)
{
	// check for pck files
	XLib::FileIO f(filename);
	if (!f.exists())
		throw std::exception("Unable to open file for reading");

	XLib::String unpackedData;
	std::vector<wchar_t>* buffer = NULL;

	if(f.isFileExtension("pck"))
	{
		size_t fileSize;
		char* fileData = f.readAll(&fileSize);
		if (!fileData || !fileSize)
		{
			if (fileData)
				delete[]fileData;
			throw std::exception("Unable to open file for reading");
		}

		size_t unpackedSize;
		unsigned char *unpacked = XLib::UnPCKData((unsigned char *)fileData, fileSize, &unpackedSize);
		if (!unpackedSize || !unpacked)
		{
			delete[]fileData;
			if (unpacked)
				delete[]unpacked;
			throw std::exception("Unable to unpack file");
		}

		unpackedData = unpacked;
		delete[]unpacked;
		delete[]fileData;
	}
	f.close();


	rapidxml::xml_document<wchar_t>* doc = new rapidxml::xml_document<wchar_t>();
	if (!unpackedData.empty())
		doc->parse<0>(&unpackedData[0]);
	else
	{
		std::wifstream in(filename);
		if (in.bad())
			throw std::exception("Unable to open file for reading");

		buffer = new std::vector<wchar_t>((std::istreambuf_iterator<wchar_t>(in)), std::istreambuf_iterator<wchar_t>());
		buffer->push_back('\0');
		doc->parse<0>(&(*buffer)[0]);
	}

	rapidxml::xml_node<wchar_t>* root_node = doc->first_node(L"script");
	if (!root_node)
	{
		if(buffer)
			delete buffer;
		delete doc;
		throw std::exception("No <script> node found in file");
	}

	rapidxml::xml_node<wchar_t>* code_node = root_node->first_node(L"codearray");
	if (!code_node)
	{
		if (buffer)
			delete buffer;
		delete doc;
		throw std::exception("No <codearray> node found in file");
	}

	rapidxml::xml_node<wchar_t>* main_node = code_node->first_node(L"sval");
	if (!main_node)
	{
		if (buffer)
			delete buffer;
		delete doc;
		throw std::exception("Invalid codearray found in file");
	}

	bool isArray = false;
	unsigned int arraySize = 0;
	// check to make sure the item is valid, it should be array of 10 items
	for (rapidxml::xml_attribute<wchar_t>* attr = main_node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::wstring name = attr->name();
		std::wstring val = attr->value();
		if (name == L"type" && val == L"array")
			isArray = true;
		else if (name == L"size")
			arraySize = std::stoi(val);
	}

	if(!isArray || arraySize != 10)
	{
		if (buffer)
			delete buffer;
		delete doc;
		throw std::exception("Invalid codearray found in file");
	}

	unsigned int codePos = 0;
	for (rapidxml::xml_node<wchar_t>* node = main_node->first_node(); node; node = node->next_sibling(), ++codePos)
	{
		std::wstring name = node->name();
		if (name == L"sval")
		{
			bool success = true;
			switch (codePos)
			{
			case 0:
				success = _readName(node);
				break;
			case 1:
				success = _readEngine(node);
				break;
			case 2:
				success = _readDesc(node);
				break;
			case 3:
				success = _readVersion(node);
				break;
			case 5:
				success = _readVariables(node);
				break;
			case 6:
				success = _readCode(node);
				break;
			case 7:
				success = _readArguments(node);
				break;
			case 8:
				success = _readHiddenItems(node);
				break;
			case 9:
				success = _readCommandType(node);
				break;
			}
			if (!success)
				break;
		}
	}

//	in.close();

	if (buffer)
		delete buffer;
	delete doc;
	return true;
}

bool ScriptRead::write(const std::wstring& outfile)
{
	std::wofstream out(outfile);
	if (out.bad())
		throw std::exception("Unable to open file for writing");

	unsigned int i = 0;
	for (auto itr = _arguments.begin(); itr != _arguments.end(); itr++, i++)
	{
		auto data = _pData->getParDefData(itr->type);
		if (data)
			out << L"SetArgument($" << _variables[i] << ", \"" << itr->desc << "\", " << data->code << ");" << std::endl;
	}

	if (_version)
		out << L"SetVersion(" << _version << ");" << std::endl;
	if (!_desc.empty())
		out << L"SetDescription(\"" << _desc << "\");" << std::endl;

	if (_command)
	{
		auto cmd = _pData->getCommand(DataTypes::ObjectCommand, _command);
		if (cmd)
			out << L"SetCommand(" << cmd->id << ");" << std::endl;
	}

	out << std::endl;

	unsigned int indent = 0;	
	bool isIndentNonBlock = false;
	int debug = 0;
	for (auto itr = _commands.begin(); itr != _commands.end(); itr++, debug++)
	{
		if (itr->isBlankLine)
		{
			out << std::endl;
			continue;
		}
		if (!itr->comment.empty())
		{
			out << "// " << itr->comment << std::endl;
			continue;
		}

		bool isSpecial = itr->data ? (_pData->findSpecialKeyword(itr->data->name) == itr->data->id) : false;

		if (itr->data && itr->data->id == _pData->endCommand())
		{
			if (indent > 0)
				--indent;
			for (unsigned int i = 0; i < indent; i++)
				out << "   ";
			out << "}" << std::endl;
			continue;
		}
		else if (itr->data && itr->data->id == _pData->elseCommand())
		{
			unsigned int doIndent = (indent > 0) ? (indent - 1) : 0;
			for (unsigned int i = 0; i < (indent - 1); i++)
				out << "   ";
			out << "}" << std::endl;
			for (unsigned int i = 0; i < (indent - 1); i++)
				out << "   ";
			out << "else" << std::endl;
			for (unsigned int i = 0; i < (indent - 1); i++)
				out << "   ";
			out << "{" << std::endl;
			continue;
		}
		else if (itr->data && itr->data->id == _pData->hiddenGotoCommand())
			continue;

		else if (itr->data && itr->data->id == _pData->defineLabelCommand())
		{
			out << itr->arguments.front() << ":" << std::endl;
			continue;
		}

		if (itr->isElseCondition)
		{
			if (indent > 0)
				--indent;
			for (unsigned int i = 0; i < indent; i++)
				out << "   ";
			out << "}" << std::endl;
		}

		for (unsigned int i = 0; i < indent; i++)
			out << "   ";

		// remove indent if not a block
		if (isIndentNonBlock)
		{
			isIndentNonBlock = false;
			--indent;
		}

		bool isCondition = false;
		if (!itr->condition.empty())
		{
			isCondition = true;
			out << itr->condition;
			++indent;
			if (!itr->isBlock)
				isIndentNonBlock = true;
			out << "(";
		}
		else if (!itr->retvar.empty())
		{
			if (!itr->data || !itr->data->returnArgument)
				out << itr->retvar;
		}

		// special function handling
		if (itr->data && itr->data == _pData->getSpecialGlobalFunction(SpecialFunction::GetArray))
		{
			out << itr->arguments.front() << "[" << itr->arguments[1] << "]";
			isSpecial = true;
		}
		else if (itr->data && itr->data == _pData->getSpecialGlobalFunction(SpecialFunction::SetArray))
		{
			out << itr->arguments.front() << "[" << itr->arguments[1] << "] = " << itr->arguments[2];
			isSpecial = true;
		}
		else
		{
			if (!itr->isExpression)
			{
				if (!itr->refobj.empty())
					out << itr->refobj << "->";

				out << itr->data->name;
				if (isSpecial)
					out << L" ";
				else
					out << L"(";
			}

			if (itr->data && (itr->data->id == _pData->gosubCommand() || itr->data->id == _pData->gotoCommand()))
			{
				auto findItr = _labels.find(itr->arguments.front());
				if (findItr == _labels.end())
					throw std::exception("Missing label entry");
				
				out << findItr->second;
			}
			else
			{
				// first argument
				bool firstArg = true;
				if (itr->data && itr->data->returnArgument > 0)
				{
					out << itr->retvar.substr(0, itr->retvar.length() - 3);
					firstArg = false;
				}

				for (auto aItr = itr->arguments.begin(); aItr != itr->arguments.end(); aItr++)
				{
					if (!firstArg && !itr->isExpression)
						out << ", ";
					firstArg = false;

					out << *aItr;
				}
			}
		}

		if (isCondition && (itr->isExpression || isSpecial))
			out << L")";
		else if (isCondition)
			out << L"))";
		else if (isSpecial || itr->isExpression)
			out << L";";
		else
			out << ");";
		out << std::endl;

		if (itr->isBlock && !isIndentNonBlock)
		{
			for (unsigned int i = 0; i < (indent - 1); i++)
				out << "   ";
			out << "{" << std::endl;
		}
	}

	return true;
}

void ScriptRead::_readSval(rapidxml::xml_node<wchar_t>* node, std::wstring& type, std::wstring& value)
{
	for (rapidxml::xml_attribute<wchar_t>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::wstring name = attr->name();
		std::wstring val = attr->value();
		if (name == L"type")
			type = val;
		else if (name == L"size")
			value = val;
		else if (name == L"val")
			value = val;
	}
}

bool ScriptRead::_readName(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type != L"string")
		throw std::exception("Invalid value type for script name");

	_name = value;
	return true;
}
bool ScriptRead::_readDesc(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type != L"string")
		throw std::exception("Invalid value type for script description");

	_desc = value;
	return true;
}
bool ScriptRead::_readVersion(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type != L"int")
		throw std::exception("Invalid value type for script version");

	_version = std::stoi(value);
	return true;
}
bool ScriptRead::_readEngine(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type != L"int")
		throw std::exception("Invalid value type for script engine");

	_engine = std::stoi(value);
	return true;
}

bool ScriptRead::_readVariables(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		_variables.reserve(std::stoi(value));
		for (rapidxml::xml_node<wchar_t>* n = node->first_node(); n; n = n->next_sibling())
		{
			if (std::wstring(n->name()) == L"sval")
			{
				std::wstring type, value;
				_readSval(n, type, value);

				if (type == L"string")
					_variables.push_back(value);
			}
		}
	}

	return true;
}

bool ScriptRead::_readCode(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		unsigned int commands = std::stoi(value);
		for (rapidxml::xml_node<wchar_t>* n = node->first_node(); n; n = n->next_sibling())
		{
			if (std::wstring(n->name()) == L"sval")
			{
				if (!_readCommand(n))
					throw std::exception("Unable to read command");
			}
		}
	}

	// find all label references
	for(unsigned int i = 0; i < _commands.size(); i++)
	{		
		if (_commands[i].data && _commands[i].data->id == _pData->defineLabelCommand())
			_labels[std::to_wstring(i)] = _commands[i].arguments.front();
	}

	return true;
}

std::wstring _convertOperator(Operators oper)
{
	switch (oper)
	{
	case Operators::Add:
		return L" + ";
	case Operators::OpenBracket:
		return L"(";
	case Operators::CloseBracket:
		return L")";
	case Operators::And:
		return L" & ";
	case Operators::BoolAnd:
		return L" && ";
	case Operators::BoolNot:
		return L"!";
	case Operators::BoolOr:
		return L" || ";
	case Operators::Divide:
		return L" / ";
	case Operators::Equals:
		return L" == ";
	case Operators::Greater:
		return L" > ";
	case Operators::GreaterEquals:
		return L" >= ";
	case Operators::Lesser:
		return L" < ";
	case Operators::LesserEquals:
		return L" <= ";
	case Operators::Modulus:
		return L" % ";
	case Operators::Multiple:
		return L" * ";
	case Operators::Negate:
		return L"-";
	case Operators::Not:
		return L"~";
	case Operators::NotEquals:
		return L" != ";
	case Operators::Or:
		return L" | ";
	case Operators::Subtract:
		return L" - ";
	case Operators::Xor:
		return L" ^ ";
	}

	throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Unknown operator value '", std::to_wstring(static_cast<int>(oper)), L"'")).c_str());
}

bool ScriptRead::_readCommand(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		unsigned int commandID = 0;
		FunctionData* currentFunction = NULL;
		unsigned int currentArg = 0;
		int argType = -1;
		unsigned int expressionSize = 0;
		bool readingInfixExpression = false;
		bool readingPosfixExpression = false;
		std::vector<std::wstring> infixExpression;
		int extraArgs = 0;

		unsigned int items = std::stoi(value);
		for (rapidxml::xml_node<wchar_t>* n = node->first_node(); n; n = n->next_sibling())
		{
			std::wstring type, value;
			_readSval(n, type, value);

			if (!currentFunction)
			{
				if (type == L"int")
				{
					commandID = std::stoi(value);
					if (commandID == _pData->expressionCommand())
					{
						_commands.push_back({});
						currentFunction = &_commands.back();
						currentFunction->isBlock = false;
						currentFunction->isElseCondition = false;
						currentFunction->isExpression = true;
						currentFunction->isBlankLine = false;
						currentArg = 0;
						extraArgs = 0;
						argType = -1;
					}
					else
					{
						const Function* funcData = _pData->getFunction(commandID);
						if (!funcData || funcData->name.empty())
							throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Unable to find command '", value, L"'")).c_str());

						_commands.push_back({});
						currentFunction = &_commands.back();
						currentFunction->data = funcData;
						currentFunction->isBlock = false;
						currentFunction->isElseCondition = false;
						currentFunction->isExpression = false;
						currentFunction->isBlankLine = false;
						currentArg = 0;
						extraArgs = 0;
						argType = -1;
					}
				}
				else
					return false;
			}
			else if (commandID == _pData->expressionCommand())
			{
				if (readingInfixExpression && readingPosfixExpression)
				{
					expressionSize = std::stoi(value);
					readingInfixExpression = false;
				}
				else if (readingPosfixExpression)
				{
					expressionSize--;
					int oper = std::stoi(value);
					if (oper < 0)
					{
						unsigned int pos = static_cast<unsigned int>(-1 - oper);
						currentFunction->arguments.push_back(infixExpression[pos]);
					}
					else
						currentFunction->arguments.push_back(_convertOperator(static_cast<Operators>(oper)));
				}
				else if (readingInfixExpression)
				{
					if (argType < 0)
						argType = std::stoi(value);
					else
					{
						infixExpression.push_back(_parseArgument(static_cast<DataTypes>(argType), value));
						argType = -1;

						--expressionSize;
						if (expressionSize <= 0)
							readingPosfixExpression = true;
					}
				}
				else
				{
					// retvar
					if (!currentArg)
					{
						currentFunction->retvar = _parseReturnValue(value);
						currentFunction->condition = _parseCondition(value, currentFunction->isBlock, currentFunction->isElseCondition);
					}
					else if (currentArg == 1)
					{
						expressionSize = std::stoi(value);
						readingInfixExpression = true;
						currentArg = -1;
					}
				}
				++currentArg;
			}
			else if (argType < 0)
			{
				if (currentArg >= currentFunction->data->order.size())
				{
					if (currentFunction->data->undefinedCount)
					{
						if (type != L"int")
							throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid sval '", type, L"' for command")).c_str());

						if (!extraArgs)
							extraArgs = std::stoi(value);
						else
							argType = std::stoi(value);
						continue;
					}
					throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid argument entries for command '", currentFunction->data->name, L"'")).c_str());
				}

				std::wstring strArg = currentFunction->data->order[currentArg];


				// check pardef value
				if (strArg.substr(0, 3) == L"Arg")
				{
					unsigned int a = std::stoi(strArg.substr(3, strArg.length() - 3));
					ParDef pd = currentFunction->data->arguments[a].pardef;
					if (pd == ParDef::CallName)
					{
						if(type != L"string")
							throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid sval '", type, L"' for callname pardef, Expected 'string'")).c_str());

						if (currentFunction->arguments.size() <= a)
							currentFunction->arguments.resize(a + 1);
						currentFunction->arguments[a] = Utils::CombineStrings(L"\"", value, L"\"");
						++currentArg;
						continue;
					}
					else if (pd == ParDef::LabelDef)
					{
						if (type != L"string")
							throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid sval '", type, L"' for labeldef pardef, Expected 'string'")).c_str());

						if (currentFunction->arguments.size() <= a)
							currentFunction->arguments.resize(a + 1);
						currentFunction->arguments[a] = value;
						++currentArg;
						continue;
					}
					else if (pd == ParDef::Label)
					{
						if (type != L"int")
							throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid sval '", type, L"' for label command")).c_str());

						if (currentFunction->arguments.size() <= a)
							currentFunction->arguments.resize(a + 1);
						currentFunction->arguments[a] = value;
						++currentArg;
						continue;
					}
				}

				if (type != L"int")
					throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid sval '", type, L"' for command")).c_str());

				if (strArg.substr(0, 3) == L"Arg")
					argType = std::stoi(value);
				else if (strArg == L"RetVar")
				{
					currentFunction->retvar = _parseReturnValue(value);
					currentFunction->condition = _parseCondition(value, currentFunction->isBlock, currentFunction->isElseCondition);
					++currentArg;
				}
				else if(strArg == L"RefObj")
					argType = std::stoi(value);
			}
			else if (argType >= 0)
			{
				if (currentArg >= currentFunction->data->order.size())
				{
					if (currentFunction->data->undefinedCount)
					{
						currentFunction->arguments.push_back(_parseArgument(static_cast<DataTypes>(argType), value));
						argType = -1;
						++currentArg;
						continue;
					}
					throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid argument entries for command '", currentFunction->data->name, L"'")).c_str());
				}

				std::wstring strArg = currentFunction->data->order[currentArg];
				if (strArg.substr(0, 3) == L"Arg")
				{
					unsigned int a = std::stoi(strArg.substr(3, strArg.length() - 3));
					if (currentFunction->arguments.size() <= a)
						currentFunction->arguments.resize(a + 1);

					// Boolean pardef: emit TRUE/FALSE instead of raw 0/1.
					// VARBOOLEAN = pardef id 63, BOOLEAN = pardef id 64 (from x3fl.xml)
					ParDef pd = currentFunction->data->arguments[a].pardef;
					bool isBoolParDef = (pd == ParDef::Boolean || pd == ParDef::VarBoolean
						|| static_cast<int>(pd) == 63 || static_cast<int>(pd) == 64);
					if (isBoolParDef)
						currentFunction->arguments[a] = (std::stoi(value) != 0) ? L"TRUE" : L"FALSE";
					else
						currentFunction->arguments[a] = _parseArgument(static_cast<DataTypes>(argType), value);
				}
				else if (strArg == L"RefObj")
					currentFunction->refobj = _parseArgument(static_cast<DataTypes>(argType), value);
				argType = -1;
				++currentArg;
			}
		}
	}
	
	return true;
}

bool ScriptRead::_readArguments(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		unsigned int items = std::stoi(value);
		for (rapidxml::xml_node<wchar_t>* n = node->first_node(); n; n = n->next_sibling())
		{
			if (!_readArgument(n))
				return false;
		}
	}
	
	return true;
}

bool ScriptRead::_readArgument(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		unsigned int items = std::stoi(value);
		if (items == 2)
		{
			_arguments.push_back({ ParDef::Unknown, L"" });

			unsigned int i = 0;
			for (rapidxml::xml_node<wchar_t>* n = node->first_node(); n; n = n->next_sibling(), i++)
			{
				std::wstring type, value;
				_readSval(n, type, value);

				if (i == 0)
				{
					if (type == L"int")
						_arguments.back().type = static_cast<ParDef>(std::stoi(value));
				}
				else if (i == 1)
				{
					if (type == L"string")
						_arguments.back().desc = value;
				}
			}
		}
		else
			throw std::exception(Utils::ws2s(Utils::CombineStrings(L"Invalid sval argument array size '", value, L"'")).c_str());
	}
	return true;
}

bool ScriptRead::_readHiddenItems(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		for (rapidxml::xml_node<wchar_t>* n = node->first_node(); n; n = n->next_sibling())
		{
			if (!_readHiddenItem(n))
				return false;
		}
	}

	return true;
}

bool ScriptRead::_readHiddenItem(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type == L"array")
	{
		unsigned int items = std::stoi(value);
		if (items >= 2)
		{
			rapidxml::xml_node<wchar_t>* n1 = node->first_node();
			rapidxml::xml_node<wchar_t>* n2 = n1->next_sibling();

			if (n1 && n2)
			{
				std::wstring type, value;
				_readSval(n1, type, value);
				if (type == L"int")
				{
					unsigned int pos = std::stoi(value);
					_readSval(n2, type, value);
					if (type == L"int")
					{
						unsigned int cmd = std::stoi(value);
						if (cmd == 1)
						{
							if (items >= 3)
							{
								_readSval(n2->next_sibling(), type, value);
								if (type == L"string")
								{
									_commands.insert(_commands.begin() + pos + _inserted, { NULL, false, value });
									++_inserted;
								}
							}

						}
						else if (cmd == 2)
						{
							_commands.insert(_commands.begin() + pos + _inserted, { NULL, true });
							++_inserted;
						}
						else
						{
							auto data = _pData->getFunction(cmd);
							if (data && data->name.size())
							{
								_commands.insert(_commands.begin() + pos + _inserted, { data, false });
								++_inserted;
							}
						}
					}
				}
			}
		}
	}

	return true;
}


std::wstring ScriptRead::_parseArgument(DataTypes type, const std::wstring& value)
{
	std::wstringstream strm;

	switch (type)
	{
	case DataTypes::Variable:
		strm << "$" << _variables[std::stoi(value)];
		return strm.str();

	case DataTypes::String:
		strm << "\"" << value << "\"";
		return strm.str();

	case DataTypes::DataType:
		return _pData->getDataTypeCode(static_cast<DataTypes>(std::stoi(value)));

	case DataTypes::Ware:
		return _pData->getWareTypeCode(std::stoi(value));

	case DataTypes::Constant:
		return _pData->getConstantCode(std::stoi(value));

	case DataTypes::Race:
		return _pData->getRaceCode(std::stoi(value));
	case DataTypes::Null:
		return L"null";
	case DataTypes::ParDef:
	{
		const ParDefData* data = _pData->getParDefData(static_cast<ParDef>(std::stoi(value)));
		if (data)
			return data->code;
	}
	}

	auto commandList = _pData->findCommandsList(type);
	if (commandList)
	{
		auto findItr = commandList->data.find(std::stoi(value));
		if (findItr != commandList->data.end())
			return findItr->second.id;

		// Command not found in the known list — check if this DataType has a prefix.
		// If so, emit  PREFIX + id  so the script is valid for XScript even when the
		// command is from a third-party mod and not in our data file.
		const DataTypeData* dt = _pData->findDatatype(type);
		if (dt && !dt->prefix.empty())
		{
			std::wstringstream strm;
			strm << dt->prefix << value;
			return strm.str();
		}
	}

	auto customData = _pData->getCustomDatatype(type);
	if (customData)
	{
		if (customData->isStringData)
		{
			auto findItr = customData->strLookup.find(value);
			if (findItr != customData->strLookup.end())
				return findItr->second;
		}
		else
		{
			auto findItr = customData->intLookup.find(std::stoi(value));
			if (findItr != customData->intLookup.end())
				return findItr->second;
		}
	}

	return value;
}

std::wstring ScriptRead::_parseReturnValue(const std::wstring& value)
{
	int pos = std::stoi(value);
	if (pos >= 0)
	{
		std::wstringstream strm;
		strm << "$" << _variables[pos] << " = ";

		return strm.str();
	}
	else
	{
		if ((pos & _SCRIPT_VARIDX_MASK_OP) == 2)
			return L"START ";
	}

	return L"";
}
std::wstring ScriptRead::_parseCondition(const std::wstring& value, bool& isBlock, bool& isElse)
{
	isBlock = false;
	isElse = false;

	int pos = std::stoi(value);
	if (pos < 0)
	{
		switch (pos & _SCRIPT_VARIDX_MASK_OP)
		{
		case 1:	// no retvar
			return L"";
		case 2:	// START
			return L""; // ignore it
		case 3: // if
			isBlock = true;
			return L"if ";
		case 4: // if not
			isBlock = true;
			return L"if not ";
		case 5: // else if
			isBlock = true;
			isElse = true;
			return L"else if ";
		case 6: // else if not
			isBlock = true;
			isElse = true;
			return L"else if not ";
		case 7: // skip if
			return L"if not ";
		case 8: // do if
			return L"if ";
		case 9: // while
			isBlock = true;
			return L"while ";
		case 10: // while not
			isBlock = true;
			return L"while not ";
		}
	}

	return L"";
}

bool ScriptRead::_readCommandType(rapidxml::xml_node<wchar_t>* node)
{
	std::wstring type, value;
	_readSval(node, type, value);

	if (type != L"int")
		return false;

	_command = std::stoi(value);
	return true;
}
