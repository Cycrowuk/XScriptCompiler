#include "pch.h"
#include "XScriptUDL.h"
#include "Utils.h"
#include "CScriptData.h"

#include <set>

using namespace XScript;

XScriptUDL::XScriptUDL(CScriptData* data) : _pData(data)
{

}

bool XScriptUDL::writeUDL(const std::wstring& file)
{
	std::ofstream out(file);
	if (out.bad())
		return false;

	out << "<NotepadPlus>" << std::endl;
	out << "\t<UserLang name=\"xscript\" ext=\"XScript\" udlVersion=\"2.1\">" << std::endl;
	out << "\t\t<Settings>" << std::endl;
	out << "\t\t\t<Global caseIgnored=\"no\" allowFoldOfComments=\"yes\" foldCompact=\"no\" forcePureLC=\"0\" decimalSeparator=\"0\" />" << std::endl;
	out << "\t\t\t<Prefix Keywords1=\"no\" Keywords2=\"yes\" Keywords3=\"no\" Keywords4=\"no\" Keywords5=\"no\" Keywords6=\"no\" Keywords7=\"no\" Keywords8=\"no\" />" << std::endl;
	out << "\t\t</Settings>" << std::endl;
	out << "\t\t<KeywordLists>" << std::endl;
	out << "\t\t\t<Keywords name=\"Comments\">00// 01 02 03/* 04*/</Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, prefix1\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, prefix2\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, extras1\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, extras2\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, suffix1\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, suffix2\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Numbers, range\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Operators1\">-&gt; () , ;</Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Operators2\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in code1, open\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in code1, middle\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in code1, close\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in code2, open\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in code2, middle\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in code2, close\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in comment, open\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in comment, middle\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Folders in comment, close\"></Keywords>" << std::endl;
	
	out << "\t\t\t<Keywords name=\"Keywords1\">";
	_writeKeyword(out, L"if");
	_writeKeyword(out, L"not");
	_writeKeyword(out, L"while");
	_writeKeyword(out, _pData->breakCommand());
	_writeKeyword(out, _pData->elseCommand());
	_writeKeyword(out, _pData->gosubCommand());
	_writeKeyword(out, _pData->gotoCommand());
	_writeKeyword(out, _pData->continueCommand());
	_writeKeyword(out, _pData->returnCommand());
	_writeKeyword(out, L"null");
	_writeKeyword(out, L"endsub");

	for (unsigned int i = 0; i < 10; i++)
		_writeKeyword(out, _pData->getConstantCode(i));

	out << "</Keywords>" << std::endl;

	out << "\t\t\t<Keywords name=\"Keywords2\">$</Keywords>" << std::endl;

	out << "\t\t\t<Keywords name=\"Keywords3\">";
	for (auto itr = _pData->dataTypes().begin(); itr != _pData->dataTypes().end(); itr++)
		_writeKeyword(out, itr->first);
	out << "</Keywords>" << std::endl;
	
	out << "\t\t\t<Keywords name=\"Keywords4\">";
	for (auto itr = _pData->races().begin(); itr != _pData->races().end(); itr++)
		_writeKeyword(out, itr->first);
	out << "</Keywords>" << std::endl;

	out << "\t\t\t<Keywords name=\"Keywords5\">";
	for (auto itr = _pData->commands().begin(); itr != _pData->commands().end(); itr++)
	{
		for (auto cItr = itr->second.list.begin(); cItr != itr->second.list.end(); cItr++)
			_writeKeyword(out, cItr->first);
	}
	out << "</Keywords>" << std::endl;

	out << "\t\t\t<Keywords name=\"Keywords6\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Keywords7\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Keywords8\"></Keywords>" << std::endl;
	out << "\t\t\t<Keywords name=\"Delimiters\">00&quot; 01\\ 02&quot; 03[ 04 05] 06( 07 08) 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23</Keywords>" << std::endl;
	out << "\t\t</KeywordLists>" << std::endl;
	out << "\t\t<Styles>" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DEFAULT\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"COMMENTS\" fgColor=\"808080\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"LINE COMMENTS\" fgColor=\"808080\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"NUMBERS\" fgColor=\"FF00FF\" bgColor=\"FFFFFF\" fontStyle=\"1\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS1\" fgColor=\"0000FF\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS2\" fgColor=\"18D122\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS3\" fgColor=\"FF80C0\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS4\" fgColor=\"FF80C0\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS5\" fgColor=\"FF80C0\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS6\" fgColor=\"FF80C0\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS7\" fgColor=\"FF80C0\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"KEYWORDS8\" fgColor=\"FF80C0\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"OPERATORS\" fgColor=\"FF8000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"FOLDER IN CODE1\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"FOLDER IN CODE2\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"FOLDER IN COMMENT\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS1\" fgColor=\"FF0000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS2\" fgColor=\"FF00FF\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"67108865\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS3\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"117701887\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS4\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"117701887\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS5\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS6\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS7\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;
	out << "\t\t\t<WordsStyle name=\"DELIMITERS8\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontStyle=\"0\" nesting=\"0\" />" << std::endl;

	out << "\t\t</Styles>" << std::endl;
	out << "\t</UserLang>" << std::endl;
	out << "</NotepadPlus>" << std::endl;

	out.close();
	return true;
}

