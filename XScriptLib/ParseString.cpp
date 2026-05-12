#include "pch.h"
#include "ParseString.h"

using namespace XScript;

ParseString::ParseString(const std::wstring& l, const std::wstring& s) : BaseParse(l, ParseType::String), _str(s)
{
	DBOUT(L"Creating ParseString: " << _line << " : " << _str);
}

ParseString::~ParseString()
{
	DBOUT(L"<< Deleting ParseString: " << _line << " : " << _str);
}

std::wstring ParseString::stringData() const
{
	return _str;
}

DataTypes ParseString::dataType() const
{
	return DataTypes::String;
}
