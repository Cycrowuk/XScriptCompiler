#include "pch.h"
#include "ParseSymbol.h"

using namespace XScript;

ParseSymbol::ParseSymbol(const std::wstring& line, const std::wstring& symbol) : BaseParse(line, ParseType::Symbol), _str(symbol)
{
	_symbol = _convertSymbol(symbol);
	DBOUT(L">> Creating ParseSymbol: " << _line << L" : " << symbol << L"(" << static_cast<int>(_symbol) << L")");
}

ParseSymbol::~ParseSymbol()
{
	DBOUT(L"<< Deleting ParseSymbol: " << _line << L" : " << _str << L"(" << static_cast<int>(_symbol) << L")");
}

std::wstring ParseSymbol::stringData() const
{
	return _str;
}

SymbolType ParseSymbol::symbol() const
{
	return _symbol;
}

void ParseSymbol::switchSymbol()
{
	switch (_symbol)
	{
	case SymbolType::DefineLabel:
		_symbol = SymbolType::InlineElse;
		break;
	case SymbolType::InlineElse:
		_symbol = SymbolType::DefineLabel;
		break;
	}
}
SymbolType ParseSymbol::_convertSymbol(const std::wstring& symbol) const
{
	if (symbol == L"(")
		return SymbolType::OpenBracket;
	else if (symbol == L")")
		return SymbolType::CloseBracket;
	else if (symbol == L",")
		return SymbolType::Comma;
	else if (symbol == L"->")
		return SymbolType::Object;
	else if (symbol == L"()")
		return SymbolType::Function;
	else if (symbol == L"=")
		return SymbolType::Assignment;
	else if (symbol == L";")
		return SymbolType::End;
	else if (symbol == L"{")
		return SymbolType::StartBlock;
	else if (symbol == L"}")
		return SymbolType::EndBlock;
	else if (symbol == L"[")
		return SymbolType::OpenArray;
	else if (symbol == L"]")
		return SymbolType::CloseArray;
	else if (symbol == L":")
		return SymbolType::DefineLabel;
	else if (symbol == L"::")
		return SymbolType::Namespace;
	else if (symbol == L"#")
		return SymbolType::Preprocessor;
	else if (symbol == L"++") return SymbolType::Increment;
	else if (symbol == L"--") return SymbolType::Decrement;
	else if (symbol == L"+=") return SymbolType::PlusAssign;
	else if (symbol == L"-=") return SymbolType::MinusAssign;
	else if (symbol == L"*=") return SymbolType::MultiplyAssign;
	else if (symbol == L"/=") return SymbolType::DivideAssign;
	return SymbolType::Unknown;
}