bool XScriptUDL::writeAutoComplete(const std::wstring& file)
{
	std::ofstream out(file);
	if (out.bad())
		return false;

	out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
	out << "<NotepadPlus>" << std::endl;

	out << "\t<AutoComplete language=\"XScript\">" << std::endl;
	out << "\t\t<Environment ignoreCase=\"no\" startFunc=\"(\" stopFunc=\")\" paramSeparator=\",\" terminal=\";\" additionalWordChar=\"\"/>" << std::endl;

	std::map<std::wstring, int> functions;

	for(int i = 0; i < _pData->functionData().size(); i++)
	{
		std::wstring name = _pData->functionData()[i].name;
		if (!name.empty())
			functions[name] = i;
	}

	for (auto itr = _pData->constantData().begin(); itr != _pData->constantData().end(); itr++)
		functions[itr->first] = -1;

	std::vector<std::wstring> sorted;
	sorted.reserve(functions.size());
	for (auto itr = functions.begin(); itr != functions.end(); itr++)
		sorted.push_back(itr->first);

	std::sort(sorted.begin(), sorted.end());

	// write out all the functions
	for (auto itr = sorted.begin(); itr != sorted.end(); itr++)
	{
		int value = functions[*itr];

		if (value > 0)
		{
			auto func = _pData->getFunction(value);
			if (func)
			{
				out << "\t\t<KeyWord name=\"" << Utils::ws2s(*itr) << "\" func=\"yes\"";
				if (!func->description.empty())
					out << " descr=\"" << Utils::ws2s(func->description) << "\"";
				out << ">" << std::endl;

				if (func->returnValueType != RetVarType::None)
				{
					std::string ret;
					out << "\t\t\t<Overload retVal=\"" << ret << "\">" << std::endl;
				}
				else
					out << "\t\t\t<Overload>" << std::endl;

				for (auto itr = func->arguments.begin(); itr != func->arguments.end(); itr++)
				{
					out << "\t\t\t\t<Param name=\"";

					auto pardef = _pData->getParDefData(itr->pardef);
					out << Utils::ws2s(pardef->code) << "\"";

					if (!itr->description.empty())
						out << " descr=\"" << Utils::ws2s(itr->description) << "\"";

					out << "/>" << std::endl;
				}

				out << "\t\t\t</Overload>" << std::endl;
				out << "\t\t</KeyWord>" << std::endl;
			}
		}
		else
			out << "\t\t<KeyWord name=\"" << Utils::ws2s(*itr) << "\" func=\"no\"/>" << std::endl;
	}


	out << "\t</AutoComplete>" << std::endl;
	out << "</NotepadPlus>" << std::endl;

	out.close();
	return true;
}


void XScriptUDL::_writeKeyword(std::ofstream& out, const std::wstring& keyword)
{
	if(!keyword.empty())
		out << Utils::ws2s(keyword) << "&#x000D;&#x000A;";
}
void XScriptUDL::_writeKeyword(std::ofstream& out, unsigned int cmd)
{
	if (cmd)
	{
		auto func = _pData->getFunction(cmd);
		if (func)
		{
			if (!func->name.empty())
				_writeKeyword(out, func->name);
		}
	}
}
