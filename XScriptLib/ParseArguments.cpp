#include "pch.h"
#include "ParseArguments.h"

#include "ParseFail.h"
#include "ParseBrackets.h"
#include "ParseSymbol.h"
#include "ParseExpression.h"

using namespace XScript;

ParseArguments::ParseArguments(const std::wstring& l) : BaseParse(l, ParseType::Arguments)
{
	DBOUT(L">> Creating ParseArguments: " << _line);
}

ParseArguments::~ParseArguments()
{
	DBOUT(L"<< Deleting ParseArguments: " << _line);
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		delete (*itr);
}

void ParseArguments::simplify()
{
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		const_cast<BaseParse*>(*itr)->simplify();
}

void ParseArguments::addParse(BaseParse* parse)
{
	_list.push_back(parse);
}
void ParseArguments::insertParse(BaseParse* parse)
{
	_list.insert(_list.begin(), parse);
}

void ParseArguments::clear()
{
	_list.clear();
}

ParseFail* ParseArguments::addArguments(ParseBrackets* brackets)
{
	ParseFail* failed = NULL;
	std::vector<BaseParse*> list;
	ParseSymbol* previousComma = NULL;
	for (auto itr = brackets->list().begin(); itr != brackets->list().end(); itr++)
	{
		BaseParse* parse = *itr;
		if (failed)
		{
			_list.push_back(parse);
			continue;
		}

		if (parse->type() == ParseType::Symbol)
		{
			ParseSymbol* symb = dynamic_cast<ParseSymbol*>(parse);
			if (symb->symbol() == SymbolType::Comma)
			{
				if (previousComma)
					delete previousComma;
				previousComma = symb;
				failed = _processList(list, previousComma);
				list.clear();
				continue;
			}
		}

		list.push_back(parse);
	}

	if (!failed)
		failed = _processList(list, previousComma);

	if (previousComma)
		delete previousComma;
	return failed;
}


size_t ParseArguments::count() const
{
	return _list.size();
}

const BaseParse* ParseArguments::get(size_t i) const
{
	return _list.at(i);
}

const std::vector<BaseParse*>& ParseArguments::list() const
{
	return _list;
}
std::vector<const BaseParse*> ParseArguments::constList() const
{
	std::vector<const BaseParse*> list;
	list.reserve(_list.size());

	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		list.push_back(*itr);

	return list;
}

ParseFail* ParseArguments::_processList(std::vector<BaseParse*>& list, ParseSymbol* comma)
{
	if (list.size() == 0)
	{
		if (comma)
		{
			ParseFail* fail = new ParseFail(comma, ParseErrors::MissingFunctionArgument);
			return fail;
		}
	}
	else if (list.size() == 1)
	{
		if (list.front()->type() == ParseType::Brackets)
		{
			const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(list.front());
			if (brackets->size() == 1)
			{
				_list.push_back(brackets->list().front());
				const_cast<ParseBrackets*>(brackets)->clear();
				delete brackets;
				return NULL;
			}
		}
		_list.push_back(list.front());
	}
	else
	{
		ParseExpression* expr = new ParseExpression(list.front()->line());
		expr->setLinePosition(list.front()->linePos());
		expr->setPosition(list.front()->startingPos(), list.front()->endingPos());
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			expr->addParse(const_cast<BaseParse*>(*itr));
			expr->setPosition(expr->startingPos(), (*itr)->endingPos());
		}
		expr->setData(expr->line().substr(expr->startingPos(), expr->endingPos() - expr->startingPos()));
		_list.push_back(expr);
	}

	return NULL;

}

