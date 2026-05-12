#include "pch.h"
#include "Utils.h"

#include <locale>
#include <codecvt>
#include <sstream>

using namespace XScript::Utils;

std::wstring XScript::Utils::trimString(const std::wstring& str) 
{
	std::wstring s = str;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
		return !std::isspace(ch);
		}));

	return s;
}

size_t XScript::Utils::findString(const std::wstring& str, const std::wstring& find, size_t startPos)
{
	size_t pos = str.find_first_of(find, startPos);
	if (pos != std::wstring::npos)
		return pos;

	return -1;
}
std::vector<std::string> XScript::Utils::splitString(const std::string& str, const std::string& token)
{
	std::vector<std::string> list;

	size_t startPos = 0;
	size_t pos = str.find_first_of(token, startPos);
	while (pos != std::string::npos)
	{
		std::string s = str.substr(startPos, pos - startPos);
		list.push_back(s);
		startPos = pos + token.length();
		pos = str.find_first_of(token, startPos);
	}

	std::string s = str.substr(startPos, str.length() - startPos);
	if (!s.empty())
		list.push_back(s);

	return list;
}
std::vector<std::wstring> XScript::Utils::splitString(const std::wstring& str, const std::wstring& token)
{
	std::vector<std::wstring> list;

	size_t startPos = 0;
	size_t pos = str.find_first_of(token, startPos);
	while (pos != std::wstring::npos)
	{
		std::wstring s = str.substr(startPos, pos - startPos);
		list.push_back(s);
		startPos = pos + token.length();
		pos = str.find_first_of(token, startPos);
	}

	std::wstring s = str.substr(startPos, str.length() - startPos);
	if (!s.empty())
		list.push_back(s);

	return list;
}

void XScript::Utils::trimString(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
}

std::wstring XScript::Utils::s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string XScript::Utils::ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

std::wstring XScript::Utils::CombineStrings(const std::wstring& str1, const std::wstring& str2)
{
	std::wstringstream strm;
	strm << str1 << str2;
	return strm.str();
}
std::wstring XScript::Utils::CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3)
{
	std::wstringstream strm;
	strm << str1 << str2 << str3;
	return strm.str();
}
std::wstring XScript::Utils::CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3, const std::wstring& str4)
{
	std::wstringstream strm;
	strm << str1 << str2 << str3 << str4;
	return strm.str();
}
std::wstring XScript::Utils::CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3, const std::wstring& str4, const std::wstring& str5)
{
	std::wstringstream strm;
	strm << str1 << str2 << str3 << str4 << str5;
	return strm.str();
}
std::wstring XScript::Utils::CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3, const std::wstring& str4, const std::wstring& str5, const std::wstring& str6)
{
	std::wstringstream strm;
	strm << str1 << str2 << str3 << str4 << str5 << str6;
	return strm.str();
}
