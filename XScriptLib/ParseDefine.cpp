#include "pch.h"
#include "ParseDefine.h"
#include "ParseKeyword.h"
#include "CScriptParser.h"

using namespace XScript;

ParseDefine::ParseDefine(const ParseKeyword* keyword) : BaseParse(keyword->line(), ParseType::Define)
{
	this->setFromParse(keyword);
	_define = keyword->keyword();
}

ParseDefine::~ParseDefine()
{
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		delete* itr;
	_list.clear();
}

void ParseDefine::addParse(const BaseParse* parse)
{
	_list.push_back(parse);
}

void ParseDefine::addVariable(const ParseKeyword* keyword)
{
	_variablesMap[keyword->keyword()] = _variables.size();
	_variables.push_back(keyword->keyword());
	this->setPosition(this->startingPos(), keyword->endingPos());
}

void ParseDefine::parseDefine(const ParseDefine* define)
{
	if (!define->define().empty() && !define->list().empty())
	{
		std::vector<const BaseParse*> oldList(_list);
		_list.clear();

		for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
		{
			if ((*itr)->type() == ParseType::Keyword)
			{
				auto keyword = dynamic_cast<const ParseKeyword*>(*itr);
				if (keyword->keyword() == define->define())
				{
					auto& list = define->list();
					for (auto dItr = list.begin(); dItr != list.end(); dItr++)
						_list.push_back(CScriptParser::CopyParse(*dItr));
					continue;
				}
			}
			_list.push_back(*itr);
		}
	}
}

const std::wstring& ParseDefine::define() const
{
	return _define;
}

std::wstring ParseDefine::stringData() const
{
	return _define;
}

const std::vector<const BaseParse*>& ParseDefine::list() const
{
	return _list;
}
const std::vector<std::wstring>& ParseDefine::variables() const
{
	return _variables;
}
