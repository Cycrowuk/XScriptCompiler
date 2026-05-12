#include "pch.h"
#include "ParseOperator.h"

using namespace XScript;

Operators ParseOperator::ConvertOperator(const std::wstring& symbol)
{
	if (symbol == L"+")
		return Operators::Add;
	else if (symbol == L"-")
		return Operators::Subtract;
	else if (symbol == L"(")
		return Operators::OpenBracket;
	else if (symbol == L")")
		return Operators::CloseBracket;
	else if (symbol == L"&&")
		return Operators::BoolAnd;
	else if (symbol == L"&")
		return Operators::And;
	else if (symbol == L"|")
		return Operators::Or;
	else if (symbol == L"/")
		return Operators::Divide;
	else if (symbol == L">")
		return Operators::Greater;
	else if (symbol == L">=")
		return Operators::GreaterEquals;
	else if (symbol == L"<")
		return Operators::Lesser;
	else if (symbol == L"<=")
		return Operators::LesserEquals;
	else if (symbol == L"%")
		return Operators::Modulus;
	else if (symbol == L"*")
		return Operators::Multiple;
	else if (symbol == L"!")
		return Operators::BoolNot;
	else if (symbol == L"!=")
		return Operators::NotEquals;
	else if (symbol == L"||")
		return Operators::BoolOr;
	else if (symbol == L"==")
		return Operators::Equals;
	else if (symbol == L"^")
		return Operators::Xor;
	else if (symbol == L"~")
		return Operators::Not;

	return Operators::Unknown;
}

ParseOperator::ParseOperator(const std::wstring& line, const std::wstring& symbol) : BaseParse(line, ParseType::Operator),
	_operator(Operators::Unknown),
	_str(symbol)
{
	_operator = ParseOperator::ConvertOperator(symbol);
	DBOUT(L">> Creating ParseOperator: " << _line << " : " << symbol << " (" << static_cast<int>(_operator) << ")");
}

ParseOperator::~ParseOperator()
{
	DBOUT(L"<< Delete ParseOperator: " << _line << " : " << _str << " (" << static_cast<int>(_operator) << ")");
}

void ParseOperator::switchType(Operators oper)
{
	_operator = oper;
}

DataTypes ParseOperator::dataType() const
{
	return DataTypes::Operator;
}

std::wstring ParseOperator::stringData() const
{
	return std::to_wstring(static_cast<int>(_operator));
}

const std::wstring &ParseOperator::operString() const
{
	return _str;
}

Operators ParseOperator::operType() const
{
	return _operator;
}

bool ParseOperator::isOperSingle() const
{
	// special case for subtract (before it gets switched to "negate")
	if (_operator == Operators::Subtract)
		return true;
	return static_cast<int>(_operator) & _SCRIPT_OP_FLAG_ONEOP;
}

bool ParseOperator::isComparison() const
{
	switch (_operator)
	{
	case Operators::BoolAnd:
	case Operators::BoolOr:
	case Operators::BoolNot:
	case Operators::Equals:
	case Operators::Greater:
	case Operators::GreaterEquals:
	case Operators::Lesser:
	case Operators::LesserEquals:
	case Operators::NotEquals:
		return true;
	}

	return false;
}

bool ParseOperator::isNumericOperator() const
{
	switch (_operator)
	{
	case Operators::And:
	case Operators::Divide:
	case Operators::Greater:
	case Operators::GreaterEquals:
	case Operators::Negate:
	case Operators::Lesser:
	case Operators::LesserEquals:
	case Operators::Modulus:
	case Operators::Multiple:
	case Operators::Not:
	case Operators::Or:
	case Operators::Subtract:
	case Operators::Xor:
		return true;
	}

	return false;
}
