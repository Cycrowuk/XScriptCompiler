#include "pch.h"
#include "ParseFail.h"

using namespace XScript;

int ParseFail::_DEBUG_FAILS = 0;

ParseFail::ParseFail(const std::wstring& l, ParseErrors e) : BaseParse(l, ParseType::Failed),
	_error(e),
	_parsed(NULL)
{
	_DEBUG_FAILS++;
}

ParseFail::ParseFail(const BaseParse *p, ParseErrors e) : BaseParse(p->line(), ParseType::Failed),
	_error(e),
	_parsed(NULL)
{
	setFromParse(p);
	_DEBUG_FAILS++;
}

ParseFail::~ParseFail()
{
	_DEBUG_FAILS--;
}

void ParseFail::addData(const std::wstring& str)
{
	_data.push_back(str);
}

std::wstring ParseFail::data(size_t i) const
{
	if (i < 0 || i >= _data.size())
		return L"";
	return _data.at(i);
}

ParseErrors ParseFail::error() const
{
	return _error;
}

