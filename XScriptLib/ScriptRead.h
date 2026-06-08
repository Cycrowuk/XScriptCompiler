#pragma once

#include "rapidxml\rapidxml.hpp"

namespace XScript
{
	class CScriptData;
	class ScriptRead
	{
	private:
		struct Argument
		{
			ParDef type;
			std::wstring desc;
		};
		struct FunctionData
		{
			const Function* data;
			bool isBlankLine;
			std::wstring comment;
			std::wstring retvar;
			std::wstring condition;
			std::wstring refobj;
			bool isBlock;
			bool isExpression;
			bool isElseCondition;
			std::vector<std::wstring> arguments;
		};

	private:
		std::wstring _name;
		std::wstring _desc;
		unsigned int _version;
		unsigned int _command;
		unsigned int _engine;
		std::vector<std::wstring> _variables;
		std::vector<FunctionData> _commands;
		std::vector<Argument> _arguments;
		std::map<std::wstring, std::wstring> _labels;

		unsigned int _inserted;
		CScriptData* _pData;
		bool _useNamespace; // when true, emit Namespace::alias instead of bare function name

	public:
		ScriptRead(CScriptData *data);
		~ScriptRead();

		void setUseNamespace(bool use) { _useNamespace = use; }

		bool read(const std::wstring& filename);
		bool write(const std::wstring& output);

	private:
		void _readSval(rapidxml::xml_node<wchar_t>* node, std::wstring& type, std::wstring& value);
		bool _readName(rapidxml::xml_node<wchar_t>* node);
		bool _readDesc(rapidxml::xml_node<wchar_t>* node);
		bool _readVersion(rapidxml::xml_node<wchar_t>* node);
		bool _readEngine(rapidxml::xml_node<wchar_t>* node);
		bool _readVariables(rapidxml::xml_node<wchar_t>* node);
		bool _readCode(rapidxml::xml_node<wchar_t>* node);
		bool _readCommand(rapidxml::xml_node<wchar_t>* node);
		bool _readArguments(rapidxml::xml_node<wchar_t>* node);
		bool _readArgument(rapidxml::xml_node<wchar_t>* node);
		bool _readHiddenItems(rapidxml::xml_node<wchar_t>* node);
		bool _readHiddenItem(rapidxml::xml_node<wchar_t>* node);
		bool _readCommandType(rapidxml::xml_node<wchar_t>* node);

		std::wstring _parseArgument(DataTypes type, const std::wstring& value);
		std::wstring _parseReturnValue(const std::wstring& value);
		std::wstring _parseCondition(const std::wstring& value, bool& isBlock, bool& isElse);
	};

}

