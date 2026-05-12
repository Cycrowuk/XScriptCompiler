#include "pch.h"
#include "ParseCondition.h"

using namespace XScript;

ParseCondition::ParseCondition(const std::wstring& line, Conditions c) : BaseParse(line, ParseType::Condition), 
	_condition(c),
	_isBlock(false),
	_blockCount(0)
{
	DBOUT(L">> Creating ParseCondition: " << _line << " : " << static_cast<int>(c));
}

ParseCondition::~ParseCondition()
{
	DBOUT(L"<< Deleting ParseCondition: " << _line << " : " << static_cast<int>(_condition));
}

void ParseCondition::setBlock(bool block)
{
	_isBlock = block;
}

void ParseCondition::setBlockCount(unsigned int count)
{
	if(count > 0)
		_isBlock = true;
	_blockCount = count;
}

Conditions ParseCondition::condition() const
{
	return _condition;
}

bool ParseCondition::isBlock() const
{
	return _isBlock;
}

unsigned int ParseCondition::blockCount() const
{
	return _blockCount;
}