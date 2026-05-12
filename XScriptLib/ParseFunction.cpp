#include "pch.h"
#include "ParseFunction.h"

#include "ParseArguments.h"
#include "ParseVariable.h"
#include "ParseCondition.h"

using namespace XScript;

ParseFunction::ParseFunction(const std::wstring& line, const std::wstring& function) : BaseParse(line, ParseType::Function), 
	_function(function),
	_args(NULL),
	_object(NULL),
	_retvar(NULL),
	_condition(NULL)
{
	DBOUT(L">> Creating ParseFunction: " << _line << " : " << function);
}

ParseFunction::~ParseFunction()
{
	DBOUT(L"<< Deleting ParseFunction: " << _line << " : " << _function);

	if (_args)
	{
		for (auto itr = _args->list().begin(); itr != _args->list().end(); itr++)
		{
			if (*itr == _retvar)
				_retvar = NULL;
		}
	}
	if (_object)
		delete _object;
	if (_args)
		delete _args;
	if (_retvar)
		delete _retvar;
	if (_condition)
		delete _condition;
}

void ParseFunction::simplify()
{
	if (_condition)
		_condition->simplify();
	if (_args)
		_args->simplify();
	if (_retvar)
		_retvar->simplify();
	if (_object)
		_object->simplify();
}


unsigned int ParseFunction::lineCount() const
{
	unsigned int count = 1;

	if (_args)
		count += _args->lineCount();
	if (_retvar)
		count += _retvar->lineCount();
	if (_object)
		count += _object->lineCount();

	return count;
}

void ParseFunction::setObject(BaseParse* obj)
{
	_object = obj;
}

void ParseFunction::setArguments(ParseArguments* args)
{
	_args = args;
}

void ParseFunction::setReturnVariable(ParseVariable* retvar)
{
	_retvar = retvar;
}

void ParseFunction::setCondition(ParseCondition* cond)
{
	_condition = cond;
}

void ParseFunction::setPostRun(bool postRun)
{
	_postRun = postRun;
}

BaseParse* ParseFunction::object() const
{
	return _object;
}

const std::wstring& ParseFunction::function() const
{
	return _function;
}

ParseArguments* ParseFunction::arguments() const
{
	return _args;
}

ParseVariable* ParseFunction::returnVariable() const
{
	return _retvar;
}

ParseCondition* ParseFunction::condition() const
{
	return _condition;
}

bool ParseFunction::isPostRun() const
{
	return _postRun;
}