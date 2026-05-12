#include "pch.h"
#include "ParseNull.h"

using namespace XScript;

ParseNull::ParseNull(const std::wstring& line) : BaseParse(line, ParseType::Null)
{

}

DataTypes ParseNull::dataType() const
{
	return DataTypes::Null;
}

std::wstring ParseNull::stringData() const
{
	return L"0";
}
