#include "pch.h"
#include "ParseArray.h"

#include "ParseExpression.h"
#include "ParseFunction.h"
#include "ParseArguments.h"

using namespace XScript;

ParseArray::ParseArray(const std::wstring& line) : BaseParse(line, ParseType::Array),
	_value(NULL),
	_value2(NULL),
	_variable(NULL),
	_assignment(NULL),
	_assign(NULL),
	_function(NULL),
	_preRun(nullptr)
{
	DBOUT(L">> Creating ParseArray: " << _line);
}

ParseArray::~ParseArray()
{
	DBOUT(L"<< Deleting ParseArray: " << _line);
	if (_value)
		delete _value;
	if (_value2)
		delete _value2;
	if (_variable)
		delete _variable;
	if (_assignment)
		delete _assignment;
	if (_assign)
		delete _assign;
	if (_preRun)
		delete _preRun;
	if (_function)
	{
		// remove the references from the function as we have already deleted them
		_function->setReturnVariable(NULL);
		_function->setCondition(NULL);
		_function->arguments()->clear();
		delete _function;
	}
}

unsigned int ParseArray::lineCount() const
{
	unsigned int count = 1;

	if (_assignment)
		count += _assignment->lineCount();
	if (_variable)
		count += _variable->lineCount();
	if (_assign)
		count += _assign->lineCount();
	if (_value)
		count += _value->lineCount();

	return count;
}

void ParseArray::simplify()
{
	if (_assignment)
		_assignment->simplify();
	if (_assign)
		_assign->simplify();
	if (_value)
		_value->simplify();
	if (_function)
		_function->simplify();

	if (_assign && _assign->type() == ParseType::Expression)
	{
		ParseExpression* expr = dynamic_cast<ParseExpression*>(_assign);
		if (expr->size() == 1)
		{
			_assign = const_cast<BaseParse*>(expr->list().front());
			expr->clearList();
			delete expr;
		}
	}

	if (_variable && _variable->type() == ParseType::Array)
	{
		ParseArray* arr = dynamic_cast<ParseArray*>(_variable);
		_value2 = _value;
		_value = arr->value();

		_variable = arr->variable();

		arr->setValue(NULL);
		arr->setVariable(NULL);

		delete arr;

		if (_variable && _variable->type() == ParseType::Array)
			dynamic_cast<ParseArray*>(_variable)->simplify();
	}
	else if (_variable)
		_variable->simplify();
}

void ParseArray::setValue(BaseParse* value)
{
	_value = value;
}
void ParseArray::setValue2(BaseParse* value)
{
	_value2 = value;
}

void ParseArray::setVariable(BaseParse* value)
{
	_variable = value;
}

void ParseArray::setAssignment(BaseParse* value)
{
	_assignment = value;
}

void ParseArray::setAssign(BaseParse* value)
{
	_assign = value;
}

void ParseArray::setPreRun(BaseParse* value)
{
	_preRun = value;
}

void ParseArray::setFunction(ParseFunction* value)
{
	_function = value;
}

BaseParse* ParseArray::value() const
{
	return _value;
}

BaseParse* ParseArray::value2() const
{
	return _value2;
}

BaseParse* ParseArray::variable() const
{
	return _variable;
}
BaseParse* ParseArray::assignment() const
{
	return _assignment;
}
BaseParse* ParseArray::assign() const
{
	return _assign;
}
BaseParse* ParseArray::preRun() const
{
	return _preRun;
}
ParseFunction* ParseArray::function() const
{
	return _function;
}


