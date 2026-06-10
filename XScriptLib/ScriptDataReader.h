#pragma once

#include "rapidxml\rapidxml.hpp"

namespace XLib
{
	class TextDB;
}
namespace XScript
{
	struct CustomData;
	class CScriptData;
	class ScriptDataReader
	{
	private:
		CScriptData* _pData;

	public:
		ScriptDataReader(CScriptData* data);

		bool readData(const std::wstring& filename);

	private:
		void _readGameData(rapidxml::xml_node<wchar_t>* root_node);
		void _readDataTypes(rapidxml::xml_node<wchar_t>* root_node);
		void _readParDefs(rapidxml::xml_node<wchar_t>* root_node);
		void _readConstants(rapidxml::xml_node<wchar_t>* root_node);
		void _readConstant(rapidxml::xml_node<wchar_t>* root_node, ConstGroup *group);
		void _readFunctions(rapidxml::xml_node<wchar_t>* root_node);
		void _readCommands(rapidxml::xml_node<wchar_t>* root_node, XLib::TextDB* textDB);
		void _readWareTypes(rapidxml::xml_node<wchar_t>* root_node, XLib::TextDB* textDB);
		void _readRaces(rapidxml::xml_node<wchar_t>* root_node);
		void _readProperties(rapidxml::xml_node<wchar_t>* root_node);
		void _readCustomEntries(rapidxml::xml_node<wchar_t>* root_node);
		void _readCustomEntry(rapidxml::xml_node<wchar_t>* root_node, CustomData *customData);
		void _readMacros(rapidxml::xml_node<wchar_t>* root_node);

		DataTypes _convertDataType(const std::wstring& type, const std::wstring &extraData);
		ParDef _convertParDef(const std::wstring& type);
		bool _extractTypesFile(const std::wstring& file, std::vector<std::wstring>& list, unsigned int textpos, std::vector<unsigned int>& textList);
		bool _parseBoolean(const std::wstring& str) const;

		void _checkConstant(const std::wstring& str, const std::wstring& section, bool checkList = true) const;
	};

}

