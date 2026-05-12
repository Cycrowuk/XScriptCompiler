#include "pch.h"
#include "ParseVariable.h"

using namespace XScript;

ParseVariable::ParseVariable(const std::wstring& l, const std::wstring& name, const std::unordered_set<DataTypes> *currentType) : BaseParse(l, ParseType::Variable), _name(name)
{
	DBOUT(L">> Creating ParseVariable: " << _line << " : " << name);

	if (currentType)
	{
		for (auto itr = currentType->begin(); itr != currentType->end(); itr++)
			_currentDataTypes.insert(*itr);
	}
}

ParseVariable::~ParseVariable()
{
	DBOUT(L"<< Deleting ParseVariable: " << _line << " : " << _name);
}

void ParseVariable::clearDataTypes()
{
	_currentDataTypes.clear();
}

void ParseVariable::addDataType(DataTypes dt)
{
	_currentDataTypes.insert(dt);
}

void ParseVariable::setDataTypes(const std::unordered_set<DataTypes>& types)
{
	_currentDataTypes.clear();
	for (auto itr = types.begin(); itr != types.end(); itr++)
		_currentDataTypes.insert(*itr);
}

DataTypes ParseVariable::dataType() const
{
	return DataTypes::Variable;
}
DataTypes ParseVariable::currentDataType() const
{
	return _currentDataType;
}

std::wstring ParseVariable::stringData() const
{
	return _name;
}

const std::wstring& ParseVariable::name() const
{
	return _name;
}

const std::unordered_set<DataTypes>& ParseVariable::currentDataTypes() const
{
	return _currentDataTypes;
}

