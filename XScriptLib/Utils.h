#pragma once

#include <string>
#include <vector>

namespace XScript
{
	namespace Utils
	{
		void trimString(std::string& s);
		size_t findString(const std::wstring& str, const std::wstring& find, size_t startPos);
		std::wstring trimString(const std::wstring& str);
		std::vector<std::string> splitString(const std::string& str, const std::string& token);
		std::vector<std::wstring> splitString(const std::wstring& str, const std::wstring& token);

		std::string ws2s(const std::wstring& wstr);
		std::wstring s2ws(const std::string& str);

		std::wstring CombineStrings(const std::wstring& str1, const std::wstring& str2);
		std::wstring CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3);
		std::wstring CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3, const std::wstring& str4);
		std::wstring CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3, const std::wstring& str4, const std::wstring& str5);
		std::wstring CombineStrings(const std::wstring& str1, const std::wstring& str2, const std::wstring& str3, const std::wstring& str4, const std::wstring& str5, const std::wstring& str6);
	}
}