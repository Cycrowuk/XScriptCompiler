#include "pch.h"
#include "ParseConstant.h"
#include "CScriptData.h"

using namespace XScript;

ParseConstant::ParseConstant(const std::wstring& l, const ConstantData* c) : BaseParse(l, ParseType::Constant), _constant(c)
{
	DBOUT(L"Creating ParseConstant: " << _line << " : " << stringData());
}

ParseConstant::~ParseConstant()
{
	DBOUT(L"<< Deleting ParseConstant: " << _line << " : " << stringData());
}

ConstGroup* ParseConstant::constGroup() const
{
	return (_constant) ? _constant->group() : NULL;
}

DataTypes ParseConstant::dataType() const
{
	if (_constant && _constant->type() == DataTypes::Custom)
		return _constant->subtype();

	return (_constant) ? _constant->type() : DataTypes::Unknown;
}

std::wstring ParseConstant::stringData() const
{
	if (_constant && _constant->type() == DataTypes::Custom)
	{
		if (!_constant->strData().empty())
			return _constant->strData();
	}

	return _constant ? std::to_wstring(_constant->id()) : L"0";
}

DataTypes ParseConstant::subType() const
{
	return (_constant) ? _constant->subtype() : DataTypes::Unknown;
}

unsigned int ParseConstant::id() const
{
	return (_constant) ? _constant->id() : 0;
}

