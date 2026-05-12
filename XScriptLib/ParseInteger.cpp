#include "pch.h"
#include "ParseInteger.h"

using namespace XScript;

ParseInteger::ParseInteger(const std::wstring& line, int value) : BaseParse(line, ParseType::Integer), _value(value)
{
	DBOUT(L">> Creating ParseInteger: " << _line << " : " << value);
}

ParseInteger::~ParseInteger()
{
	DBOUT(L"<< Deleting ParseInteger: " << _line << " : " << _value);
}

int ParseInteger::value() const
{
	return _value;
}

DataTypes ParseInteger::dataType() const
{
	return DataTypes::Number;
}

std::wstring ParseInteger::stringData() const
{
	return std::to_wstring(_value);
}

