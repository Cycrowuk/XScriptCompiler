#include "pch.h"
#include "ParseExpression.h"
#include "ParseArray.h"
#include "ParseCondition.h"
#include "ParseVariable.h"
#include "ParseOperator.h"
#include "ParseBrackets.h"
#include "ParseFunction.h"

using namespace XScript;

ParseExpression::ParseExpression(const std::wstring& line) : BaseParse(line, ParseType::Expression),
	_condition(NULL),
	_assignment(NULL),
	_dataType(DataTypes::Unknown)
{
	DBOUT(L">> Creating ParseExpression: " << _line);
}

ParseExpression::~ParseExpression()
{
	DBOUT(L"<< Deleting ParseExpression: " << _line);
	if (_condition)
		delete _condition;
	if (_assignment)
		delete _assignment;
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		delete (*itr);
}

void ParseExpression::simplify()
{
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		const_cast<BaseParse*>(*itr)->simplify();
	if(_condition)
		_condition->simplify();
	if(_assignment)
		_assignment->simplify();

	{
		std::vector<const BaseParse*> list(_list);
		_list.clear();
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			if ((*itr)->type() == ParseType::Brackets)
			{
				const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(*itr);
				if (brackets->list().size() == 1)
				{
					auto parse = brackets->list().front();
					_list.push_back(parse);
					const_cast<ParseBrackets*>(brackets)->clear();
					delete brackets;
					continue;
				}
			}
			_list.push_back(*itr);
		}
	}

	{
		std::vector<const BaseParse*> list(_list);
		_list.clear();
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			if ((*itr)->type() == ParseType::Expression)
			{
				const ParseExpression* expr = dynamic_cast<const ParseExpression*>(*itr);
				if (expr->list().size() == 1 && !expr->condition() && !expr->assignment())
				{
					auto parse = expr->list().front();
					_list.push_back(parse);
					const_cast<ParseExpression*>(expr)->clearList();
					delete expr;
					continue;
				}
			}
			_list.push_back(*itr);
		}
	}
}

void ParseExpression::addParse(BaseParse* parse)
{
	_list.push_back(parse);
}
void ParseExpression::removeLastParse()
{
	_list.pop_back();
}

void ParseExpression::clearList()
{
	_list.clear();
}

void ParseExpression::setAssignment(ParseVariable* variable)
{
	_assignment = variable;
}

void ParseExpression::setCondition(ParseCondition* cond)
{
	_condition = cond;
}

void ParseExpression::setDataType(DataTypes dt)
{
	_dataType = dt;
}

DataTypes ParseExpression::dataType() const
{
	return _dataType;
}

ParseVariable* ParseExpression::returnValue() const
{
	if (_assignment)
	{
		if (_assignment->type() == ParseType::Variable)
			return _assignment;
	}
	else if (!_list.empty() && _list.front()->type() == ParseType::Function)
	{
		const ParseFunction* func = dynamic_cast<const ParseFunction*>(_list.front());
		if (func->returnVariable())
			return func->returnVariable();
	}
	else if (!_list.empty() && _list.front()->type() == ParseType::Array)
	{
		const ParseArray* arr = dynamic_cast<const ParseArray*>(_list.front());
		if (arr->assignment() && arr->assignment()->type() == ParseType::Variable)
			return dynamic_cast<ParseVariable*>(arr->assignment());
	}

	return NULL;
}
ParseVariable* ParseExpression::assignment() const
{
	return _assignment;
}

ParseCondition* ParseExpression::condition() const
{
	return _condition;
}

const std::vector<const BaseParse*> &ParseExpression::list() const
{
	return _list;
}

size_t ParseExpression::size() const
{
	return _list.size();
}

bool ParseExpression::isComparison() const
{
	return _isComparison(_list);
}

unsigned int ParseExpression::lineCount() const
{
	if (_list.empty())
		return 0;

	unsigned int count = 0;
	for (auto itr = _list.begin(); itr != _list.end(); itr++)
		count += (*itr)->lineCount();

	return count ? count : 1;
}

bool ParseExpression::_isComparison(const std::vector<const BaseParse*>& list) const
{
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		if ((*itr)->type() == ParseType::Operator && dynamic_cast<const ParseOperator*>(*itr)->isComparison())
			return true;
		else if ((*itr)->type() == ParseType::Brackets)
		{
			if (_isComparison(dynamic_cast<const ParseBrackets*>(*itr)->constList()))
				return true;
		}
		else if ((*itr)->type() == ParseType::Expression && dynamic_cast<const ParseExpression*>(*itr)->isComparison())
			return true;
	}

	return false;

}
