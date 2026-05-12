#include "pch.h"
#include "ParseKeyword.h"

using namespace XScript;

ParseKeyword::ParseKeyword(const std::wstring& line, const std::wstring& str) : BaseParse(line, ParseType::Keyword), _str(str)
{
	DBOUT(L">> Creating ParseKeyword: " << _line << " : " << _str);
}

ParseKeyword::~ParseKeyword()
{
	DBOUT(L"<< Deleting ParseKeyword: " << _line << " : " << _str);
}

const std::wstring& ParseKeyword::keyword() const
{
	return _str;
}

std::wstring ParseKeyword::stringData() const
{
	return _str;
}

DataTypes ParseKeyword::dataType() const
{
	return DataTypes::String;
}

