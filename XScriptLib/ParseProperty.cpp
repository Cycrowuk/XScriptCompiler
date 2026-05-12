#include "pch.h"
#include "ParseProperty.h"

#include "ParseFunction.h"
#include "ParseArguments.h"
#include "ParseExpression.h"
#include "ParseBrackets.h"

using namespace XScript;

ParseProperty::ParseProperty(const std::wstring& line, const std::wstring& prop) : BaseParse(line, ParseType::Property),
	_object(NULL),
	_property(prop),
	_getter(NULL),
	_setter(NULL),
	_getterFunction(NULL),
	_setterFunction(NULL)
{
	this->setData(prop);
}

ParseProperty::~ParseProperty()
{
	if (_getterFunction)
	{
		_getterFunction->setObject(NULL);
		_getterFunction->setReturnVariable(NULL);
		delete _getterFunction;
	}
	if (_setterFunction)
	{
		_setterFunction->setObject(NULL);
		_setterFunction->arguments()->clear();
		delete _setterFunction;
	}

	if (_getter)
		delete _getter;
	if (_setter)
		delete _setter;
	if (_object)
		delete _object;
}


void ParseProperty::setObject(BaseParse* obj)
{
	_object = obj;
}

void ParseProperty::setSetter(BaseParse* setter)
{
	_setter = setter;
}
void ParseProperty::setGetter(BaseParse* getter)
{
	_getter = getter;
}

void ParseProperty::setGetterFunction(ParseFunction* func)
{
	_getterFunction = func;
}
void ParseProperty::setSetterFunction(ParseFunction* func)
{
	_setterFunction = func;
}

ParseFunction* ParseProperty::getterFunction() const
{
	return _getterFunction;
}
ParseFunction* ParseProperty::setterFunction() const
{
	return _setterFunction;
}

BaseParse* ParseProperty::setter() const
{
	return _setter;
}
BaseParse* ParseProperty::getter() const
{
	return _getter;
}

BaseParse* ParseProperty::object() const
{
	return _object;
}

const std::wstring& ParseProperty::property() const
{
	return _property;
}

void ParseProperty::simplify()
{
	if (_setter && _setter->type() == ParseType::Expression)
	{
		ParseExpression* expr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(_setter));
		if (expr->size() == 1)
		{
			_setter = const_cast<BaseParse*>(expr->list().front());
			expr->clearList();
			delete expr;
		}
	}

	if (_setter && _setter->type() == ParseType::Brackets)
	{
		ParseBrackets* brackets = const_cast<ParseBrackets*>(dynamic_cast<const ParseBrackets*>(_setter));
		if (brackets->size() == 1)
		{
			_setter = brackets->list().front();
			brackets->clear();
			delete brackets;
		}
	}

	if (_setter)
		_setter->simplify();
	if (_getter)
		_getter->simplify();
	if (_object)
		_object->simplify();
}
