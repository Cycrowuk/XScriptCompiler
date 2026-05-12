#include "pch.h"
#include "ParseBrackets.h"

using namespace XScript;

ParseBrackets::ParseBrackets(const std::wstring& line) : BaseParse(line, ParseType::Brackets)
{
	DBOUT(L">> Creating ParseBrackets: " << _line);
}

ParseBrackets::~ParseBrackets()
{
	DBOUT(L"<< Deleting ParseBrackets: " << _line);
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		delete* itr;
}

void ParseBrackets::simplify()
{
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		(*itr)->simplify();
}


const std::vector<BaseParse*>& ParseBrackets::list() const
{
	return _list;
}

std::vector<const BaseParse*> ParseBrackets::constList() const
{
	std::vector<const BaseParse*> list;
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		list.push_back(*itr);
	return list;
}

void ParseBrackets::addParse(BaseParse* parse)
{
	_list.push_back(parse);
}

void ParseBrackets::clear()
{
	_list.clear();
}

size_t ParseBrackets::size() const
{
	return _list.size();
}

BaseParse* ParseBrackets::singleItem() const
{
	if (_list.size() == 1)
	{
		BaseParse* item = _list[0];
		if (item->type() == ParseType::Brackets)
			return dynamic_cast<ParseBrackets*>(item)->singleItem();
		return item;
	}

	return NULL;
}

unsigned int ParseBrackets::lineCount() const
{
	if (_list.empty())
		return 0;

	unsigned int count = 1;
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		count += (*itr)->lineCount();

	return count;
}
