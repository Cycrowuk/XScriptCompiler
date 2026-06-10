#include "pch.h"
#include "CScriptParser.h"
#include <fstream>

#include "CScript.h"
#include "CScriptData.h"
#include "ParseSymbol.h"

#include <sstream>
#include "ParseFunction.h"
#include "ParseBrackets.h"
#include "ParseCondition.h"
#include "ParseExpression.h"
#include "ParseOperator.h"
#include "ParseArguments.h"
#include "ParseFail.h"
#include "ParseArray.h"
#include "ParseInteger.h"
#include "ParseConstant.h"
#include "ParseString.h"
#include "ParseKeyword.h"
#include "ParseVariable.h"
#include "ParseNull.h"
#include "ParseLabel.h"
#include "ParseNamespace.h"
#include "ParseProperty.h"
#include "ParseDefine.h"

using namespace XScript;


std::wstring CombineStrings(const std::wstring& str1, const std::wstring& str2)
{
	std::wstringstream strm;
	strm << str1 << str2;
	return strm.str();
}

/////////////////////////////////////////////////////////////////////////////////////////

CScriptParser::CScriptParser(const CScriptData* data) :
	_currentScript(NULL),
	_data(data),
	_generatedVariables(0),
	_whileGeneratedVariables(0),
	_tabSize(6),
	_isInComment(false),
	_prePassMode(false),
	_subEndedOnLine(false),
	_inLineContinuation(false),
	_prePassDepth(0),
	_pVariables(&_variables)
{
	_currentScript = new CScript(data);
}

CScriptParser::~CScriptParser()
{
	if (_currentScript)
		delete _currentScript;
	_clearData();

}

/////////////////////////////////////////////////////////////////////////////////////////
// Statics

BaseParse* CScriptParser::CopyParse(const BaseParse* parse)
{
	BaseParse* newParse = NULL;

	switch (parse->type())
	{
	case ParseType::Keyword:
		newParse = new ParseKeyword(*dynamic_cast<const ParseKeyword*>(parse));
		break;
	case ParseType::Symbol:
		newParse = new ParseSymbol(*dynamic_cast<const ParseSymbol*>(parse));
		break;
	case ParseType::Operator:
		newParse = new ParseOperator(*dynamic_cast<const ParseOperator*>(parse));
		break;
	case ParseType::Condition:
		newParse = new ParseCondition(*dynamic_cast<const ParseCondition*>(parse));
		break;
	case ParseType::Constant:
		newParse = new ParseConstant(*dynamic_cast<const ParseConstant*>(parse));
		break;
	case ParseType::Integer:
		newParse = new ParseInteger(*dynamic_cast<const ParseInteger*>(parse));
		break;
	case ParseType::String:
		newParse = new ParseString(*dynamic_cast<const ParseString*>(parse));
		break;
	case ParseType::Variable:
		newParse = new ParseVariable(*dynamic_cast<const ParseVariable*>(parse));
		break;
	}

	if (newParse)
		newParse->setFromParse(parse);
	return newParse;
}

/////////////////////////////////////////////////////////////////////////////////////////


CScript* CScriptParser::currentScript() const
{
	return _currentScript;
}

const std::vector<const ParseFail*>& CScriptParser::errorData() const
{
	return _errors;
}

void CScriptParser::_clearData()
{
	for (auto itr = _errors.begin(); itr != _errors.end(); itr++)
		delete (*itr);
	for (auto itr = _currentDataList.begin(); itr != _currentDataList.end(); itr++)
		delete (*itr);
	for (auto itr = _createdData.begin(); itr != _createdData.end(); itr++)
		delete (*itr);
	for (auto itr = _defines.begin(); itr != _defines.end(); itr++)
		delete itr->second;
	_currentDataList.clear();
	_createdData.clear();
	_errors.clear();
	_warnings.clear();
	_defines.clear();
	for (void* p : _syntheticConstants)
		delete static_cast<ConstantData*>(p);
	_syntheticConstants.clear();
	_subVariables.clear();
	_currentSubLabel.clear();
	_subEndedOnLine = false;
	_inLineContinuation = false;
	_continuationText.clear();
	_macroStack.clear();
	_prePassDepth = 0;
	_pVariables = &_variables;
	_conditionStack.clear();
	_whileGeneratedVariables = 0;
	_createdExpressions.clear();
	_ifDefStack.clear();
}

bool CScriptParser::hasWarnings() const
{
	return !_warnings.empty();
}

const std::vector<Warnings>& CScriptParser::warnings() const
{
	return _warnings;
}


/////////////////////////////////////////////////////////////////////////////////////////

void CScriptParser::addCurrentFile(const std::wstring& file)
{
	_currentFile.push_back(file);
}

void CScriptParser::removeCurrentFile()
{
	_currentFile.pop_back();
}

void CScriptParser::addDefine(const std::wstring& name)
{
	// Create an empty (presence-only) define — same as writing #define NAME in the script.
	// The ParseKeyword is stored in _createdData so it gets cleaned up correctly.
	ParseKeyword* key = new ParseKeyword(name, name);
	ParseDefine* define = new ParseDefine(key);
	_createdData.push_back(key);
	_createdData.push_back(define);
	_defines[define->define()] = define;
}

bool CScriptParser::includeFile(const std::wstring& filename)
{
	// Detect circular includes — if this file is already in the stack, bail
	for (const auto& f : _currentFile)
	{
		if (f == filename)
		{
			ParseKeyword stub(filename, filename);
			_addError(ParseErrors::InvalidPreprocessor, &stub);
			return false;
		}
	}

	std::wifstream infile(filename);
	if (!infile.is_open())
	{
		// Emit an error — we don't have a parse node here so use a minimal stub
		ParseKeyword stub(filename, filename);
		_addError(ParseErrors::MissingPreprocessor, &stub);
		return false;
	}

	addCurrentFile(filename);

	std::wstring line;
	size_t lineNum = 0;
	bool ok = true;
	while (std::getline(infile, line))
	{
		++lineNum;
		if (!parseLine(lineNum, line))
		{
			ok = false;
			break;
		}
	}

	infile.close();
	removeCurrentFile();
	return ok;
}

bool CScriptParser::_expandMacro(const MacroData* macro, const std::vector<std::wstring>& args, const std::vector<std::wstring>& body)
{
	// Helper: substitute %ARG0%, %ARG1% etc. with the actual argument strings
	auto substitute = [&](const std::wstring& tmpl) -> std::wstring
		{
			std::wstring result = tmpl;
			for (size_t i = 0; i < args.size(); i++)
			{
				std::wstring placeholder = L"%ARG" + std::to_wstring(i) + L"%";
				size_t pos = 0;
				while ((pos = result.find(placeholder, pos)) != std::wstring::npos)
				{
					result.replace(pos, placeholder.size(), args[i]);
					pos += args[i].size();
				}
			}
			return result;
		};

	// Replay each routine line
	bool anyError = false;
	for (const MacroRoutineLine& rline : macro->routine)
	{
		bool ok = true;
		switch (rline.type)
		{
		case MacroRoutineLine::Type::Expression:
		{
			std::wstring expanded = substitute(rline.text);

			// Substitute $0, $1 etc. using FunctionArgument entries
			// Each $N is replaced with a generated temp variable that holds the
			// result of calling the specified function with the specified argument
			if (!rline.funcArgs.empty())
			{
				for (size_t fi = 0; fi < rline.funcArgs.size(); fi++)
				{
					const MacroRoutineLine::FuncArg& fa = rline.funcArgs[fi];
					const Function* fn = _data->getFunction(fa.funcId);
					if (!fn) continue;

					// Build the function call string: funcName(arg)
					std::wstring callArg;
					if (fa.argPos >= 0 && fa.argPos < static_cast<int>(args.size()))
						callArg = args[fa.argPos];

					// Generate a unique temp variable name
					std::wstring tempVar = L"$_macro_" + fn->name + L"_" + std::to_wstring(fi);

					// Emit: $tempVar = funcName(arg);
					std::wstring assignLine = tempVar + L" = " + fn->name + L"(" + callArg + L");";
					if (!parseLine(0, assignLine))
						return false;

					// Replace $N in expanded with the temp variable
					std::wstring placeholder = L"$" + std::to_wstring(fi);
					size_t pos = 0;
					while ((pos = expanded.find(placeholder, pos)) != std::wstring::npos)
					{
						expanded.replace(pos, placeholder.size(), tempVar);
						pos += tempVar.size();
					}
				}
			}

			// Determine if semicolon is needed
			bool needsSemicolon = !expanded.empty()
				&& expanded.back() != L';'
				&& expanded.back() != L'{'
				&& expanded.back() != L'}';
			if (needsSemicolon)
			{
				std::wstring trimmed = expanded;
				size_t first = trimmed.find_first_not_of(L" \t");
				if (first != std::wstring::npos)
				{
					std::wstring word;
					size_t pos = first;
					while (pos < trimmed.size() && iswalpha(trimmed[pos]))
						word += trimmed[pos++];
					if (word == L"while" || word == L"whilenot" ||
						word == L"if" || word == L"ifnot" ||
						word == L"else")
						needsSemicolon = false;
				}
			}
			if (needsSemicolon)
				expanded += L";";
			ok = parseLine(0, expanded);
			break;
		}
		case MacroRoutineLine::Type::StartBlock:
			ok = parseLine(0, L"{");
			break;
		case MacroRoutineLine::Type::EndBlock:
			ok = parseLine(0, L"}");
			break;
		case MacroRoutineLine::Type::BlockCommands:
			// Replay the captured body lines
			for (const std::wstring& bodyLine : body)
			{
				// Skip empty/whitespace-only lines
				if (bodyLine.find_first_not_of(L" \t\r\n") == std::wstring::npos)
					continue;
				bool bodyOk = parseLine(0, bodyLine);
				if (!bodyOk)
					anyError = true;
			}
			break;
		}
		if (!ok) anyError = true;
	}
	return !anyError;
}

BaseParse* CScriptParser::parseCondition(const std::wstring& line) const
{
	if (line == L"if")
		return new ParseCondition(line, Conditions::If);
	else if (line == L"ifnot")
		return new ParseCondition(line, Conditions::IfNot);
	else if (line == L"else")
		return new ParseCondition(line, Conditions::Else);
	else if (line == L"elseif")
		return new ParseCondition(line, Conditions::ElseIf);
	else if (line == L"elseifnot")
		return new ParseCondition(line, Conditions::ElseIfNot);
	else if (line == L"while")
		return new ParseCondition(line, Conditions::While);
	else if (line == L"whilenot")
		return new ParseCondition(line, Conditions::WhileNot);
	else if (line == L"not")
		return new ParseCondition(line, Conditions::Not);
	else if (line == L"START")
		return new ParseCondition(line, Conditions::Start);

	return NULL;
}

BaseParse* CScriptParser::parseConstant(const std::wstring& line) const
{
	// check if its a variable
	if (line[0] == '$')
	{
		const std::unordered_set<DataTypes>* dt = NULL;
		auto itr = _variables.find(line);
		if (itr != _variables.end())
			dt = &itr->second;
		return new ParseVariable(line, line, dt);
	}

	BaseParse* condition = parseCondition(line);
	if (condition)
		return condition;

	if (line == L"null")
		return new ParseNull(line);

	const ConstantData* c = _data->findConstant(line);
	if (c)
		return new ParseConstant(line, c);

	// Check whether the name matches a DataType's prefix pattern.
	// Check whether the name matches a DataType's prefix pattern.
	// e.g. "SHIPCOMMAND_1000" matches a DataType with prefix "SHIPCOMMAND_",
	// and is accepted as a constant with numeric id 1000, using the DataType's
	// id as subtype — allowing custom mod commands without pre-defined entries.
	// Allocated on the heap; tracked in _syntheticConstants (as void* to avoid
	// including CScriptData.h in the parser header) and deleted in _clearData.
	unsigned int prefixId = 0;
	const DataTypeData* prefixDt = _data->findDatatypeByPrefix(line, prefixId);
	if (prefixDt)
	{
		// Use the prefix's actual DataType as the constant's type so the script
		// serialiser writes the correct sval type to the binary output.
		// Using DataTypes::Constant here causes the engine to misinterpret it
		// as a generic constant instead of e.g. DATATYPE_SHIPCOMMAND.
		ConstantData* synthetic = new ConstantData(prefixDt->id, line, prefixId, prefixDt->id);
		_syntheticConstants.push_back(static_cast<void*>(synthetic));
		return new ParseConstant(line, synthetic);
	}

	ParseFail* fail = new ParseFail(line, ParseErrors::UnknownConstant);
	fail->addData(line);

	return fail;
}

std::vector<const BaseParse*>::iterator CScriptParser::addBracket(std::vector<const BaseParse*>::iterator startItr, const std::vector<const BaseParse*>& list, ParseBrackets* currentBracket)
{
	currentBracket->setPosition((*startItr)->startingPos(), 0);

	auto itr = startItr;
	itr++;

	for (; itr != list.end(); itr++)
	{
		if ((*itr)->type() == ParseType::Symbol)
		{
			const ParseSymbol* symbol = dynamic_cast<const ParseSymbol*>(*itr);
			if (symbol->symbol() == SymbolType::OpenBracket)
			{
				ParseBrackets* brackets = new ParseBrackets(symbol->line());
				brackets->setFromParse(symbol);
				currentBracket->addParse(brackets);
				auto checkItr = addBracket(itr, list, brackets);

				delete symbol;
				if (checkItr == list.end())
				{
					ParseFail* fail = new ParseFail(currentBracket, ParseErrors::MissingBracket);
					_errors.push_back(fail);
					return checkItr;
				}
				itr = checkItr;
				delete* itr;
				continue;
			}
			else if (symbol->symbol() == SymbolType::Function)
			{
				ParseBrackets* brackets = new ParseBrackets(symbol->line());
				brackets->setFromParse(symbol);
				currentBracket->addParse(brackets);
				delete* itr;
				continue;
			}
			else if (symbol->symbol() == SymbolType::CloseBracket)
			{
				currentBracket->setPosition(currentBracket->startingPos(), (*itr)->endingPos());
				currentBracket->setData(currentBracket->line().substr(currentBracket->startingPos(), currentBracket->endingPos() - currentBracket->startingPos()));
				return itr;
			}
		}
		currentBracket->addParse(const_cast<BaseParse*>(*itr));
	}

	if (itr == list.end())
	{
		ParseFail* fail = new ParseFail(currentBracket, ParseErrors::MissingBracket);
		_errors.push_back(fail);
		return itr;
	}
	return itr;
}

bool CScriptParser::parseListBrackets(std::vector<const BaseParse*>& list)
{
	bool error = false;

	std::vector<const BaseParse*> oldList(list);
	list.clear();
	for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
	{
		if (!error && (*itr)->type() == ParseType::Symbol)
		{
			const ParseSymbol* symbol = dynamic_cast<const ParseSymbol*>(*itr);
			if (symbol->symbol() == SymbolType::OpenBracket)
			{
				ParseBrackets* brackets = new ParseBrackets(symbol->line());
				brackets->setLinePosition(symbol->linePos());
				brackets->setFile(_currentFile.back());
				brackets->setPosition(symbol->startingPos(), symbol->endingPos());
				itr = addBracket(itr, oldList, brackets);
				delete symbol;
				list.push_back(brackets);
				if (itr == oldList.end())
					return false;
				delete* itr;
				continue;
			}
			else if (symbol->symbol() == SymbolType::Function)
			{
				ParseBrackets* brackets = new ParseBrackets(symbol->line());
				brackets->setLinePosition(symbol->linePos());
				brackets->setFile(_currentFile.back());
				list.push_back(brackets);
				delete* itr;
				continue;
			}
			else if (symbol->symbol() == SymbolType::CloseBracket)
			{
				ParseFail* fail = new ParseFail(symbol, ParseErrors::MissingStartBracket);
				_errors.push_back(fail);
				error = true;
			}
		}
		list.push_back(*itr);
	}

	return !error;
}

std::vector<const BaseParse*>::iterator CScriptParser::_parseEndArray(std::vector<const BaseParse*>& list, std::vector<const BaseParse*>::iterator startItr, ParseArray* arr)
{
	std::vector<const BaseParse*> internalList;
	const BaseParse* previous = NULL;
	for (auto itr = startItr; itr != list.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
			if (sym->symbol() == SymbolType::OpenArray)
			{
				if (!previous || (previous->type() != ParseType::Function && previous->type() != ParseType::Variable))
				{
					ParseFail* fail = new ParseFail(sym, ParseErrors::InvalidVariable);
					_errors.push_back(fail);
					// Do not push to the outer list — we are inside _parseEndArray
					// which reads from the outer list but must not modify it.
					continue;
				}

				ParseArray* arr = new ParseArray(sym->line());
				arr->setLinePosition(sym->linePos());
				arr->setFile(_currentFile.back());
				arr->setPosition(sym->startingPos(), sym->endingPos());
				arr->setVariable(const_cast<BaseParse*>(previous));

				auto startItr = itr;
				itr = _parseEndArray(list, ++startItr, arr);
				if (itr == list.end())
				{
					delete arr;
					ParseFail* fail = new ParseFail(sym, ParseErrors::MissingEndArray);
					_errors.push_back(fail);
				}
				else
				{
					arr->setPosition(arr->startingPos(), (*itr)->endingPos());
					previous = arr;
					delete (*itr);
					delete sym;
					internalList.pop_back();
					internalList.push_back(arr);
					continue;
				}

			}
			else if (sym->symbol() == SymbolType::CloseArray)
			{
				if (internalList.size() == 0)
				{
					ParseFail* fail = new ParseFail(*startItr, ParseErrors::MissingArraySubscript);
					_errors.push_back(fail);
					return startItr;
				}
				else if (internalList.size() == 1)
				{
					if (internalList.front()->type() == ParseType::Brackets)
					{
						const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(internalList.front());
						ParseExpression* expr = new ParseExpression(brackets->line());
						expr->setFromParse(brackets);
						arr->setValue(expr);
						for (auto iItr = brackets->list().begin(); iItr != brackets->list().end(); iItr++)
							expr->addParse(const_cast<BaseParse*>(*iItr));
						const_cast<ParseBrackets*>(brackets)->clear();
						delete brackets;
					}
					else
						arr->setValue(const_cast<BaseParse*>(internalList.front()));
				}
				else
				{
					ParseExpression* expr = new ParseExpression(internalList.front()->line());
					expr->setLinePosition(internalList.front()->linePos());
					expr->setFile(_currentFile.back());
					expr->setPosition(internalList.front()->startingPos(), internalList.back()->endingPos());
					arr->setValue(expr);
					for (auto iItr = internalList.begin(); iItr != internalList.end(); iItr++)
						expr->addParse(const_cast<BaseParse*>(*iItr));
				}
				return itr;
			}
		}
		internalList.push_back(parse);
		previous = parse;
	}

	return list.end();
}
bool CScriptParser::_findArrays(std::vector<const BaseParse*>& list, bool topLevel)
{
	bool error = false;

	// first, replace the [brackets] with ParseArray
	{
		std::vector<const BaseParse*> oldList(list);
		list.clear();

		const BaseParse* previous = NULL;
		for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
		{
			// if we have an error, just add all the remaining items and move on
			if (error)
			{
				list.push_back(*itr);
				continue;
			}

			const BaseParse* parse = *itr;

			// if we have brackets, then we need do a recursive search inside
			if (parse->type() == ParseType::Brackets)
			{
				const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);

				// parse the brackets list for arrays
				std::vector<const BaseParse*> bracketsList(brackets->constList());

				// Desugar ++/-- and compound assignments inside brackets
				if (!_parseCompoundAssignment(bracketsList))
					error = true;

				if (!error)
					error = !_findArrays(bracketsList, false);

				const_cast<ParseBrackets*>(brackets)->clear();
				// if the brackets only have 1 entry in them, then we dont need to brackets, so just add the data and remove the brackets
				if (bracketsList.size() == 1 && bracketsList.front()->type() == ParseType::Array)
				{
					list.push_back(bracketsList.front());
					delete brackets;
					previous = bracketsList.front();
					continue;
				}

				// add the new list back into the brackets data
				for (auto bItr = bracketsList.begin(); bItr != bracketsList.end(); bItr++)
					const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
			}
			// for functions we also need to check all the arguments
			else if (parse->type() == ParseType::Function)
			{
				const ParseFunction* func = dynamic_cast<const ParseFunction*>(parse);
				if (func->arguments())
				{
					std::vector<const BaseParse*> argList(func->arguments()->constList());
					const_cast<ParseArguments*>(func->arguments())->clear();
					for (auto aItr = argList.begin(); aItr != argList.end(); aItr++)
					{
						auto p = *aItr;
						if (p->type() == ParseType::Expression)
						{
							const ParseExpression* expr = dynamic_cast<const ParseExpression*>(p);
							std::vector<const BaseParse*> exprList(expr->list());
							// Desugar ++/-- inside property setter
							if (!_parseCompoundAssignment(exprList))
								error = true;

							if (!error)
								error = !_parseArrays(exprList, false);

							const_cast<ParseExpression*>(expr)->clearList();
							if (exprList.size() == 1)
							{
								const_cast<ParseArguments*>(func->arguments())->addParse(const_cast<BaseParse*>(exprList.front()));
								delete expr;
							}
							else
							{
								for (auto bItr = exprList.begin(); bItr != exprList.end(); bItr++)
									const_cast<ParseExpression*>(expr)->addParse(const_cast<BaseParse*>(*bItr));
								const_cast<ParseArguments*>(func->arguments())->addParse(const_cast<ParseExpression*>(expr));
							}
						}
						else if (p->type() == ParseType::Brackets)
						{
							const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(p);
							std::vector<const BaseParse*> exprList(brackets->constList());
							// Desugar ++/-- inside bracket arguments
							if (!_parseCompoundAssignment(exprList))
								error = true;

							if (!error)
								error = !_parseArrays(exprList, false);

							const_cast<ParseBrackets*>(brackets)->clear();
							if (exprList.size() == 1)
							{
								const_cast<ParseArguments*>(func->arguments())->addParse(const_cast<BaseParse*>(exprList.front()));
								delete brackets;
							}
							else
							{
								for (auto bItr = exprList.begin(); bItr != exprList.end(); bItr++)
									const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
								const_cast<ParseArguments*>(func->arguments())->addParse(const_cast<ParseBrackets*>(brackets));
							}
						}
						else
							const_cast<ParseArguments*>(func->arguments())->addParse(const_cast<BaseParse*>(p));
					}
				}
			}
			else if (parse->type() == ParseType::Property)
			{
				const ParseProperty* prop = dynamic_cast<const ParseProperty*>(parse);
				if (prop->setter() && prop->setter()->type() == ParseType::Expression)
				{
					const ParseExpression* expr = dynamic_cast<const ParseExpression*>(prop->setter());
					std::vector<const BaseParse*> exprList(expr->list());
					error = !_parseArrays(exprList, false);

					const_cast<ParseExpression*>(expr)->clearList();
					if (exprList.size() == 1)
					{
						const_cast<ParseProperty*>(prop)->setSetter(const_cast<BaseParse*>(exprList.front()));
						delete expr;
					}
					else
					{
						const_cast<ParseExpression*>(expr)->clearList();
						for (auto bItr = exprList.begin(); bItr != exprList.end(); bItr++)
							const_cast<ParseExpression*>(expr)->addParse(const_cast<BaseParse*>(*bItr));
					}
				}
				if (prop->getter() && prop->getter()->type() == ParseType::Expression)
				{
					const ParseExpression* expr = dynamic_cast<const ParseExpression*>(prop->getter());
					std::vector<const BaseParse*> exprList(expr->list());
					// Desugar ++/-- and compound assignments inside brackets
					if (!_parseCompoundAssignment(exprList))
						error = true;

					if (!error)
						error = !_parseArrays(exprList, false);

					const_cast<ParseExpression*>(expr)->clearList();
					if (exprList.size() == 1)
					{
						const_cast<ParseProperty*>(prop)->setGetter(const_cast<BaseParse*>(exprList.front()));
						delete expr;
					}
					else
					{
						const_cast<ParseExpression*>(expr)->clearList();
						for (auto bItr = exprList.begin(); bItr != exprList.end(); bItr++)
							const_cast<ParseExpression*>(expr)->addParse(const_cast<BaseParse*>(*bItr));
					}
				}
			}
			// check fo a symbol, specifically, we are looking for the array brackets '[' and ']'
			else if (parse->type() == ParseType::Symbol)
			{
				const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
				if (sym->symbol() == SymbolType::OpenArray)
				{
					// check if the previous is a bracket
					if (previous && previous->type() == ParseType::Brackets)
					{
						const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(previous);
						if (brackets->list().size() == 1 && (brackets->list().front()->type() == ParseType::Function || brackets->list().front()->type() == ParseType::Array || brackets->list().front()->type() == ParseType::Variable))
						{
							previous = brackets->list().front();
							// clear the list first so it doesn't delete it
							const_cast<ParseBrackets*>(brackets)->clear();
							delete brackets;
						}
					}
					// the previous data should be either a function or a variable (for a function, we use the functions return value as the variable)
					// otherwise its an error
					if (!previous || (previous->type() != ParseType::Function && previous->type() != ParseType::Variable && previous->type() != ParseType::Array))
					{
						ParseFail* fail = new ParseFail(previous ? previous : parse, ParseErrors::InvalidVariable);
						_errors.push_back(fail);
						error = true;
						list.push_back(parse);
						continue;
					}

					// create the array data
					ParseArray* arr = new ParseArray(sym->line());
					arr->setLinePosition(sym->linePos());
					arr->setFile(_currentFile.back());
					arr->setPosition(previous->startingPos(), sym->endingPos());

					// search for the end of the error
					auto startItr = itr;
					itr = _parseEndArray(oldList, ++startItr, arr);

					// the iterator should be the position of the end array ']'
					// if its the end of the list, it means we are missing the symbol
					if (itr == oldList.end())
					{
						delete arr;
						ParseFail* fail = new ParseFail(sym, ParseErrors::MissingEndArray);
						_errors.push_back(fail);
						error = true;
						itr = --startItr;
					}
					// if the iterator is the same as the one we started it, then it failed
					else if (itr == startItr)
					{
						// delete the failed array
						delete arr;
						error = true;
						// add the old data back to the list (so it can be removed later)
						list.push_back(parse);
						list.push_back(*itr);
						continue;
					}
					else
					{
						// set the new array values
						arr->setPosition(arr->startingPos(), (*itr)->endingPos());
						arr->setVariable(const_cast<BaseParse*>(previous));
						previous = arr;
						// remove the previous item from the list, as its now added as part of the array data
						list.pop_back();
						// delete the open and closed brackets items
						delete (*itr);
						delete sym;
						// add the array data to the list
						list.push_back(arr);
						continue;
					}
				}
				// we shouldn't get a close array symbol here
				// if we do, it means we are missing the open one so its an error
				else if (sym->symbol() == SymbolType::CloseArray)
				{
					ParseFail* fail = new ParseFail(sym, ParseErrors::MissingStartArray);
					_errors.push_back(fail);
					error = true;
				}
			}
			// add any other parse data back to the list
			list.push_back(parse);
			previous = parse;
		}
	}

	return !error;
}

bool CScriptParser::_parseArrays(std::vector<const BaseParse*>& list, bool topLevel)
{
	bool error = false;

	// -------------------------------------------------------
	// Pre-pass: detect ++/-- adjacent to ParseArray nodes
	// and split into getter + setter pair with temp variable
	// -------------------------------------------------------
	{
		std::vector<const BaseParse*> oldList(list);
		list.clear();

		const BaseParse* previous = nullptr;
		for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
		{
			const BaseParse* parse = *itr;

			// Post-increment: $array[0]++
			if (parse->type() == ParseType::Array)
			{
				auto nextItr = itr + 1;
				if (nextItr != oldList.end() && (*nextItr)->type() == ParseType::Symbol)
				{
					const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(*nextItr);
					if (sym->symbol() == SymbolType::Increment ||
						sym->symbol() == SymbolType::Decrement)
					{
						std::wstring funcName = (sym->symbol() == SymbolType::Increment)
							? L"inc" : L"dec";
						ParseArray* arr = const_cast<ParseArray*>(
							dynamic_cast<const ParseArray*>(parse));

						ParseVariable* tempVar = _generateTempVariable(parse);

						// Getter: $VarGen.1 = $array[0]
						arr->setAssignment(tempVar);

						// inc/dec on tempVar — deferred
						ParseVariable* tempVarForInc = new ParseVariable(*tempVar);
						tempVarForInc->setFromParse(tempVar);

						ParseFunction* func = new ParseFunction(parse->line(), funcName);
						func->setLinePosition(parse->linePos());
						func->setFile(_currentFile.back());
						func->setPosition(parse->startingPos(), sym->endingPos());
						func->setPostRun(false); // not post-run, runs in deferred list

						ParseArguments* args = new ParseArguments(parse->line());
						args->setLinePosition(parse->linePos());
						args->setFile(_currentFile.back());
						args->setPosition(parse->startingPos(), sym->endingPos());
						args->addParse(tempVarForInc);
						func->setArguments(args);

						// Setter: write tempVar back to array — deferred
						ParseVariable* tempVarForSet = new ParseVariable(*tempVar);
						tempVarForSet->setFromParse(tempVar);

						BaseParse* varCopy = CopyParse(arr->variable());
						BaseParse* valCopy = CopyParse(arr->value());
						if (!valCopy) valCopy = arr->value();

						ParseArray* setArr = new ParseArray(arr->line());
						setArr->setFromParse(arr);
						setArr->setVariable(varCopy);
						setArr->setValue(valCopy);
						setArr->setAssign(tempVarForSet);

						// Store inc and set as a deferred list to run after
						// the current statement completes
						std::vector<const BaseParse*> deferred;
						deferred.push_back(func);
						deferred.push_back(setArr);
						_deferredLists.push_back(deferred);

						delete sym;
						itr = nextItr;

						// Only the getter array goes into the current list
						list.push_back(arr);
						continue;
					}
				}
			}

			// Pre-increment: ++$array[0]  — same structure, func->setPostRun(false)
			if (parse->type() == ParseType::Symbol)
			{
				const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
				if (sym->symbol() == SymbolType::Increment ||
					sym->symbol() == SymbolType::Decrement)
				{
					auto nextItr = itr + 1;
					if (nextItr != oldList.end() &&
						(*nextItr)->type() == ParseType::Array)
					{
						std::wstring funcName = (sym->symbol() == SymbolType::Increment)
							? L"inc" : L"dec";
						ParseArray* arr = const_cast<ParseArray*>(
							dynamic_cast<const ParseArray*>(*nextItr));

						// Check if there's an external assignment target already
						// accumulated in list: [...][Variable][Symbol "="]
						ParseVariable* assignTarget = nullptr;
						if (list.size() >= 2)
						{
							const BaseParse* prevSym = list[list.size() - 1];
							const BaseParse* prevVar = list[list.size() - 2];
							if (prevSym->type() == ParseType::Symbol &&
								dynamic_cast<const ParseSymbol*>(prevSym)->symbol()
								== SymbolType::Assignment &&
								prevVar->type() == ParseType::Variable)
							{
								assignTarget = const_cast<ParseVariable*>(
									dynamic_cast<const ParseVariable*>(prevVar));
							}
						}

						// Use the external variable if available, otherwise temp
						ParseVariable* workVar = nullptr;
						if (assignTarget)
						{
							// Clone it — arr takes ownership of the clone,
							// the original stays in list for the expression pipeline
							workVar = new ParseVariable(*assignTarget);
							workVar->setFromParse(assignTarget);
						}
						else
						{
							workVar = _generateTempVariable(arr);
							arr->setAssignment(workVar);
						}

						// inc on workVar — deferred
						ParseVariable* tempVarForInc = new ParseVariable(*workVar);
						tempVarForInc->setFromParse(workVar);

						ParseFunction* func = new ParseFunction(arr->line(), funcName);
						func->setLinePosition(arr->linePos());
						func->setFile(_currentFile.back());
						func->setPosition(sym->startingPos(), arr->endingPos());
						func->setPostRun(false);

						// set the function to pre assign
						arr->setPreRun(func);

						ParseArguments* args = new ParseArguments(arr->line());
						args->setLinePosition(arr->linePos());
						args->setFile(_currentFile.back());
						args->setPosition(sym->startingPos(), arr->endingPos());
						args->addParse(tempVarForInc);
						func->setArguments(args);

						// Setter: write workVar back to array — deferred
						ParseVariable* tempVarForSet = new ParseVariable(*workVar);
						tempVarForSet->setFromParse(workVar);

						BaseParse* varCopy = CopyParse(arr->variable());
						BaseParse* valCopy = CopyParse(arr->value());
						if (!valCopy) valCopy = arr->value();

						ParseArray* setArr = new ParseArray(arr->line());
						setArr->setFromParse(arr);
						setArr->setVariable(varCopy);
						setArr->setValue(valCopy);
						setArr->setAssign(tempVarForSet);

						std::vector<const BaseParse*> deferred;
						//deferred.push_back(func);
						deferred.push_back(setArr);
						_deferredLists.push_back(deferred);

						delete sym;
						itr = nextItr;

						// Push arr — if assignTarget was found, the variable is
						// already in list and arr->assignment points to it,
						// so the assignment pass sees [Variable][Array(assignment=var)]
						// and links them correctly.
						// If no assignTarget, arr is standalone with tempVar.
						list.push_back(arr);
						continue;
					}
				}
			}
			list.push_back(parse);
			previous = parse;
		}
	}

	// next, find the assignments and add them to arrays
	if (!error)
	{
		std::vector<const BaseParse*> oldList(list);
		list.clear();

		const BaseParse* previous = NULL;
		for (auto itr = oldList.rbegin(); itr != oldList.rend(); itr++)
		{
			const BaseParse* parse = *itr;
			if (error)
			{
				list.push_back(parse);
				continue;
			}

			if (previous && previous->type() == ParseType::Array)
			{
				ParseArray* arr = const_cast<ParseArray*>(dynamic_cast<const ParseArray*>(previous));
				if (!arr->assignment())
				{
					if (parse->type() == ParseType::Condition)
					{
						arr->setPosition(parse->startingPos(), arr->endingPos());
						arr->setAssignment(const_cast<BaseParse*>(parse));
						continue;
					}
					else if (parse->type() == ParseType::Symbol)
					{
						const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
						if (sym->symbol() == SymbolType::Assignment)
						{
							//Missing variable?
							if (++itr == oldList.rend())
							{
								_addError(ParseErrors::MissingArrayVariable, sym);
								delete sym;
								error = true;
								break;
							}
							else if ((*itr)->type() == ParseType::Variable || (*itr)->type() == ParseType::Array)
							{
								delete sym;
								arr->setPosition((*itr)->startingPos(), arr->endingPos());
								arr->setAssignment(const_cast<BaseParse*>(*itr));
								previous = *itr;
								continue;
							}
							else
							{
								_addError(ParseErrors::InvalidAssignmentVariable, *itr);
								delete sym;
								error = true;
								previous = *itr;
								list.push_back(*itr);
								continue;
							}
						}
					}
				}
			}

			list.push_back(parse);
			previous = parse;
		}
		std::reverse(list.begin(), list.end());
	}

	// finally, set any assign values
	if (!error)
	{
		std::vector<const BaseParse*> oldList(list);
		list.clear();

		const BaseParse* previous = NULL;
		ParseExpression* expr = NULL;
		for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
		{
			const BaseParse* parse = *itr;
			if (error)
			{
				list.push_back(parse);
				continue;
			}
			if (previous && previous->type() == ParseType::Array)
			{
				ParseArray* arr = const_cast<ParseArray*>(dynamic_cast<const ParseArray*>(previous));
				if (parse->type() == ParseType::Symbol)
				{
					const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
					if (sym->symbol() == SymbolType::Assignment)
					{
						// missing
						if (++itr == oldList.end())
						{
							_addError(ParseErrors::MissingArrayAssignment, sym);
							error = true;
							delete sym;
							break;
						}
						else
						{
							expr = new ParseExpression((*itr)->line());
							expr->setLinePosition((*itr)->linePos());
							expr->setFile(_currentFile.back());
							expr->setPosition((*itr)->startingPos(), (*itr)->endingPos());
							expr->addParse(const_cast<BaseParse*>(*itr));
							arr->setAssign(expr);
							previous = *itr;
							delete sym;
							continue;
						}
					}
				}
			}

			if (parse && parse->type() == ParseType::Symbol)
			{
				const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
				if (sym->symbol() == SymbolType::OpenBracket)
					expr = NULL;
			}

			if (expr)
			{
				expr->addParse(const_cast<BaseParse*>(parse));
				expr->setPosition(expr->startingPos(), parse->endingPos());
			}
			else
				list.push_back(parse);
			previous = parse;
		}
	}

	// check if the arrays have at least an assignment or assign
	if (!error && topLevel)
	{
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			if ((*itr)->type() == ParseType::Array)
			{
				const ParseArray* arr = dynamic_cast<const ParseArray*>(*itr);
				if (!arr->assign() && !arr->assignment())
				{
					_addError(ParseErrors::InvalidArray, arr);
					error = true;
					break;
				}
			}
		}
	}

	return !error;
}
bool CScriptParser::_parseAllConditions(std::vector<const BaseParse*>& list)
{
	bool error = false;

	std::vector<const BaseParse*> oldList(list);
	list.clear();
	std::vector<const BaseParse*> newList;

	// replace all keywords with conditions
	for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
	{
		if ((*itr)->type() == ParseType::Keyword)
		{
			const ParseKeyword* keyword = dynamic_cast<const ParseKeyword*>(*itr);
			auto cond = parseCondition(keyword->keyword());
			if (cond)
			{
				cond->setLinePosition(keyword->linePos());
				cond->setFile(_currentFile.back());
				cond->setPosition(keyword->startingPos(), keyword->endingPos());
				newList.push_back(cond);
				delete* itr;
				continue;
			}
		}
		newList.push_back(*itr);
	}

	// combine multiple conditions, ie else if
	const BaseParse* previous = NULL;
	for (auto itr = newList.begin(); itr != newList.end(); itr++)
	{
		// add the remaining items to the list so they can be deleted later
		if (error)
		{
			list.push_back(*itr);
			continue;
		}

		// check for 2 conditions next to each other
		if ((*itr)->type() == ParseType::Condition && previous && previous->type() == ParseType::Condition)
		{
			const ParseCondition* c1 = dynamic_cast<const ParseCondition*>(*itr);
			const ParseCondition* c2 = dynamic_cast<const ParseCondition*>(previous);

			// START cannot be combined with any other condition keyword —
			// it is a standalone execution modifier, not a conditional.
			// Catch this before the Not/Else combination logic below.
			if (c2->condition() == Conditions::Start || c1->condition() == Conditions::Start)
			{
				error = true;
				ParseFail* fail = new ParseFail(c2->line(), ParseErrors::InvalidStartCondition);
				fail->setLinePosition(c2->linePos());
				fail->setFile(_currentFile.back());
				fail->setPosition(c2->startingPos(), c1->endingPos());
				_errors.push_back(fail);
			}
			else if (c1->condition() == Conditions::Not)
			{
				ParseCondition* newCond = NULL;
				switch (c2->condition())
				{
				case Conditions::If:
					newCond = new ParseCondition(c1->line(), Conditions::IfNot);
					break;
				case Conditions::ElseIf:
					newCond = new ParseCondition(c1->line(), Conditions::ElseIfNot);
					break;
				case Conditions::While:
					newCond = new ParseCondition(c1->line(), Conditions::WhileNot);
					break;
				case Conditions::SkipIf:
					newCond = new ParseCondition(c1->line(), Conditions::SkipIfNot);
					break;
				}

				if (newCond)
				{
					newCond->setLinePosition(c2->linePos());
					newCond->setFile(_currentFile.back());
					newCond->setPosition(c2->startingPos(), c1->endingPos());
					delete c1;
					delete c2;
					list.pop_back();
					list.push_back(newCond);
					previous = newCond;
					continue;
				}
				// invalid double condition
				else
				{
					error = true;
					ParseFail* fail = new ParseFail(c2->line(), ParseErrors::InvalidDoubleCondition);
					fail->setLinePosition(c2->linePos());
					fail->setFile(_currentFile.back());
					fail->setPosition(c2->startingPos(), c1->endingPos());
					_errors.push_back(fail);
				}
			}
			else if (c2->condition() == Conditions::Else)
			{
				if (c1->condition() == Conditions::If)
				{
					ParseCondition* newCond = new ParseCondition(c1->line(), Conditions::ElseIf);
					newCond->setLinePosition(c2->linePos());
					newCond->setFile(_currentFile.back());
					newCond->setPosition(c2->startingPos(), c1->endingPos());
					delete c1;
					delete c2;
					list.pop_back();
					list.push_back(newCond);
					previous = newCond;
					continue;
				}
				else
				{
					error = true;
					ParseFail* fail = new ParseFail(c2->line(), ParseErrors::InvalidDoubleCondition);
					fail->setFile(_currentFile.back());
					fail->setLinePosition(c2->linePos());
					fail->setPosition(c2->startingPos(), c1->endingPos());
					_errors.push_back(fail);
				}
			}
			else
			{
				error = true;
				ParseFail* fail = new ParseFail(c2->line(), ParseErrors::InvalidDoubleCondition);
				fail->setLinePosition(c2->linePos());
				fail->setFile(_currentFile.back());
				fail->setPosition(c2->startingPos(), c1->endingPos());
				_errors.push_back(fail);
			}
		}
		previous = *itr;
		list.push_back(*itr);
	}

	// finally, check if else comes without an if
	if (!_prePassMode)
	{
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			if ((*itr)->type() == ParseType::Condition)
			{
				const ParseCondition* cond = dynamic_cast<const ParseCondition*>(*itr);
				if (cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot || cond->condition() == Conditions::Else)
				{
					if (!_currentScript->isIfOpen())
					{
						ParseFail* fail = new ParseFail(cond, ParseErrors::MissingIf);
						_errors.push_back(fail);
						error = true;
						break;
					}
				}
			}
		}
	}
	return !error;
}

bool CScriptParser::_parseCompoundAssignment(std::vector<const BaseParse*>& list)
{
	std::vector<const BaseParse*> oldList(list);
	list.clear();
	bool error = false;

	for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
	{
		if (error)
		{
			list.push_back(*itr);
			continue;
		}

		const BaseParse* parse = *itr;

		// -------------------------------------------------------
		// Pre-increment/decrement: ++$var or --$var
		// Operator comes BEFORE the variable
		// -------------------------------------------------------
		if (parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
			SymbolType symType = sym->symbol();

			if (symType == SymbolType::Increment || symType == SymbolType::Decrement)
			{
				auto nextItr = itr + 1;
				if (nextItr != oldList.end() &&
					(*nextItr)->type() == ParseType::Variable)
				{
					// Check if this variable is actually an array subscript target
					// i.e. followed by '[' — if so, skip and let _parseArrays handle it
					auto afterVarItr = nextItr + 1;
					bool isArray = (afterVarItr != oldList.end() &&
						(*afterVarItr)->type() == ParseType::Symbol &&
						dynamic_cast<const ParseSymbol*>(*afterVarItr)->symbol()
						== SymbolType::OpenArray);

					if (!isArray)
					{
						const BaseParse* varNode = *nextItr;
						std::wstring funcName = (symType == SymbolType::Increment)
							? L"inc" : L"dec";

						ParseFunction* func = new ParseFunction(parse->line(), funcName);
						func->setLinePosition(parse->linePos());
						func->setFile(_currentFile.back());
						func->setPosition(parse->startingPos(), varNode->endingPos());
						func->setPostRun(false); // pre — runs immediately

						ParseArguments* args = new ParseArguments(parse->line());
						args->setLinePosition(parse->linePos());
						args->setFile(_currentFile.back());
						args->setPosition(parse->startingPos(), varNode->endingPos());
						args->addParse(const_cast<BaseParse*>(varNode));
						func->setArguments(args);

						delete sym; // consume the ++/-- symbol
						itr = nextItr; // advance past variable

						list.push_back(func);
						continue;
					}
				}
			}

			// Not a pre-increment pattern, pass through
			list.push_back(parse);
			continue;
		}
		else if (parse->type() == ParseType::Brackets)
		{
			const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);
			std::vector<const BaseParse*> bracketsList(brackets->constList());
			error = !_parseCompoundAssignment(bracketsList);

			const_cast<ParseBrackets*>(brackets)->clear();

			// add the new list back into the brackets data
			for (auto bItr = bracketsList.begin(); bItr != bracketsList.end(); bItr++)
				const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
		}

		// -------------------------------------------------------
		// Must be a Variable for post-increment or compound assign
		// -------------------------------------------------------
		if (parse->type() != ParseType::Variable)
		{
			list.push_back(parse);
			continue;
		}

		auto nextItr = itr + 1;
		if (nextItr == oldList.end() || (*nextItr)->type() != ParseType::Symbol)
		{
			list.push_back(parse);
			continue;
		}

		const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(*nextItr);
		SymbolType symType = sym->symbol();

		// -------------------------------------------------------
		// Post-increment/decrement: $var++ or $var--
		// Variable comes BEFORE the operator
		// -------------------------------------------------------
		if (symType == SymbolType::Increment || symType == SymbolType::Decrement)
		{
			std::wstring funcName = (symType == SymbolType::Increment)
				? L"inc" : L"dec";

			ParseFunction* func = new ParseFunction(parse->line(), funcName);
			func->setLinePosition(parse->linePos());
			func->setFile(_currentFile.back());
			func->setPosition(parse->startingPos(), sym->endingPos());
			func->setPostRun(true); // post — deferred via flushPostRun at statement level

			ParseArguments* args = new ParseArguments(parse->line());
			args->setLinePosition(parse->linePos());
			args->setFile(_currentFile.back());
			args->setPosition(parse->startingPos(), sym->endingPos());
			args->addParse(const_cast<BaseParse*>(parse));
			func->setArguments(args);

			delete sym;
			itr = nextItr;

			list.push_back(func);
			continue;
		}

		// -------------------------------------------------------
		// Compound assignment: $var OP= expr
		// Always statement-level, never post-run
		// -------------------------------------------------------
		std::wstring expandedOp;
		switch (symType)
		{
		case SymbolType::PlusAssign:     expandedOp = L"+"; break;
		case SymbolType::MinusAssign:    expandedOp = L"-"; break;
		case SymbolType::MultiplyAssign: expandedOp = L"*"; break;
		case SymbolType::DivideAssign:   expandedOp = L"/"; break;
		default:
			list.push_back(parse);
			continue;
		}

		auto rhsStart = nextItr + 1;
		if (rhsStart == oldList.end())
		{
			_addError(ParseErrors::InvalidAssignment, sym);
			error = true;
			list.push_back(parse);
			continue;
		}

		// Rewrite: $var OP= rhs  →  $var = $var OP rhs
		list.push_back(parse);

		ParseSymbol* assignSym = new ParseSymbol(parse->line(), L"=");
		assignSym->setLinePosition(parse->linePos());
		assignSym->setFile(_currentFile.back());
		assignSym->setPosition(sym->startingPos(), sym->startingPos() + 1);
		list.push_back(assignSym);

		BaseParse* varCopy = CopyParse(parse);
		if (!varCopy)
		{
			_addError(ParseErrors::InternalFunctionError, parse);
			error = true;
			continue;
		}
		list.push_back(varCopy);

		ParseOperator* opNode = new ParseOperator(parse->line(), expandedOp);
		opNode->setLinePosition(parse->linePos());
		opNode->setFile(_currentFile.back());
		opNode->setPosition(sym->startingPos(), sym->endingPos());
		list.push_back(opNode);

		delete sym;
		itr = nextItr;

		for (auto rhsItr = rhsStart; rhsItr != oldList.end(); rhsItr++)
		{
			list.push_back(*rhsItr);
			itr = rhsItr;
		}
	}

	// check for negative values
	if (error) return error;

	oldList = list;
	list.clear();
	const BaseParse* previous = NULL;
	for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (parse->type() == ParseType::Operator && dynamic_cast<const ParseOperator*>(parse)->operType() == Operators::Subtract)
		{
			auto nextItr = itr + 1;
			if (nextItr != oldList.end() && (*nextItr)->type() == ParseType::Integer && previous && previous->type() == ParseType::Operator)
			{
				itr = nextItr;
				ParseInteger* intNode = const_cast<ParseInteger*>(dynamic_cast<const ParseInteger*>(*nextItr));
				intNode->negate();
				list.push_back(intNode);
				delete parse; // consume the minus operator
				previous = intNode;
				continue;
			}
		}

		list.push_back(parse);
		previous = parse;
	}


	return !error;
}

bool CScriptParser::_parsePreprocessor(std::vector<const BaseParse*>& list)
{
	bool error = false;

	// preprocessor always starts with a '#'
	if (!list.empty() && list.front()->type() == ParseType::Symbol)
	{
		auto symb = dynamic_cast<const ParseSymbol*>(list.front());
		if (symb->symbol() == SymbolType::Preprocessor)
		{
			if (list.size() < 2)
			{
				_addError(ParseErrors::MissingPreprocessor, symb);
				return false;
			}
			if (list[1]->type() != ParseType::Keyword)
			{
				_addError(ParseErrors::InvalidPreprocessor, list[1]);
				return false;
			}

			auto keyword = dynamic_cast<const ParseKeyword*>(list[1]);
			if (keyword->keyword() == L"define")
			{
				if (list.size() < 2)
				{
					_addError(ParseErrors::MissingDefine, list[1]);
					return false;
				}
				if (list.size() >= 3 && list[2]->type() != ParseType::Keyword)
				{
					_addError(ParseErrors::InvalidDefine, list[2]);
					return false;
				}

				// #define TEST  (no value) — valid, defines the symbol as empty for #ifdef use
				if (list.size() < 3)
				{
					ParseDefine* define = new ParseDefine(dynamic_cast<const ParseKeyword*>(list[1]));
					delete symb;
					delete keyword;
					list.clear();
					_defines[define->define()] = define;
					return true;
				}

				ParseDefine* define = new ParseDefine(dynamic_cast<const ParseKeyword*>(list[2]));

				delete symb;
				delete keyword;
				delete list[2];
				// remove them from the list
				list.erase(list.begin());
				list.erase(list.begin());
				list.erase(list.begin());

				// contains variables
				if (!list.empty() && list.front()->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(list.front())->symbol() == SymbolType::OpenBracket)
				{
					auto bracket = dynamic_cast<const ParseSymbol*>(list.front());
					list.erase(list.begin());

					auto itr = list.begin();
					bool expectedKeyword = true;
					while (itr != list.end() && !((*itr)->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(*itr)->symbol() == SymbolType::CloseBracket))
					{
						if (expectedKeyword)
						{
							if ((*itr)->type() == ParseType::Keyword)
							{
								define->addVariable(dynamic_cast<const ParseKeyword*>(*itr));
								expectedKeyword = false;
							}
							else
							{
								_addError(ParseErrors::InvalidKeyword, *itr);
								delete bracket;
								return false;
							}
						}
						else
						{
							if ((*itr)->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(*itr)->symbol() == SymbolType::Comma)
								expectedKeyword = true;
							else
							{
								_addError(ParseErrors::InvalidComma, *itr);
								delete bracket;
								return false;
							}
						}
						delete* itr;
						itr = list.erase(itr);
					}

					// first item should be a close bracket, if not, its missing
					if (list.empty() || (list.front()->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(list.front())->symbol() != SymbolType::CloseBracket))
					{
						_addError(ParseErrors::MissingBracket, bracket);
						delete bracket;
						return false;
					}

					if (expectedKeyword)
					{
						_addError(ParseErrors::MissingFunctionArgument, *itr);
						delete bracket;
						return false;
					}

					delete bracket;

					delete list.front();
					list.erase(list.begin());
				}

				for (auto itr = list.begin(); itr != list.end(); itr++)
				{
					bool found = false;
					// replace any keywords in previous defines
					if ((*itr)->type() == ParseType::Keyword)
					{
						auto keyword = dynamic_cast<const ParseKeyword*>(*itr);
						for (auto dItr = _defines.begin(); dItr != _defines.end(); dItr++)
						{
							if (keyword->keyword() == dItr->first)
							{
								auto& list = dItr->second->list();
								for (auto pItr = list.begin(); pItr != list.end(); pItr++)
									define->addParse(CScriptParser::CopyParse(*pItr));
								found = true;
								break;
							}
						}
					}

					if (!found)
						define->addParse(*itr);
				}

				_defines[define->define()] = define;
				list.clear();
			}
			else if (keyword->keyword() == L"undef")
			{
				if (list.size() < 3)
				{
					_addError(ParseErrors::MissingDefine, list[1]);
					return false;
				}
				if (list[2]->type() != ParseType::Keyword)
				{
					_addError(ParseErrors::InvalidDefine, list[2]);
					return false;
				}
				if (list.size() > 3)
				{
					_addError(ParseErrors::TooMuchData, list[3]);
					return false;
				}

				const ParseKeyword* keyword = dynamic_cast<const ParseKeyword*>(list[2]);
				auto itr = _defines.find(keyword->keyword());
				if (itr != _defines.end())
					_defines.erase(itr);
			}
			else if (keyword->keyword() == L"ifdef" || keyword->keyword() == L"ifndef")
			{
				if (list.size() < 3 || list[2]->type() != ParseType::Keyword)
				{
					_addError(ParseErrors::MissingDefine, list[1]);
					return false;
				}

				const ParseKeyword* nameKey = dynamic_cast<const ParseKeyword*>(list[2]);
				bool isDefined = (_defines.find(nameKey->keyword()) != _defines.end());

				// Optional comparison: #ifdef NAME == value  or  #ifdef NAME != value
				bool condResult = isDefined;
				if (list.size() >= 5 && list[3]->type() == ParseType::Symbol)
				{
					const ParseSymbol* op = dynamic_cast<const ParseSymbol*>(list[3]);
					std::wstring compareVal;
					for (size_t ci = 4; ci < list.size(); ci++)
						compareVal += list[ci]->data();

					std::wstring defineVal;
					auto defItr = _defines.find(nameKey->keyword());
					if (defItr != _defines.end() && !defItr->second->list().empty())
						defineVal = defItr->second->list().front()->data();

					std::wstring opStr = op->stringData();
					if (opStr == L"==" || opStr == L"=")
						condResult = (defineVal == compareVal);
					else if (opStr == L"!=")
						condResult = (defineVal != compareVal);
					else if (opStr == L">")
						condResult = (std::stod(defineVal) > std::stod(compareVal));
					else if (opStr == L"<")
						condResult = (std::stod(defineVal) < std::stod(compareVal));
					else if (opStr == L">=")
						condResult = (std::stod(defineVal) >= std::stod(compareVal));
					else if (opStr == L"<=")
						condResult = (std::stod(defineVal) <= std::stod(compareVal));
				}

				if (keyword->keyword() == L"ifndef")
					condResult = !condResult;

				_ifDefStack.push_back({ condResult, condResult });
				list.clear();
			}
			else if (keyword->keyword() == L"elseif" || keyword->keyword() == L"elseifdef" || keyword->keyword() == L"elseifndef")
			{
				if (_ifDefStack.empty())
				{
					_addError(ParseErrors::MissingIf, list[1]);
					return false;
				}
				if (list.size() < 3 || list[2]->type() != ParseType::Keyword)
				{
					_addError(ParseErrors::MissingDefine, list[1]);
					return false;
				}

				IfDefEntry& top = _ifDefStack.back();
				if (top.anyActive)
				{
					// A previous branch was already active — skip this one
					top.active = false;
				}
				else
				{
					const ParseKeyword* nameKey = dynamic_cast<const ParseKeyword*>(list[2]);
					bool isDefined = (_defines.find(nameKey->keyword()) != _defines.end());
					bool condResult = isDefined;

					if (list.size() >= 5 && list[3]->type() == ParseType::Symbol)
					{
						const ParseSymbol* op = dynamic_cast<const ParseSymbol*>(list[3]);
						std::wstring compareVal;
						for (size_t ci = 4; ci < list.size(); ci++)
							compareVal += list[ci]->data();

						std::wstring defineVal;
						auto defItr = _defines.find(nameKey->keyword());
						if (defItr != _defines.end() && !defItr->second->list().empty())
							defineVal = defItr->second->list().front()->data();

						std::wstring opStr = op->stringData();
						if (opStr == L"==" || opStr == L"=")
							condResult = (defineVal == compareVal);
						else if (opStr == L"!=")
							condResult = (defineVal != compareVal);
						else if (opStr == L">")
							condResult = (std::stod(defineVal) > std::stod(compareVal));
						else if (opStr == L"<")
							condResult = (std::stod(defineVal) < std::stod(compareVal));
						else if (opStr == L">=")
							condResult = (std::stod(defineVal) >= std::stod(compareVal));
						else if (opStr == L"<=")
							condResult = (std::stod(defineVal) <= std::stod(compareVal));
					}

					if (keyword->keyword() == L"elseifndef")
						condResult = !condResult;

					top.active = condResult;
					if (condResult) top.anyActive = true;
				}
				list.clear();
			}
			else if (keyword->keyword() == L"else")
			{
				if (_ifDefStack.empty())
				{
					_addError(ParseErrors::MissingIf, list[1]);
					return false;
				}
				IfDefEntry& top = _ifDefStack.back();
				top.active = !top.anyActive;
				list.clear();
			}
			else if (keyword->keyword() == L"endif")
			{
				if (_ifDefStack.empty())
				{
					_addError(ParseErrors::MissingIf, list[1]);
					return false;
				}
				_ifDefStack.pop_back();
				list.clear();
			}
			else if (keyword->keyword() == L"include")
			{
				if (list.size() < 3)
				{
					_addError(ParseErrors::MissingPreprocessor, list[1]);
					return false;
				}

				// Argument must be a string: #include "myfile.xs"
				if (list[2]->type() != ParseType::String)
				{
					_addError(ParseErrors::InvalidPreprocessor, list[2]);
					return false;
				}

				std::wstring includeName = dynamic_cast<const ParseString*>(list[2])->stringData();

				// Resolve path relative to the current file's directory
				std::wstring includePath = includeName;
				if (!_currentFile.empty())
				{
					std::wstring currentDir = _currentFile.back();
					size_t lastSlash = currentDir.find_last_of(L"\\/");
					if (lastSlash != std::wstring::npos)
						includePath = currentDir.substr(0, lastSlash + 1) + includeName;
				}

				list.clear();

				if (!includeFile(includePath))
					return false;
			}
			else if (keyword->keyword() == L"DESCRIPTION" ||
				keyword->keyword() == L"VERSION" ||
				keyword->keyword() == L"COMMAND")
			{
				if (list.size() < 3)
				{
					_addError(ParseErrors::MissingSpecialArgument, list[1]);
					return false;
				}

				const BaseParse* arg = list[2];

				if (keyword->keyword() == L"DESCRIPTION")
				{
					if (arg->type() != ParseType::String)
					{
						_addError(ParseErrors::InvalidArgumentDataType, arg);
						return false;
					}
					_currentScript->setDescription(dynamic_cast<const ParseString*>(arg)->stringData());
				}
				else if (keyword->keyword() == L"VERSION")
				{
					if (arg->type() != ParseType::Integer)
					{
						_addError(ParseErrors::InvalidArgumentDataType, arg);
						return false;
					}
					_currentScript->setVersion(dynamic_cast<const ParseInteger*>(arg)->value());
				}
				else if (keyword->keyword() == L"COMMAND")
				{
					if (arg->type() == ParseType::Constant)
					{
						const ParseConstant* c = dynamic_cast<const ParseConstant*>(arg);
						if (c->dataType() != DataTypes::ObjectCommand)
						{
							_addError(ParseErrors::InvalidArgumentDataType, arg);
							return false;
						}
						_currentScript->setCommand(c->id());
					}
					else if (arg->type() == ParseType::Integer)
					{
						_currentScript->setCommand(dynamic_cast<const ParseInteger*>(arg)->value());
					}
					else
					{
						_addError(ParseErrors::InvalidArgumentDataType, arg);
						return false;
					}
				}

				list.clear();
			}
			else if (keyword->keyword() == L"datatype")
			{
				// #datatype $varname DATATYPE_X
				// #datatype $varname DATATYPE_X|DATATYPE_Y
				// Sets the type hint for $varname so object method resolution works correctly.
				if (list.size() < 4)
				{
					_addError(ParseErrors::MissingSpecialArgument, list[1]);
					return false;
				}

				// At preprocessor time, variables may still be ParseKeyword (with $ prefix)
				// rather than ParseVariable — accept both
				std::wstring varName;
				if (list[2]->type() == ParseType::Variable)
				{
					varName = dynamic_cast<const ParseVariable*>(list[2])->name();
				}
				else if (list[2]->type() == ParseType::Keyword)
				{
					const ParseKeyword* kw = dynamic_cast<const ParseKeyword*>(list[2]);
					if (kw->keyword().empty() || kw->keyword()[0] != L'$')
					{
						_addError(ParseErrors::InvalidVariable, list[2]);
						return false;
					}
					varName = kw->keyword();
				}
				else
				{
					_addError(ParseErrors::InvalidVariable, list[2]);
					return false;
				}
				std::unordered_set<DataTypes> types;

				// Collect all datatype tokens — separated by '|' symbols
				for (size_t ti = 3; ti < list.size(); ti++)
				{
					const BaseParse* token = list[ti];
					// Skip '|' operator tokens (separators)
					if (token->type() == ParseType::Operator)
						continue;
					if (token->type() == ParseType::Symbol)
						continue;

					// Resolve the datatype name
					std::wstring dtName = token->data();
					const DataTypeData* dtData = _data->findDatatype(dtName);
					if (dtData)
					{
						types.insert(dtData->id);
					}
					else
					{
						ParseFail* fail = new ParseFail(token, ParseErrors::InvalidArgumentDataType);
						fail->addData(dtName);
						_errors.push_back(fail);
						return false;
					}
				}

				if (!types.empty())
					(*_pVariables)[varName] = types;

				list.clear();
			}
			// invalid preprocessor
			else
			{
				_addError(ParseErrors::UnknownPreprocessor, list[1]);
				return false;
			}
		}
	}

	return !error;
}
bool CScriptParser::_parseNamespaces(std::vector<const BaseParse*>& list)
{
	std::vector<const BaseParse*> oldList(list);
	list.clear();

	bool error = false;

	const BaseParse* previous = NULL;
	for (auto itr = oldList.begin(); itr != oldList.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (error)
		{
			list.push_back(parse);
			continue;
		}

		if (parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* symb = dynamic_cast<const ParseSymbol*>(parse);
			if (symb->symbol() == SymbolType::Namespace)
			{
				if (!previous || previous->type() != ParseType::Keyword || symb->hasWhitespaceBefore())
				{
					delete parse;
					continue;
				}
				else if ((++itr) == oldList.end() || (*itr)->type() != ParseType::Keyword || symb->hasWhitespaceAfter())
				{
					_addError(ParseErrors::InvalidNamespace, parse);
					error = true;
				}
				else
				{
					ParseNamespace* ns = new ParseNamespace(parse->line());
					ns->setPosition(previous->startingPos(), (*itr)->endingPos());
					ns->setKeyword((*itr)->data());
					ns->setNamespace(previous->data());
					list.pop_back();
					list.push_back(ns);
					delete parse;
					delete previous;
					delete* itr;
					previous = ns;
					continue;
				}
			}
		}

		list.push_back(parse);
		previous = parse;
	}

	return !error;
}

bool CScriptParser::findProperties(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>& newList)
{
	bool error = false;
	const BaseParse* previous = NULL;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		if (error)
		{
			newList.push_back(*itr);
			continue;
		}

		const BaseParse* parse = *itr;
		if (previous && parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* symb = dynamic_cast<const ParseSymbol*>(*itr);
			if (symb->symbol() == SymbolType::Object)
			{
				if (++itr == list.end())
				{
					newList.push_back(symb);
					error = true;
					_addError(ParseErrors::MissingObjectFunction, symb);
					break;
				}

				if ((*itr)->type() != ParseType::Keyword)
				{
					_addError(ParseErrors::InvalidObject, *itr);
					error = true;
					newList.push_back(symb);
					newList.push_back(*itr);
					previous = *itr;
					continue;
				}

				const ParseKeyword* keyword = dynamic_cast<const ParseKeyword*>(*itr);

				// if the keyword is followed by brackets, then its a function, otherwise its a property
				if ((itr + 1) != list.end() && (*(itr + 1))->type() == ParseType::Brackets)
				{
					newList.push_back(*(itr - 1));
					newList.push_back(*itr);
					previous = *itr;
					continue;
				}

				delete symb;
				newList.pop_back();
				ParseProperty* prop = new ParseProperty(keyword->line(), keyword->keyword());
				prop->setLinePosition(keyword->linePos());
				prop->setFile(_currentFile.back());
				prop->setPosition(previous->startingPos(), keyword->endingPos());
				prop->setObject(const_cast<BaseParse*>(previous));

				delete keyword;
				previous = prop;
				newList.push_back(prop);
				continue;
			}
		}
		// if we have brackets, then we need do a recursive search inside
		else if (parse->type() == ParseType::Brackets)
		{
			const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);

			// parse the brackets list for arrays
			std::vector<const BaseParse*> bracketsList;
			error = !findProperties(brackets->constList(), bracketsList);

			const_cast<ParseBrackets*>(brackets)->clear();
			// if the brackets only have 1 entry in them, then we dont need to brackets, so just add the data and remove the brackets
			if (bracketsList.size() == 1 && bracketsList.front()->type() == ParseType::Array)
			{
				newList.push_back(bracketsList.front());
				delete brackets;
				previous = bracketsList.front();
				continue;
			}

			// add the new list back into the brackets data
			for (auto bItr = bracketsList.begin(); bItr != bracketsList.end(); bItr++)
				const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
		}

		newList.push_back(*itr);
		previous = *itr;
	}

	return !error;
}

bool CScriptParser::parseProperties(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>& newList)
{
	bool error = false;
	const BaseParse* previous = NULL;
	ParseExpression* setterProperty = NULL;
	std::vector<const BaseParse*> getterArray;

	for (auto itr = list.rbegin(); itr != list.rend(); itr++)
	{
		if (error)
		{
			newList.push_back(*itr);
			continue;
		}

		const BaseParse* parse = *itr;

		if ((*itr)->type() == ParseType::Symbol &&
			dynamic_cast<const ParseSymbol*>(*itr)->symbol() == SymbolType::Assignment)
		{
			const ParseSymbol* symb = dynamic_cast<const ParseSymbol*>(*itr);

			// Check what's to the left (next in reverse = what comes after itr)
			auto nextItr = itr + 1;
			const BaseParse* leftToken = (nextItr != list.rend()) ? *nextItr : nullptr;

			// Determine if either side involves a property
			bool rightIsProperty = previous && previous->type() == ParseType::Property;
			bool leftIsProperty = leftToken && leftToken->type() == ParseType::Property;

			// Neither side is a property — this is a plain assignment, leave it alone
			if (!rightIsProperty && !leftIsProperty)
			{
				if (setterProperty)
				{
					setterProperty->addParse(const_cast<BaseParse*>(parse));
					setterProperty->setFile(_currentFile.back());
					setterProperty->setLinePosition(parse->linePos());
					setterProperty->setPosition(parse->startingPos(), setterProperty->endingPos());
				}
				else
					newList.push_back(parse);
				previous = parse;
				continue;
			}

			// must have a value before the assignment symbol
			if (!previous)
			{
				_addError(ParseErrors::InvalidAssignment, symb);
				error = true;
				newList.push_back(symb);
				continue;
			}

			if (++itr == list.rend())
			{
				_addError(ParseErrors::MissingAssignment, symb);
				error = true;
				newList.push_back(symb);
				return false;
			}

			// setter — right side is a property
			if ((*itr)->type() == ParseType::Property)
			{
				ParseProperty* prop = const_cast<ParseProperty*>(
					dynamic_cast<const ParseProperty*>(*itr));

				ParseExpression* expr = new ParseExpression(prop->line());
				if (setterProperty)
				{
					expr->setLinePosition(setterProperty->linePos());
					expr->setFile(_currentFile.back());
					expr->setPosition(setterProperty->startingPos(),
						setterProperty->endingPos());
					for (auto eItr = setterProperty->list().rbegin();
						eItr != setterProperty->list().rend(); eItr++)
						expr->addParse(const_cast<BaseParse*>(*eItr));
					setterProperty->clearList();
					delete setterProperty;
					setterProperty = NULL;
				}
				else
				{
					if (previous->type() == ParseType::Variable)
					{
						const ParseVariable* vari =
							dynamic_cast<const ParseVariable*>(previous);
						ParseVariable* newVari = new ParseVariable(
							vari->line(), vari->data(), &vari->currentDataTypes());
						newVari->setFromParse(vari);
						previous = newVari;
					}
					else
						newList.pop_back();

					expr->setLinePosition(parse->linePos());
					expr->setFile(_currentFile.back());
					expr->setPosition(parse->startingPos(), parse->endingPos());
					expr->addParse(const_cast<BaseParse*>(previous));
				}
				prop->setSetter(expr);
				newList.push_back(prop);
				delete symb;
				previous = *itr;
				continue;
			}
			else if ((*itr)->type() == ParseType::Expression &&
				(*itr)->type() != ParseType::Property)
			{
				ParseExpression* prevExpr = const_cast<ParseExpression*>(
					dynamic_cast<const ParseExpression*>(*itr));
				if (prevExpr->size() == 1)
				{
					auto item = prevExpr->list().at(0);
					if (item->type() == ParseType::Property)
					{
						ParseProperty* itemProp = const_cast<ParseProperty*>(
							dynamic_cast<const ParseProperty*>(item));
						ParseExpression* expr = new ParseExpression(itemProp->line());
						expr->setLinePosition(parse->linePos());
						expr->setFile(_currentFile.back());
						expr->setPosition(parse->startingPos(), parse->endingPos());
						itemProp->setSetter(expr);
						expr->addParse(const_cast<BaseParse*>(previous));
						previous = *itr;
						setterProperty = NULL;
						delete symb;
						continue;
					}
					else if (!(item->type() == ParseType::Symbol &&
						dynamic_cast<const ParseSymbol*>(item)->symbol() ==
						SymbolType::CloseArray))
					{
						_addError(ParseErrors::InvalidReturnValue, item);
						error = true;
					}
				}
				else
				{
					_addError(ParseErrors::InvalidReturnValue, parse);
					error = true;
				}
			}
			// getter
			else
			{
				if (previous && previous->type() == ParseType::Property)
				{
					ParseProperty* prop = const_cast<ParseProperty*>(
						dynamic_cast<const ParseProperty*>(previous));

					if ((*itr)->type() == ParseType::Expression)
					{
						ParseExpression* expr = const_cast<ParseExpression*>(
							dynamic_cast<const ParseExpression*>(*itr));
						if (expr->size() == 1)
						{
							auto item = expr->list().at(0);
							if (item->type() == ParseType::Variable)
							{
								ParseVariable* itemVar = const_cast<ParseVariable*>(
									dynamic_cast<const ParseVariable*>(item));
								ParseVariable* newVar = new ParseVariable(
									itemVar->line(), itemVar->name(),
									&itemVar->currentDataTypes());
								newVar->setFromParse(itemVar);
								prop->setGetter(newVar);
								delete symb;
								continue;
							}
							else if (item->type() == ParseType::Property)
							{
								ParseProperty* itemProp = const_cast<ParseProperty*>(
									dynamic_cast<const ParseProperty*>(item));
								ParseExpression* expr = new ParseExpression(
									itemProp->line());
								expr->setLinePosition(parse->linePos());
								expr->setFile(_currentFile.back());
								expr->setPosition(parse->startingPos(),
									parse->endingPos());
								itemProp->setSetter(expr);
								expr->addParse(const_cast<BaseParse*>(*itr));
								setterProperty = expr;
								previous = expr;
								delete symb;
								continue;
							}
							else if (!(item->type() == ParseType::Symbol &&
								dynamic_cast<const ParseSymbol*>(item)->symbol()
								== SymbolType::CloseArray))
							{
								_addError(ParseErrors::InvalidReturnValue, item);
								error = true;
							}
						}
						else
						{
							_addError(ParseErrors::InvalidReturnValue, parse);
							error = true;
						}
					}
					else if ((*itr)->type() == ParseType::Array)
					{
						ParseArray* arr = const_cast<ParseArray*>(
							dynamic_cast<const ParseArray*>(*itr));
						arr->setAssign(prop);
						newList.pop_back();
						newList.push_back(arr);
						delete symb;
					}
					else if ((*itr)->type() == ParseType::Variable ||
						(*itr)->type() == ParseType::Property)
					{
						prop->setGetter(const_cast<BaseParse*>(*itr));
						newList.pop_back();
						newList.push_back(prop);
						delete symb;
					}
					else if ((*itr)->type() == ParseType::Symbol &&
						dynamic_cast<const ParseSymbol*>(*itr)->symbol() ==
						SymbolType::CloseArray)
					{
						newList.pop_back();
						getterArray.clear();
						getterArray.push_back(previous);
						getterArray.push_back(symb);
						getterArray.push_back(*itr);
					}
					else
					{
						delete symb;
						_addError(ParseErrors::InvalidReturnValue, *itr);
						newList.push_back(*itr);
						error = true;
					}
				}
				// Left side is a property but right (previous) isn't — 
				// still a plain assignment, leave it alone
				else
				{
					if (setterProperty)
					{
						setterProperty->addParse(const_cast<BaseParse*>(parse));
						setterProperty->setFile(_currentFile.back());
						setterProperty->setLinePosition(parse->linePos());
						setterProperty->setPosition(parse->startingPos(),
							setterProperty->endingPos());
					}
					else
						newList.push_back(parse);
					// push back the token we advanced past
					newList.push_back(*itr);
					itr--; // undo the advance since we didn't consume it
					previous = parse;
					continue;
				}
			}
		}
		else if (!getterArray.empty())
		{
			getterArray.push_back(*itr);
		}
		else if (setterProperty)
		{
			setterProperty->addParse(const_cast<BaseParse*>(*itr));
			setterProperty->setFile(_currentFile.back());
			setterProperty->setLinePosition((*itr)->linePos());
			setterProperty->setPosition((*itr)->startingPos(),
				setterProperty->endingPos());
		}
		else if ((*itr)->type() == ParseType::Property)
			newList.push_back(*itr);
		else
		{
			setterProperty = new ParseExpression((*itr)->line());
			setterProperty->setLinePosition((*itr)->linePos());
			setterProperty->setFile(_currentFile.back());
			setterProperty->setPosition((*itr)->startingPos(), (*itr)->endingPos());
			setterProperty->addParse(const_cast<BaseParse*>(*itr));
		}
		previous = *itr;
	}

	if (!getterArray.empty())
	{
		for (auto itr = getterArray.begin(); itr != getterArray.end(); itr++)
			newList.insert(newList.begin(), *itr);
		getterArray.clear();
	}

	if (setterProperty)
	{
		for (auto eItr = setterProperty->list().rbegin();
			eItr != setterProperty->list().rend(); eItr++)
			newList.push_back(*eItr);
		setterProperty->clearList();
		delete setterProperty;
	}

	return !error;
}
bool CScriptParser::parseConstants(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>& newList)
{
	bool error = false;
	const BaseParse* previous = NULL;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		if (error)
		{
			newList.push_back(*itr);
			continue;
		}

		if ((*itr)->type() == ParseType::Keyword)
		{
			const ParseKeyword* keyword = dynamic_cast<const ParseKeyword*>(*itr);

			// special handling for goto command
			if (previous && previous->type() == ParseType::Keyword)
			{
				const ParseKeyword* prevKeyword = dynamic_cast<const ParseKeyword*>(previous);

				auto func = _data->findGlobalFunction(prevKeyword->keyword());
				if (func && (func->id == _data->gotoCommand() || func->id == _data->gosubCommand()))
				{
					newList.push_back(new ParseLabel(keyword));
					previous = newList.back();
					delete* itr;
					continue;
				}
			}
			// ignore properties/object function
			if (previous && previous->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(previous)->symbol() == SymbolType::Object)
			{
				newList.push_back(*itr);
				previous = *itr;
				continue;
			}

			// first check if its a condition
			auto condition = parseCondition(keyword->keyword());
			if (condition)
			{
				condition->setFromParse(keyword);
				newList.push_back(condition);
			}
			// is a function
			else if ((itr + 1) != list.end() && (*(itr + 1))->type() == ParseType::Brackets)
			{
				newList.push_back(*itr);
				previous = *itr;
				continue;
			}
			else
			{
				// now check for the constant
				if (_data->findSpecialKeyword(keyword->keyword()))
				{
					newList.push_back(*itr);
					previous = *itr;
					continue;
				}

				auto constant = parseConstant(keyword->keyword());
				constant->setFromParse(keyword);
				if (constant->type() == ParseType::Failed)
				{
					_errors.push_back(dynamic_cast<ParseFail*>(constant));
					error = true;
				}
				else
				{
					newList.push_back(constant);
					if (constant->type() == ParseType::Variable)
					{
						const ParseVariable* vari = dynamic_cast<const ParseVariable*>(constant);
						const wchar_t c = vari->name().at(1);
						if ((c >= '0' && c <= '9') || c == '.')
						{
							error = true;
							ParseFail* fail = new ParseFail(constant, ParseErrors::InvalidVariableName);
							fail->addData(vari->name());
							_errors.push_back(fail);
						}
					}
				}
			}
			delete* itr;
			continue;
		}
		else if ((*itr)->type() == ParseType::Namespace)
		{
			// if next is brackets, pass through — parseFunctions will handle the function call
			if ((itr + 1) != list.end() && (*(itr + 1))->type() == ParseType::Brackets)
			{
				newList.push_back(*itr);
				previous = *itr;
				continue;
			}

			const ParseNamespace* ns = dynamic_cast<const ParseNamespace*>(*itr);
			const ConstantData* c = _data->findConstant(ns->namespaceString(), ns->keyword());
			if (c)
			{
				ParseConstant* constant = new ParseConstant(ns->line(), c);
				constant->setLinePosition(ns->linePos());
				constant->setFile(_currentFile.back());
				constant->setPosition(ns->startingPos(), ns->endingPos());
				newList.push_back(constant);
				delete ns;
				previous = constant;
				continue;
			}

			_addError(ParseErrors::UnknownConstant, ns);
			delete ns;
			error = true;
			continue;
		}
		else if ((*itr)->type() == ParseType::Brackets)
		{
			ParseBrackets* brackets = const_cast<ParseBrackets*>(dynamic_cast<const ParseBrackets*>(*itr));

			// Resolve namespace constants (e.g. RaceFlag::NPC) inside brackets
			// before recursing into parseConstants — _parseNamespaces was only
			// run on the top-level list in _parseDataList, not inside brackets.
			std::vector<const BaseParse*> nsResolved(brackets->constList());
			_parseNamespaces(nsResolved);
			brackets->clear();
			for (auto bItr = nsResolved.begin(); bItr != nsResolved.end(); bItr++)
				brackets->addParse(const_cast<BaseParse*>(*bItr));

			std::vector<const BaseParse*> createdList;
			if (!parseConstants(brackets->constList(), createdList))
				error = true;

			brackets->clear();
			for (auto bItr = createdList.begin(); bItr != createdList.end(); bItr++)
				brackets->addParse(const_cast<BaseParse*>(*bItr));
		}

		newList.push_back(*itr);
		previous = *itr;
	}

	return !error;
}

bool CScriptParser::_checkListOrder(const BaseParse* parse, ParseErrors errorType)
{
	if (parse->type() == ParseType::Brackets)
	{
		if (!_checkListOrder(dynamic_cast<const ParseBrackets*>(parse)->constList(), errorType))
			return false;
	}
	else if (parse->type() == ParseType::Array)
	{
		const ParseArray* arr = dynamic_cast<const ParseArray*>(parse);
		if (arr->value() && arr->value()->type() == ParseType::Brackets)
		{
			if (!_checkListOrder(dynamic_cast<const ParseBrackets*>(arr->value())->constList(), ParseErrors::DataBeforeEndArray))
				return false;
		}
		else if (arr->value() && arr->value()->type() == ParseType::Expression)
		{
			if (!_checkListOrder(dynamic_cast<const ParseExpression*>(arr->value())->list(), ParseErrors::DataBeforeEndArray))
				return false;
		}
		if (arr->value2() && arr->value2()->type() == ParseType::Brackets)
		{
			if (!_checkListOrder(dynamic_cast<const ParseBrackets*>(arr->value2())->constList(), ParseErrors::DataBeforeEndArray))
				return false;
		}
		else if (arr->value2() && arr->value2()->type() == ParseType::Expression)
		{
			if (!_checkListOrder(dynamic_cast<const ParseExpression*>(arr->value2())->list(), ParseErrors::DataBeforeEndArray))
				return false;
		}
		if (arr->assign() && arr->assign()->type() == ParseType::Brackets)
		{
			if (!_checkListOrder(dynamic_cast<const ParseBrackets*>(arr->assign())->constList(), errorType))
				return false;
		}
		if (arr->assign() && arr->assign()->type() == ParseType::Expression)
		{
			if (!_checkListOrder(dynamic_cast<const ParseExpression*>(arr->assign())->list(), errorType))
				return false;
		}
	}
	else if (parse->type() == ParseType::Function)
	{
		const ParseFunction* func = dynamic_cast<const ParseFunction*>(parse);
		if (func->arguments())
		{
			for (unsigned int i = 0; i < func->arguments()->count(); i++)
			{
				auto p = func->arguments()->get(i);
				if (p->type() == ParseType::Expression)
				{
					if (!_checkListOrder(dynamic_cast<const ParseExpression*>(p)->list(), errorType))
						return false;
				}
			}
		}
	}
	else if (parse->type() == ParseType::Property)
	{
		const ParseProperty* prop = dynamic_cast<const ParseProperty*>(parse);
		if (prop->setter())
		{
			if (!_checkListOrder(prop->setter(), errorType))
				return false;
		}
		else if (prop->getter())
		{
			if (!_checkListOrder(prop->getter(), errorType))
				return false;
		}
	}
	else if (parse->type() == ParseType::Expression)
	{
		if (!_checkListOrder(dynamic_cast<const ParseExpression*>(parse)->list(), errorType))
			return false;
	}
	return true;
}
bool CScriptParser::_checkListOrder(const std::vector<const BaseParse*>& list, ParseErrors errorType)
{
	auto isBracket = [](const BaseParse* parse)
		{
			if (parse->type() == ParseType::Symbol)
			{
				const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
				switch (sym->symbol())
				{
				case SymbolType::CloseBracket:
				case SymbolType::OpenBracket:
				case SymbolType::StartBlock:
				case SymbolType::EndBlock:
					return true;
				}
			}

			return false;
		};

	const BaseParse* previous = NULL;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (parse->type() == ParseType::Brackets || parse->type() == ParseType::Expression)
		{
			if (!_checkListOrder(parse, errorType))
				return false;
			previous = NULL;
			continue;
		}
		else if (parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* symbol = dynamic_cast<const ParseSymbol*>(parse);
			if (symbol->symbol() == SymbolType::Unknown)
			{
				ParseFail* fail = new ParseFail(parse, ParseErrors::InvalidSymbol);
				fail->addData(parse->stringData());
				_errors.push_back(fail);
				return false;
			}
		}
		else
		{
			if (!_checkListOrder(parse, errorType))
				return false;
		}

		if (previous)
		{
			bool isPreviousSymbol = previous->type() == ParseType::Symbol || previous->type() == ParseType::Operator;
			bool isCurrentSymbol = parse->type() == ParseType::Symbol || parse->type() == ParseType::Operator;

			// ignore brackets
			if (!isBracket(previous) && !isBracket(parse) && previous->type() != ParseType::Condition && ((!isPreviousSymbol && !isCurrentSymbol) || (isPreviousSymbol && isCurrentSymbol)))
			{
				// make exception for the assignment operator.  Non urary operators will be caught later
				bool exception = false;
				if (previous->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(previous)->symbol() == SymbolType::Assignment)
					exception = true;

				// Allow post-run inc/dec function between two Array nodes
				if (previous->type() == ParseType::Array &&
					parse->type() == ParseType::Function)
				{
					const ParseFunction* func = dynamic_cast<const ParseFunction*>(parse);
					if (func->isPostRun())
						exception = true;
				}

				// Allow setter Array to follow a post-run inc/dec function
				if (previous->type() == ParseType::Function &&
					parse->type() == ParseType::Array)
				{
					const ParseFunction* prevFunc = dynamic_cast<const ParseFunction*>(previous);
					if (prevFunc->isPostRun())
						exception = true;
				}

				// check if there is an "else"
				if (previous->type() == ParseType::Function && parse->type() == ParseType::Function)
				{
					auto prevFunc = dynamic_cast<const ParseFunction*>(previous);
					if (prevFunc->function() == L"else")
						exception = true;
					else if (prevFunc->condition())
						exception = true;
				}
				else if (previous->type() == ParseType::Array)
				{
					auto prevArr = dynamic_cast<const ParseArray*>(previous);
					if (prevArr->assignment() && prevArr->assignment()->type() == ParseType::Condition)
						exception = true;
				}
				// check for a function condition
				else if (previous->type() == ParseType::Function)
				{
					auto prevFunc = dynamic_cast<const ParseFunction*>(previous);
					if (prevFunc->condition())
						exception = true;
				}

				if (parse->type() == ParseType::Operator)
				{
					const ParseOperator* oper = dynamic_cast<const ParseOperator*>(parse);
					if (oper->isOperSingle())
						exception = true;
				}

				// if we have 2 properties next to each other, then allow them
				if (previous->type() == ParseType::Property && parse->type() == ParseType::Property)
				{
					if (dynamic_cast<const ParseProperty*>(previous)->getter() && dynamic_cast<const ParseProperty*>(parse)->setter())
						exception = true;
				}

				if (!exception)
				{
					ParseFail* fail = new ParseFail(parse, errorType);
					fail->addData(parse->data());
					_errors.push_back(fail);
					return false;
				}
			}
		}

		previous = parse;
	}

	return true;
}

bool CScriptParser::parseFunctions(const std::vector<const BaseParse*>& originalList, std::vector<const BaseParse*>& parseList)
{
	const BaseParse* previous = NULL;
	ParseFunction* function = NULL;

	bool error = false;
	for (auto itr = originalList.rbegin(); itr != originalList.rend(); itr++)
	{
		const BaseParse* parse = *itr;

		if (error)
		{
			parseList.push_back(parse);
			continue;
		}

		if (parse->type() == ParseType::Brackets)
		{
			const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);
			std::vector<const BaseParse*> newList;
			if (!parseFunctions(brackets->constList(), newList))
				return false;
			const_cast<ParseBrackets*>(brackets)->clear();
			for (auto bItr = newList.begin(); bItr != newList.end(); bItr++)
				const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
			parseList.insert(parseList.begin(), brackets);
		}

		if (parse->type() == ParseType::Operator)
		{
			parseList.insert(parseList.begin(), parse);
			function = NULL;
		}
		else if (parse->type() == ParseType::Keyword)
		{
			const ParseKeyword* keyword = dynamic_cast<const ParseKeyword*>(parse);
			if (previous && previous->type() == ParseType::Brackets)
			{
				ParseBrackets* brackets = const_cast<ParseBrackets*>(dynamic_cast<const ParseBrackets*>(previous));

				// ── Check for macro call ──────────────────────────────────────
				const MacroData* macro = _data->findMacro(keyword->keyword());
				if (macro)
				{
					// Extract argument strings from brackets (comma-separated items)
					std::vector<std::wstring> macroArgs;
					std::wstring currentArg;
					int depth = 0;
					for (const BaseParse* item : brackets->constList())
					{
						if (item->type() == ParseType::Symbol)
						{
							const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(item);
							if (sym->symbol() == SymbolType::Comma && depth == 0)
							{
								// Trim whitespace
								size_t s = currentArg.find_first_not_of(L" \t");
								size_t e = currentArg.find_last_not_of(L" \t");
								if (s != std::wstring::npos)
									macroArgs.push_back(currentArg.substr(s, e - s + 1));
								else
									macroArgs.push_back(L"");
								currentArg.clear();
								continue;
							}
							else if (sym->symbol() == SymbolType::OpenBracket) depth++;
							else if (sym->symbol() == SymbolType::CloseBracket) depth--;
						}
						currentArg += item->data();
					}
					// Last arg
					{
						size_t s = currentArg.find_first_not_of(L" \t");
						size_t e = currentArg.find_last_not_of(L" \t");
						if (s != std::wstring::npos)
							macroArgs.push_back(currentArg.substr(s, e - s + 1));
					}

					delete keyword;
					// Remove the brackets from parseList
					if (!parseList.empty()) parseList.erase(parseList.begin());
					delete brackets;

					if (macro->hasBlock)
					{
						// Push state — body lines will be captured in parseLine
						MacroCallState state;
						state.macro = macro;
						state.args = macroArgs;
						state.depth = 0;
						state.inBody = false;
						_macroStack.push_back(state);

						// Check if '{' or a single-statement body follows on the same line
						bool foundBlock = false;
						std::wstring singleBody;

						for (auto rem = parseList.begin(); rem != parseList.end(); ++rem)
						{
							if ((*rem)->type() == ParseType::Symbol &&
								dynamic_cast<const ParseSymbol*>(*rem)->symbol() == SymbolType::StartBlock)
							{
								// Consume the { — start body capture immediately
								_macroStack.back().inBody = true;
								_macroStack.back().depth = 1;
								delete* rem;
								++rem;
								// Any remaining items after { are the first statement
								if (rem != parseList.end())
								{
									std::wstring firstBodyLine;
									for (auto rest = rem; rest != parseList.end(); ++rest)
									{
										firstBodyLine += (*rest)->data();
										auto next = rest; ++next;
										if (next != parseList.end())
											firstBodyLine += L" ";
										delete* rest;
									}
									if (!firstBodyLine.empty())
										_macroStack.back().body.push_back(firstBodyLine + L";");
								}
								foundBlock = true;
								parseList.clear();
								break;
							}
							// Accumulate non-brace items as potential single-statement body
							if (!singleBody.empty()) singleBody += L" ";
							singleBody += (*rem)->data();
							delete* rem;
						}
						parseList.clear();

						// If no { was found but there are items, it's a single-statement body
						// — expand immediately without waiting for parseLine
						if (!foundBlock && !singleBody.empty())
						{
							std::vector<std::wstring> body = { singleBody + L";" };
							std::vector<std::wstring> args = _macroStack.back().args;
							const MacroData* m = _macroStack.back().macro;
							_macroStack.pop_back();
							if (!_expandMacro(m, args, body))
								error = true;
						}
						break;
					}
					else
					{
						// No block — expand immediately
						if (!_expandMacro(macro, macroArgs, {}))
							error = true;
					}

					previous = parse;
					continue;
				}

				// create the new function parse
				ParseFunction* func = new ParseFunction(keyword->line(), keyword->keyword());
				func->setPosition(keyword->startingPos(), brackets->endingPos());
				func->setLinePosition(keyword->linePos());
				func->setFile(_currentFile.back());
				func->setData(CombineStrings(keyword->data(), brackets->data()));

				// convert the brackets to an arguments list
				ParseArguments* args = new ParseArguments(brackets->line());
				args->setFromParse(brackets);
				func->setArguments(args);
				parseList.erase(parseList.begin());
				parseList.insert(parseList.begin(), func);
				function = func;

				delete keyword;

				// check for valid arguments
				auto fail = args->addArguments(brackets);

				brackets->clear();
				delete brackets;

				if (fail)
				{
					delete args;
					func->setArguments(NULL);
					_errors.push_back(fail);
					error = true;
					continue;
				}
				previous = function;
				continue;
			}
			// check for any special keywords
			else
			{
				unsigned int funcID = (keyword) ? _data->findSpecialKeyword(keyword->keyword()) : 0;
				if (funcID)
				{
					auto* func = _data->findGlobalFunction(keyword->keyword());
					if (func && func->id == funcID)
					{
						function = NULL;
						ParseFunction* newFunc = new ParseFunction(keyword->line(), keyword->keyword());
						if (!parseList.empty())
						{
							// add everything after as an expression
							ParseExpression* expr = new ParseExpression(keyword->line());
							expr->setFromParse(parseList.front());
							for (auto addItr = parseList.begin(); addItr != parseList.end(); addItr++)
							{
								expr->addParse(const_cast<BaseParse*>(*addItr));
								expr->setPosition(expr->startingPos(), (*addItr)->endingPos());
							}
							expr->setData(expr->line().substr(expr->startingPos(), expr->endingPos()));

							ParseArguments* args = new ParseArguments(keyword->line());
							args->setFromParse(expr);
							if (expr->size() == 1)
							{
								args->addParse(const_cast<BaseParse*>(expr->list().front()));
								expr->clearList();
								delete expr;
							}
							else
								args->addParse(expr);
							newFunc->setArguments(args);
							parseList.clear();
						}
						else
						{
							newFunc->setFromParse(keyword);
							ParseArguments* args = new ParseArguments(keyword->line());
							newFunc->setArguments(args);
						}
						parseList.insert(parseList.begin(), newFunc);
						delete keyword;
						previous = parse;
						continue;
					}
				}
			}
			parseList.insert(parseList.begin(), parse);
		}
		else if (parse->type() == ParseType::Namespace)
		{
			const ParseNamespace* ns = dynamic_cast<const ParseNamespace*>(parse);
			if (previous && previous->type() == ParseType::Brackets)
			{
				ParseBrackets* brackets = const_cast<ParseBrackets*>(dynamic_cast<const ParseBrackets*>(previous));

				// Look up the namespace function to get the real global name
				const Function* fn = _data->findNamespaceFunction(ns->namespaceString(), ns->keyword());
				std::wstring funcName = fn ? fn->name : ns->keyword();

				ParseFunction* func = new ParseFunction(ns->line(), funcName);
				func->setPosition(ns->startingPos(), brackets->endingPos());
				func->setLinePosition(ns->linePos());
				func->setFile(_currentFile.back());
				func->setData(CombineStrings(ns->data(), brackets->data()));

				ParseArguments* args = new ParseArguments(brackets->line());
				args->setFromParse(brackets);
				func->setArguments(args);
				parseList.erase(parseList.begin());
				parseList.insert(parseList.begin(), func);
				function = func;

				delete ns;

				auto fail = args->addArguments(brackets);
				brackets->clear();
				delete brackets;

				if (fail)
				{
					delete args;
					func->setArguments(NULL);
					_errors.push_back(fail);
					error = true;
				}

				previous = parse;
				continue;
			}
			parseList.insert(parseList.begin(), parse);
		}
		else if (parse->type() == ParseType::Condition)
		{
			const ParseCondition* cond = dynamic_cast<const ParseCondition*>(parse);
			if (cond->condition() == Conditions::Else)
			{
				ParseFunction* elseFunction = new ParseFunction(cond->line(), L"else");
				ParseArguments* arguments = new ParseArguments(L"");
				arguments->setLinePosition(cond->linePos());
				arguments->setFile(_currentFile.back());
				arguments->setPosition(cond->endingPos(), cond->endingPos());
				elseFunction->setArguments(arguments);
				elseFunction->setLinePosition(cond->linePos());
				elseFunction->setFile(_currentFile.back());
				elseFunction->setPosition(cond->startingPos(), cond->endingPos());
				elseFunction->setCondition(const_cast<ParseCondition*>(cond));
				parseList.insert(parseList.begin(), elseFunction);
			}
			else if (previous && previous->type() == ParseType::Brackets)
			{
				const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(previous);
				const BaseParse* baseItem = brackets->singleItem();
				if (baseItem)
				{
					if (baseItem->type() == ParseType::Function)
					{
						const ParseFunction* func = dynamic_cast<const ParseFunction*>(baseItem);
						const_cast<ParseFunction*>(func)->setCondition(const_cast<ParseCondition*>(cond));
						parseList.erase(parseList.begin());
						parseList.insert(parseList.begin(), func);
						const_cast<ParseBrackets*>(brackets)->clear();
						delete brackets;
					}
					else
						parseList.insert(parseList.begin(), parse);
				}
				else
					parseList.insert(parseList.begin(), parse);
			}
			else if (previous && previous->type() == ParseType::Function)
			{
				const ParseFunction* func = dynamic_cast<const ParseFunction*>(previous);
				const_cast<ParseFunction*>(func)->setCondition(const_cast<ParseCondition*>(cond));
			}
			else if (!parseList.empty() && parseList.front()->type() == ParseType::Function)
			{
				// Handles: START $object->func(...)
				// After the -> handler consumes the object variable, 'previous' points to the
				// variable, not the function. The function is at the front of parseList with
				// its _object already set — attach the condition there instead.
				const ParseFunction* func = dynamic_cast<const ParseFunction*>(parseList.front());
				if (func && func->object())
				{
					const_cast<ParseFunction*>(func)->setCondition(const_cast<ParseCondition*>(cond));
				}
				else
				{
					_addError(ParseErrors::InvalidCondition, cond);
					error = true;
				}
			}
			else
			{
				_addError(ParseErrors::InvalidCondition, cond);
				error = true;
			}
		}
		else if (parse->type() != ParseType::Brackets)
		{
			bool dontAdd = false;
			if (previous)
			{
				if (previous->type() == ParseType::Symbol)
				{
					const ParseSymbol* symbol = dynamic_cast<const ParseSymbol*>(previous);
					if (symbol->symbol() == SymbolType::Object)
					{
						if (function && !function->object())
						{
							function->setObject(const_cast<BaseParse*>(parse));
							parseList.erase(parseList.begin());
							delete previous;
							dontAdd = true;
						}
					}
					else if (symbol->symbol() == SymbolType::Assignment)
					{
						if (function && !function->returnVariable())
						{
							if (parse->type() == ParseType::Variable)
							{
								function->setReturnVariable(const_cast<ParseVariable*>(dynamic_cast<const ParseVariable*>(parse)));
								parseList.erase(parseList.begin());
								delete previous;
								dontAdd = true;
							}
							else if (parse->type() != ParseType::Property && !(parse->type() == ParseType::Symbol && dynamic_cast<const ParseSymbol*>(parse)->symbol() == SymbolType::CloseArray))
							{
								_addError(ParseErrors::InvalidReturnValue, parse);
								error = true;
							}
						}
					}
				}
			}

			if (parse && parse->type() == ParseType::Symbol)
			{
				// check if the function is inside an array
				const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
				if (sym->symbol() == SymbolType::OpenArray)
					function = NULL;
			}

			if (!dontAdd)
				parseList.insert(parseList.begin(), parse);
		}

		previous = parse;
	}

	return !error;
}


bool CScriptParser::_parseExpressions(const std::vector<const BaseParse*>& originalList, std::vector<const BaseParse*>& parseList)
{
	auto finalise = [](ParseExpression* expression)
		{
			if (expression->condition() && expression->list().front()->type() == ParseType::Brackets && expression->list().size() == 1)
			{
				const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(expression->list().front());
				expression->clearList();

				if (brackets->singleItem() && brackets->singleItem()->type() == ParseType::Expression)
				{
					const ParseExpression* exp = dynamic_cast<const ParseExpression*>(brackets->singleItem());
					for (auto itr = exp->list().begin(); itr != exp->list().end(); itr++)
						expression->addParse(const_cast<BaseParse*>(*itr));
					const_cast<ParseExpression*>(exp)->clearList();
					delete exp;
				}
				else
				{
					for (auto itr = brackets->list().begin(); itr != brackets->list().end(); itr++)
						expression->addParse(const_cast<BaseParse*>(*itr));
				}
				const_cast<ParseBrackets*>(brackets)->clear();
				delete brackets;
			}
		};

	auto compact = [](ParseExpression* expression)
		{
			if (expression->list().size() == 2)
			{
				if (expression->list()[0]->type() == ParseType::Operator && dynamic_cast<const ParseOperator*>(expression->list()[0])->operType() == Operators::Subtract && expression->list()[1]->type() == ParseType::Integer)
				{
					ParseInteger* integer = const_cast<ParseInteger*>(dynamic_cast<const ParseInteger*>(expression->list()[1]));
					integer->negate();
					expression->clearList();
					expression->addParse(integer);
				}
			}
		};

	auto internalExpression = [](ParseExpression* expr, std::vector<const BaseParse*>& newList)
		{
			expr->clearList();
			if (newList.size() == 1)
			{
				if (newList.front()->type() == ParseType::Expression)
				{
					ParseExpression* iExpr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(newList.front()));
					for (auto eItr = iExpr->list().begin(); eItr != iExpr->list().end(); eItr++)
						expr->addParse(const_cast<BaseParse*>(*eItr));
					iExpr->clearList();
					delete iExpr;
				}
				else
					expr->addParse(const_cast<BaseParse*>(newList.front()));
			}
			else
			{
				for (auto eItr = newList.begin(); eItr != newList.end(); eItr++)
					expr->addParse(const_cast<BaseParse*>(*eItr));
			}
		};

	bool error = false;

	ParseExpression* expression = NULL;
	const BaseParse* previous = NULL;
	for (auto itr = originalList.begin(); itr != originalList.end(); itr++)
	{
		if (error)
		{
			parseList.push_back(*itr);
			continue;
		}

		const BaseParse* parse = *itr;

		if (!expression)
		{
			expression = new ParseExpression(parse->line());
			parseList.push_back(expression);
		}

		if (parse->type() == ParseType::Condition)
		{
			const ParseCondition* cond = dynamic_cast<const ParseCondition*>(parse);
			if (expression->condition())
			{
				error = true;
				ParseFail* fail = new ParseFail(parse, ParseErrors::InvalidDoubleCondition);
				_errors.push_back(fail);
			}
			else if (expression->size() > 0)
			{
				// Condition seen after expression content — start a new expression
				finalise(expression);
				expression = new ParseExpression(parse->line());
				parseList.push_back(expression);
				expression->setCondition(const_cast<ParseCondition*>(cond));
				previous = cond;
				continue;
			}
			else
			{
				expression->setCondition(const_cast<ParseCondition*>(cond));
				previous = cond;
				continue;
			}
		}

		else if (parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
			if (sym->symbol() == SymbolType::Assignment)
			{
				if (expression->assignment())
				{
					// Second assignment in chain: $x = $y = value
					// The last item in the current expression list is the new assignment variable ($y).
					// Finalise the current expression ($x = $y), start a new one ($y = value).
					if (!previous || previous->type() != ParseType::Variable)
					{
						error = true;
						_addError(ParseErrors::InvalidVariable, previous ? previous : parse);
					}
					else
					{
						// Remove $y from the current expression list — it becomes the
						// assignment target of the new inner expression, but also
						// remains as the value in the outer expression ($x = $y).
						ParseVariable* chainVar = const_cast<ParseVariable*>(
							dynamic_cast<const ParseVariable*>(previous));

						// Create a copy of $y for chainExpr's assignment — the original
						// stays in expression->list() as the value for $x = $y.
						// Owned exclusively by chainExpr via setAssignment — do NOT add to _createdData.
						ParseVariable* chainVarCopy = new ParseVariable(*chainVar);

						// Insert the new inner expression ($y = value) BEFORE the current
						// outer expression ($x = $y) so it executes first.
						ParseExpression* chainExpr = new ParseExpression(parse->line());
						auto curPos = std::find(parseList.begin(), parseList.end(),
							static_cast<const BaseParse*>(expression));
						parseList.insert(curPos, chainExpr);

						delete sym;
						chainExpr->setAssignment(chainVarCopy);
						previous = parse;
						expression = chainExpr;
						continue;
					}
				}
				else
				{
					if (!previous)
					{
						error = true;
						ParseFail* fail = new ParseFail(parse, ParseErrors::MissingAssignment);
						_errors.push_back(fail);
					}
					else if (previous->type() != ParseType::Variable)
					{
						error = true;
						_addError(ParseErrors::InvalidVariable, previous);
					}
					else
					{
						delete sym;
						expression->removeLastParse();
						expression->setAssignment(const_cast<ParseVariable*>(dynamic_cast<const ParseVariable*>(previous)));
						previous = parse;
						continue;
					}
				}
			}
			else if (sym->symbol() == SymbolType::StartBlock)
			{
				if (previous && (previous->type() == ParseType::Function || previous->type() == ParseType::Array))
				{
					// If previous is a post-run function (e.g. inc from $i++), the condition
					// belongs to the last finalised expression in parseList, not the current one.
					const ParseFunction* prevFn = (previous->type() == ParseType::Function)
						? dynamic_cast<const ParseFunction*>(previous) : nullptr;
					if (prevFn && prevFn->isPostRun() && parseList.size() >= 2)
					{
						// After finalise(), a new empty expression is at parseList.back().
						// The if($i++) expression is at parseList[size-2].
						const ParseExpression* expr = nullptr;
						for (int pi = static_cast<int>(parseList.size()) - 1; pi >= 0; pi--)
						{
							if (parseList[pi]->type() == ParseType::Expression)
							{
								const ParseExpression* candidate = dynamic_cast<const ParseExpression*>(parseList[pi]);
								if (candidate->condition())
								{
									expr = candidate;
									break;
								}
							}
						}
						if (expr)
						{
							const_cast<ParseCondition*>(expr->condition())->setBlock(true);
							finalise(const_cast<ParseExpression*>(expr));
							delete parse;
							parse = NULL;
						}
					}
					else
					{
						if (expression->size() == 0 && !parseList.empty() && parseList.back() == expression)
						{
							if (!parseList.empty() && parseList.back()->type() == ParseType::Expression)
							{
								parseList.pop_back();
								delete expression;
								expression = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(parseList.back()));
							}
						}
						expression->addParse(const_cast<BaseParse*>(parse));
						finalise(expression);
						expression = NULL;
					}
				}
				else if (previous && previous->type() == ParseType::Expression)
				{
					const ParseExpression* expr = dynamic_cast<const ParseExpression*>(previous);
					if (expr->condition())
					{
						const_cast<ParseCondition*>(expr->condition())->setBlock(true);
						finalise(const_cast<ParseExpression*>(expr));
					}
					delete parse;
					parse = NULL;
				}
				else if (expression && expression->condition())
				{
					const_cast<ParseCondition*>(expression->condition())->setBlock(true);
					finalise(expression);
					expression = new ParseExpression(parse->line());
					parseList.push_back(expression);
					delete parse;
					parse = NULL;
				}

				previous = parse;
			}
			else if (sym->symbol() == SymbolType::EndBlock || sym->symbol() == SymbolType::End)
			{
				if (!expression->size())
				{
					parseList.pop_back();
					delete expression;
				}
				expression = new ParseExpression(parse->line());
				parseList.push_back(expression);
			}
		}
		else if (parse->type() == ParseType::Brackets)
		{
			const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);
			std::vector<const BaseParse*> newList;
			if (!_parseExpressions(brackets->constList(), newList))
				error = true;
			const_cast<ParseBrackets*>(brackets)->clear();
			for (auto bItr = newList.begin(); bItr != newList.end(); bItr++)
				const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
			newList.clear();

			if (expression->list().empty() && expression->condition())
			{
				expression->addParse(const_cast<BaseParse*>(parse));
				finalise(expression);
				previous = expression;
				expression = NULL;
				continue;
			}
		}
		else if (parse->type() == ParseType::Property)
		{
			ParseProperty* prop = const_cast<ParseProperty*>(dynamic_cast<const ParseProperty*>(parse));
			if (prop->setter())
			{
				if (prop->setter()->type() == ParseType::Expression)
				{
					ParseExpression* expr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(prop->setter()));
					std::vector<const BaseParse*> newList;
					if (!_parseExpressions(expr->list(), newList))
						error = true;
					expr->clearList();
					if (newList.size() == 1)
					{
						delete expr;
						prop->setSetter(const_cast<BaseParse*>(newList.at(0)));
					}
					else
					{
						for (auto bItr = newList.begin(); bItr != newList.end(); bItr++)
							expr->addParse(const_cast<BaseParse*>(*bItr));
					}
					newList.clear();
				}
			}
		}
		else if (parse->type() == ParseType::Array)
		{
			const ParseArray* arr = dynamic_cast<const ParseArray*>(parse);
			if (arr->assign() && arr->assign()->type() == ParseType::Brackets)
			{
				const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(arr->assign());
				std::vector<const BaseParse*> newList;
				if (!_parseExpressions(brackets->constList(), newList))
					error = true;

				const_cast<ParseBrackets*>(brackets)->clear();
				if (newList.size() == 1)
				{
					const_cast<ParseArray*>(arr)->setAssign(const_cast<BaseParse*>(newList.front()));
					delete brackets;
				}
				else
				{
					for (auto bItr = newList.begin(); bItr != newList.end(); bItr++)
						const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*bItr));
				}
				newList.clear();
			}
			else if (arr->assign() && arr->assign()->type() == ParseType::Expression)
			{
				ParseExpression* expr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(arr->assign()));
				std::vector<const BaseParse*> newList;
				if (!_parseExpressions(expr->list(), newList))
					error = true;

				internalExpression(expr, newList);
			}

			if (arr->value() && arr->value()->type() == ParseType::Expression)
			{
				ParseExpression* expr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(arr->value()));
				std::vector<const BaseParse*> newList;
				if (!_parseExpressions(expr->list(), newList))
					error = true;

				internalExpression(expr, newList);
			}
			if (arr->value2() && arr->value2()->type() == ParseType::Expression)
			{
				ParseExpression* expr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(arr->value2()));
				std::vector<const BaseParse*> newList;
				if (!_parseExpressions(expr->list(), newList))
					error = true;

				internalExpression(expr, newList);
			}

			if (arr->assignment() && arr->assignment()->type() == ParseType::Condition)
			{
				expression->addParse(const_cast<BaseParse*>(parse));
				finalise(expression);
				expression = NULL;
				previous = parse;
				continue;
			}
		}
		else if (parse->type() == ParseType::Function)
		{
			const ParseFunction* func = dynamic_cast<const ParseFunction*>(parse);
			if (func->arguments())
			{
				std::vector<const BaseParse*> argList(func->arguments()->constList());
				auto args = const_cast<ParseArguments*>(func->arguments());
				args->clear();
				for (auto aItr = argList.begin(); aItr != argList.end(); aItr++)
				{
					auto p = *aItr;
					if (p->type() == ParseType::Brackets)
					{
						const ParseBrackets* b = dynamic_cast<const ParseBrackets*>(p);
						if (b->size() == 1)
						{
							args->addParse(const_cast<BaseParse*>(b->list().front()));
							delete b;
						}
						else if (b->size() > 1)
						{
							std::vector<const BaseParse*> newList;
							if (!_parseExpressions(b->constList(), newList))
								error = true;
							const_cast<ParseBrackets*>(b)->clear();
							if (newList.size() == 1)
							{
								args->addParse(const_cast<BaseParse*>(newList.front()));
								delete b;
							}
							else
							{
								for (auto bItr = newList.begin(); bItr != newList.end(); bItr++)
									const_cast<ParseBrackets*>(b)->addParse(const_cast<BaseParse*>(*bItr));
								args->addParse(const_cast<ParseBrackets*>(b));
							}
							newList.clear();
						}
					}
					else if (p->type() == ParseType::Expression)
					{
						ParseExpression* expr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(p));
						std::vector<const BaseParse*> newList;
						error = !_parseExpressions(expr->list(), newList);

						expr->clearList();

						if (newList.size() == 1)
						{
							auto newExpr = newList.front();
							if (newExpr->type() == ParseType::Expression && dynamic_cast<const ParseExpression*>(newExpr)->size() == 1)
							{
								ParseExpression* iExpr = const_cast<ParseExpression*>(dynamic_cast<const ParseExpression*>(newExpr));
								args->addParse(const_cast<BaseParse*>(iExpr->list().front()));
								iExpr->clearList();
								delete iExpr;
							}
							else
							{
								const_cast<BaseParse*>(newExpr)->setFromParse(expr);
								args->addParse(const_cast<BaseParse*>(newExpr));
							}
							delete expr;
						}
						else
						{
							for (auto eItr = newList.begin(); eItr != newList.end(); eItr++)
								expr->addParse(const_cast<BaseParse*>(*eItr));
							args->addParse(expr);
						}
					}
					else
						args->addParse(const_cast<BaseParse*>(p));
				}
			}

			if (func->condition())
			{
				expression->addParse(const_cast<BaseParse*>(parse));
				finalise(expression);
				expression = NULL;
				previous = parse;
				continue;
			}
		}
		if (parse && expression) {
			expression->addParse(const_cast<BaseParse*>(parse));
			previous = parse;
		}
	}

	if (expression)
		compact(expression);
	if (expression)
		finalise(expression);

	// check for multiple line expression for if
	if (parseList.size() > 1)
	{
		std::vector<const BaseParse*> list(parseList);
		parseList.clear();

		const BaseParse* previous = NULL;
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			parseList.push_back(*itr);

			const BaseParse* parse = *itr;
			if (parse->type() == ParseType::Expression && previous && previous->type() == ParseType::Expression)
			{
				const ParseExpression* prevExpr = dynamic_cast<const ParseExpression*>(previous);
				const ParseExpression* expr = dynamic_cast<const ParseExpression*>(parse);
				if (prevExpr->condition() && !prevExpr->condition()->isBlock() && expr->lineCount() > 1)
				{
					const_cast<ParseCondition*>(prevExpr->condition())->setBlock(true);
					const_cast<ParseCondition*>(prevExpr->condition())->setBlockCount(expr->lineCount());
					ParseSymbol* endBrace = new ParseSymbol(L"}", L"}");
					ParseExpression* newExpr = new ParseExpression(L"}");
					newExpr->addParse(endBrace);
					parseList.push_back(newExpr);
				}
				else if (prevExpr->condition() && !prevExpr->condition()->isBlock())
				{
					// If the single-line body contains continue or break, force a block.
					// This ensures the re-evaluation code can be inserted inside the
					// block before the continue, instead of becoming do if which has
					// no block to insert into.
					bool hasContinueOrBreak = false;
					for (auto item : expr->list())
					{
						if (item->type() == ParseType::Function)
						{
							const ParseFunction* fn = dynamic_cast<const ParseFunction*>(item);
							if (fn->function() == L"continue" || fn->function() == L"break")
							{
								hasContinueOrBreak = true;
								break;
							}
						}
					}
					if (hasContinueOrBreak)
					{
						const_cast<ParseCondition*>(prevExpr->condition())->setBlock(true);
						const_cast<ParseCondition*>(prevExpr->condition())->setBlockCount(1);
						ParseSymbol* endBrace = new ParseSymbol(L"}", L"}");
						ParseExpression* newExpr = new ParseExpression(L"}");
						newExpr->addParse(endBrace);
						parseList.push_back(newExpr);
					}
				}
			}

			previous = parse;
		}
	}


	// combine multiple arrays together to simplify the function
	if (!error)
	{
		for (auto itr = parseList.begin(); itr != parseList.end(); itr++)
			const_cast<BaseParse*>(*itr)->simplify();
	}

	return !error;
}

/**
 * checkStatus
 * Checks the previous and current parse status.  IF we match, we simply combine them, if they different, we then created a new Parse Data type
 *
 * \arg oldStatus		-	The status of the previous character
 * \arg	newStatus		-	The status of the current character
 * \arg str				-	The current string for our status
 * \arg c				-	The current character we are checking
 * \arg line			-	The full line, used when creating a new parse data type
 *
 * Returns the new parse type if its created, otherwise just NULL
 */
BaseParse* CScriptParser::checkStatus(ParseStatus oldStatus, ParseStatus newStatus, std::wstring& str, wchar_t c, const std::wstring& line)
{
	// if its the start, then we ignore anything from befor (although should be nothing)
	if (oldStatus == ParseStatus::Start)
		str = c;
	// the same status, so we just add it to the current string
	else if (oldStatus == newStatus)
		str += c;
	// check of a status, we can create the parse data and reset the current string
	else
	{
		BaseParse* parse = NULL;
		switch (oldStatus)
		{
		case ParseStatus::KeyWord:
			parse = new ParseKeyword(line, str);
			break;
		case ParseStatus::End: // end should be a single symbol, ie ;
		case ParseStatus::Symbol:
			if (str != L"(" && str != L")" && ParseOperator::ConvertOperator(str) != Operators::Unknown)
				parse = new ParseOperator(line, str);
			else
				parse = new ParseSymbol(line, str);
			break;
		case ParseStatus::Integer:
			parse = new ParseInteger(line, std::stoi(str));
			break;
		case ParseStatus::String:
			parse = new ParseString(line, str);
			break;
		}
		// set the string we used, so we can use it like (ie for Errors)
		if (parse)
		{
			parse->setData(str);
			if (newStatus == ParseStatus::Whitespace)
				parse->setWhitespaceAfter(true);
		}

		// clear the old string and start it again with the new status
		str.clear();
		str = c;

		return parse;
	}

	return NULL;
}

std::wstring Trim(const std::wstring& s)
{
	size_t start = s.find_first_not_of(L" \t\r\n");
	if (start == std::wstring::npos)
		return L"";

	size_t end = s.find_last_not_of(L" \t\r\n");
	return s.substr(start, end - start + 1);
}

std::wstring CScriptParser::_parseDefine(const std::wstring& line)
{
	if (_defines.empty())
		return line;

	// check if its another define
	bool checkPre = false;
	bool checkDefine = false;
	bool inBracket = false;
	bool doRest = false;
	std::wstring preprocessor;
	std::wstring newLine;
	std::wstring define;
	std::wstring rest;
	preprocessor.reserve(line.length());
	newLine.reserve(line.length());
	define.reserve(line.length());
	rest.reserve(line.length());
	for (size_t pos = 0; pos < line.length(); ++pos)
	{
		wchar_t c = line[pos];
		if (!doRest)
			newLine += c;

		if (doRest)
			rest += c;
		else if (inBracket)
		{
			if (c == ')')
			{
				c = line[++pos];
				while (pos < line.length() && (c == ' ' || c == '\t' || c == '\n' || c == '\r'))
				{
					newLine += c;
					c = line[++pos];
				}
				rest += c;
				doRest = true;
			}
		}
		else if (checkDefine)
		{
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.')
				;
			else if (c == '(')
				inBracket = true;
			else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			{
				c = line[++pos];
				while (pos < line.length() && (c == ' ' || c == '\t' || c == '\n' || c == '\r'))
				{
					newLine += c;
					c = line[++pos];
				}
				rest += c;
				doRest = true;
			}
		}
		else if (checkPre)
		{
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.')
				preprocessor += c;
			else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			{
				if (preprocessor == L"undef" || preprocessor == L"define" || preprocessor == L"ifdef")
					checkDefine = true;
				else
					break;
			}
		}
		else
		{
			// first check for preprocessor symbol
			if (c == '#')
				checkPre = true;
			// ignore the whitespace
			else if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
				break;
			// break if its anything but whitespace or preprocessor '#'
		}
	}

	if (rest.empty())
	{
		rest = line;
		newLine.clear();
	}

	bool error = false;

	// first, find any define matching define
	std::wstring sLine = newLine;
	for (auto itr = _defines.begin(); itr != _defines.end(); itr++)
	{
		// Skip defines with no replacement value — they exist only for #ifdef checks
		if (itr->second->list().empty())
			continue;

		std::wstring replaceLine = itr->second->line().substr(itr->second->list().front()->startingPos(), itr->second->list().back()->endingPos() - itr->second->list().front()->startingPos());
		std::wstring processLine = rest;
		rest.clear();
		size_t startPos = 0;
		size_t pos = processLine.find(itr->first, startPos);
		while (pos != std::wstring::npos)
		{
			rest += processLine.substr(startPos, pos - startPos);

			// find any variables in brackets
			startPos = pos + itr->first.length();
			size_t checkPos = startPos;
			// move past whitespace
			while (processLine[checkPos] == ' ' || processLine[checkPos] == '\n' || processLine[checkPos] == '\r' || processLine[checkPos] == '\t')
				checkPos++;
			// check for open brackets
			if (processLine[checkPos] == '(')
			{
				// split anything into variables, seperated by comma ','
				std::vector<std::wstring> variables;
				startPos = ++checkPos;
				bool inString = false;
				while (processLine[checkPos] != ')' && checkPos < processLine.length())
				{
					if (processLine[checkPos] == '"')
					{
						if (processLine[checkPos - 1] != '\\')
							inString = !inString;
					}
					else if (!inString && processLine[checkPos] == ',')
					{
						variables.push_back(Trim(processLine.substr(startPos, checkPos - startPos)));
						startPos = checkPos + 1;
					}
					checkPos++;
				}

				//missing end bracket
				if (processLine[checkPos] != ')')
				{
					error = true;
					break;
				}

				variables.push_back(Trim(processLine.substr(startPos, checkPos - startPos)));
				startPos = checkPos + 1;

				if (!variables.empty())
				{
					auto& list = itr->second->variables();

					std::wstring oldReplace = replaceLine;
					replaceLine.clear();
					auto vItr = list.begin();
					for (auto vItr2 = variables.begin(); vItr != list.end() && vItr2 != variables.end(); vItr++, vItr2++)
					{
						size_t startPos = 0;
						size_t pos = oldReplace.find(*vItr, startPos);
						size_t len = oldReplace.length();
						while (pos != std::wstring::npos)
						{
							replaceLine += oldReplace.substr(startPos, pos - startPos);
							replaceLine += *vItr2;

							startPos = pos + vItr->length();
							pos = oldReplace.find(*vItr, startPos);
						}

						// Carry any unmatched tail forward so the next parameter
						// substitution can still find and replace its placeholder.
						replaceLine += oldReplace.substr(startPos);
						oldReplace = replaceLine;
						replaceLine.clear();
					}
					// After all substitutions, oldReplace holds the final result
					replaceLine = oldReplace;
				}
			}

			if (!itr->second->list().empty())
				rest += replaceLine;

			pos = processLine.find(itr->first, startPos);
		}
		if (error)
			break;

		rest += processLine.substr(startPos);
	}

	if (error)
		return line;

	return newLine + rest;
}

/**
 * parseLine
 * \arg	linePos		-	The current line position in the file, used for displaying errors/warnings
 * \arg sLine		-	The actual string for the line, should include everything from a line in the file
 *
 * This will take the line string and split into various data types, Symbols, Words and String
 * This data is then added to a list which can included data created from a previous line
 *
 * The data is then processed once it gets to an end ';' or '}'
 *
 * Any errors will add a ParseFail type to the errors list, and will return false to indicate an error was found
 * These errors contains the complete line, and the position within the line the error was found
 */
bool CScriptParser::parseLine(size_t linePos, const std::wstring& line)
{
	// adds data to the new parse type
	// this computes the start and end positions of the current string
	auto addParse = [](BaseParse* parse, std::vector<const BaseParse*>& parseList, size_t pos, size_t len, size_t linePos, size_t movePosition, const std::wstring& file)
		{
			if (parse)
			{
				parse->setLinePosition(linePos);
				parse->setFile(file);
				parse->setPosition(pos - len + movePosition, pos + movePosition);
				if (!parseList.empty())
					parse->setWhitespaceBefore(parseList.back()->hasWhitespaceAfter());
				parseList.push_back(parse);
			}
		};

	std::wstring str;							// the current string for the status
	ParseStatus status = ParseStatus::Start;	// the current status
	std::vector<const BaseParse*> parseList;	// The created parse datas
	size_t stringStart = 0;						// the position the string starts at, this is only used for the error if theres no end quote found

	if (_isInComment)
		status = ParseStatus::Comment;

	// ── Macro body capture ────────────────────────────────────────────────────
	if (!_macroStack.empty())
	{
		MacroCallState& state = _macroStack.back();

		if (!state.inBody)
		{
			// Waiting for the opening brace
			std::wstring trimmed = line;
			size_t first = trimmed.find_first_not_of(L" \t");
			if (first != std::wstring::npos && trimmed[first] == L'{')
			{
				state.inBody = true;
				state.depth = 1;
				// If there's content after the { on the same line, buffer it
				std::wstring rest = trimmed.substr(first + 1);
				size_t rFirst = rest.find_first_not_of(L" \t");
				if (rFirst != std::wstring::npos)
				{
					// Check if this single line also closes the block
					for (wchar_t c : rest)
					{
						if (c == L'{') state.depth++;
						else if (c == L'}') state.depth--;
					}
					if (state.depth <= 0)
					{
						// Single-line body
						std::wstring bodyLine = rest.substr(0, rest.find_last_not_of(L" \t}") + 1);
						if (!bodyLine.empty())
							state.body.push_back(bodyLine);
						const MacroData* macro = state.macro;
						std::vector<std::wstring> args = state.args;
						std::vector<std::wstring> body = state.body;
						_macroStack.pop_back();
						return _expandMacro(macro, args, body);
					}
					state.body.push_back(rest);
				}
				return true;
			}
			else if (first != std::wstring::npos)
			{
				// No brace — treat the whole line as a single-statement body
				const MacroData* macro = state.macro;
				std::vector<std::wstring> args = state.args;
				std::vector<std::wstring> body = { trimmed };
				_macroStack.pop_back();
				return _expandMacro(macro, args, body);
			}
			// Empty line — keep waiting
		}
		else
		{
			// Counting braces to track nesting depth
			for (wchar_t c : line)
			{
				if (c == L'{') state.depth++;
				else if (c == L'}') state.depth--;
			}

			if (state.depth <= 0)
			{
				// Closing brace reached — expand
				const MacroData* macro = state.macro;
				std::vector<std::wstring> args = state.args;
				std::vector<std::wstring> body = state.body;
				_macroStack.pop_back();
				return _expandMacro(macro, args, body);
			}
			else
			{
				state.body.push_back(line);
				return true;
			}
		}
	}
	// with '#' so we can correctly track nested #ifdef / #endif depth.
	bool inSkippedBlock = !_ifDefStack.empty() && !_ifDefStack.back().active;
	if (inSkippedBlock)
	{
		// Check if this line is a preprocessor directive
		std::wstring trimmed = line;
		size_t firstNonSpace = trimmed.find_first_not_of(L" \t");
		bool isPreprocessor = (firstNonSpace != std::wstring::npos && trimmed[firstNonSpace] == L'#');
		if (!isPreprocessor)
			return true; // skip the line entirely
		// Fall through to process the preprocessor directive (for nesting)
	}

	// check for any defines — skip expansion for preprocessor lines (#ifdef, #define etc.)
	// so that the symbol name being tested/defined doesn't get incorrectly substituted.
	bool isPreprocessorLine = false;
	{
		size_t first = line.find_first_not_of(L" \t");
		isPreprocessorLine = (first != std::wstring::npos && line[first] == L'#');
	}

	// Handle multi-line #define continuation using trailing backslash.
	// When a #define line ends with \, accumulate it and wait for the next line.
	if (_inLineContinuation)
	{
		// Append this line to the accumulated continuation text
		std::wstring trimmed = line;
		// Check if this line also ends with backslash
		size_t last = trimmed.find_last_not_of(L" \t");
		if (last != std::wstring::npos && trimmed[last] == L'\\')
		{
			// Another continuation line — strip the backslash and accumulate
			_continuationText += trimmed.substr(0, last);
			return true;
		}
		else
		{
			// Final line — complete the accumulated define and process it
			_continuationText += trimmed;
			std::wstring fullLine = _continuationText;
			_continuationText.clear();
			_inLineContinuation = false;
			return parseLine(linePos, fullLine);
		}
	}

	if (isPreprocessorLine)
	{
		// Check if this #define line ends with backslash — start continuation
		std::wstring trimmed = line;
		size_t last = trimmed.find_last_not_of(L" \t");
		size_t first = trimmed.find_first_not_of(L" \t");
		// Only handle continuation for #define lines
		if (last != std::wstring::npos && trimmed[last] == L'\\')
		{
			std::wstring keyword;
			size_t keyStart = first + 1; // skip '#'
			size_t keyEnd = trimmed.find_first_of(L" \t", keyStart);
			if (keyEnd != std::wstring::npos)
				keyword = trimmed.substr(keyStart, keyEnd - keyStart);
			else
				keyword = trimmed.substr(keyStart);

			if (keyword == L"define")
			{
				_continuationText = trimmed.substr(0, last); // strip backslash
				_inLineContinuation = true;
				return true;
			}
		}
	}

	std::wstring sLine = isPreprocessorLine ? line : _parseDefine(line);

	// pre-reserve the max space needed so we dont to keep allocing more memory
	str.reserve(sLine.length());

	bool error = false;

	size_t movePosition = 0;
	// split the string into parse types, seperates out Symbols and Words
	for (size_t pos = 0; pos < sLine.length(); ++pos)
	{
		wchar_t c = sLine[pos];
		size_t len = str.length();

		// We are currently in a string, so we only care about the end of a string '"'.  Everything else just gets added the string
		// we also check for the escape character '\"' in the string to allow including quotes.  Any others will be added as is and upto the game to use them correctly
		if (status == ParseStatus::String)
		{
			if (c == '"')
			{
				//check for escape character in the string
				if (str.length() > 0 && str.back() == '\\')
					str += c;
				else
				{
					auto parse = checkStatus(status, ParseStatus::Start, str, c, sLine);
					addParse(parse, parseList, pos + 1, len + 2, linePos, movePosition, _currentFile.back());
					status = ParseStatus::Start;
				}
			}
			else
				str += c;
		}
		// check if we are currently in a comment '/*'
		else if (status == ParseStatus::Comment)
		{
			if (str.empty())
			{
				if (c == '*')
					str += c;
			}
			else
			{
				str += c;
				if (str == L"*/")
				{
					status = ParseStatus::Start;
					_isInComment = false;
				}
				str.clear();
			}
		}
		//if we are not currently in a keyword, then check for an integer
		//NOTE: numbers are allowed in keywords so if we are currently reading a keyword, the number becomes part of that instead of an integer
		else if (status != ParseStatus::KeyWord && status != ParseStatus::WhitespaceKeyWord && c >= '0' && c <= '9')
		{
			auto parse = checkStatus(status, ParseStatus::Integer, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());
			status = ParseStatus::Integer;
		}
		// start of the string (end of the string is caught above)
		else if (c == '"')
		{
			stringStart = pos;
			auto parse = checkStatus(status, ParseStatus::String, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());
			status = ParseStatus::String;
			str.clear();
		}
		// check for a variable, the $ character must always be at the start of a keyword to indicate a variable
		// using it anywhere is invalid (except in a string or comment)
		else if (c == '$')
		{
			if (status == ParseStatus::KeyWord)
			{
				ParseFail* fail = new ParseFail(sLine, ParseErrors::InvalidKeywordDollar);
				fail->setLinePosition(linePos);
				fail->setFile(_currentFile.back());
				fail->setPosition(pos - str.length(), pos);
				fail->addData(str + c);
				_errors.push_back(fail);
				error = true;
				break;
			}
			auto parse = checkStatus(status, ParseStatus::KeyWord, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());
			status = ParseStatus::KeyWord;
		}
		// check for keyword
		//NOTE: we allow '.' in a keyword instead of symbol, as its commonly used in variables and constants in X3 Scripts
		else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.')
		{
			if (status == ParseStatus::Integer)
			{
				ParseFail* fail = new ParseFail(sLine, ParseErrors::InvalidKeywordIntStart);
				fail->setLinePosition(linePos);
				fail->setFile(_currentFile.back());
				fail->setPosition(pos - str.length(), pos);
				fail->addData(str + c);
				_errors.push_back(fail);
				error = true;
				break;
			}
			auto parse = checkStatus(status, ParseStatus::KeyWord, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());
			status = ParseStatus::KeyWord;
		}
		// check for any whitespace characters which we will ignore
		//Whitespace will seperate keywords and symbols, so a whitespace between 2 symbols will be seen as 2 seperate symbols
		else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
		{
			auto parse = checkStatus(status, ParseStatus::Whitespace, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());
			status = ParseStatus::Whitespace;
		}
		// Check of any single symbols.  These are special cases where we cant them to be seperated
		else if (c == ';' || c == ')' || c == '(' || c == '}' || c == '{' || c == ']' || c == '[' || c == ',')
		{
			auto parse = checkStatus(status, ParseStatus::End, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());

			ParseSymbol* symbol = new ParseSymbol(sLine, str);
			addParse(symbol, parseList, pos + 1, str.length(), linePos, movePosition, _currentFile.back());
			str.clear();

			status = ParseStatus::Start;
		}
		// everything else must be a symbol
		else
		{
			auto parse = checkStatus(status, ParseStatus::Symbol, str, c, sLine);
			addParse(parse, parseList, pos, len, linePos, movePosition, _currentFile.back());
			status = ParseStatus::Symbol;

			// check for the start of comments
			if (str.length() == 2)
			{
				if (str == L"++" || str == L"--" ||
					str == L"+=" || str == L"-=" ||
					str == L"*=" || str == L"/=")
				{
					// Leave str as-is — it will be emitted as one token
					// when the next status change occurs
				}
			}
			if (str == L"//")
			{
				// everything else in the line should be ignored as its part of the comment
				str.clear();
				break;
			}
			// switch the status to comment as the end of a comment can come in the same line
			else if (str == L"/*")
			{
				str.clear();
				status = ParseStatus::Comment;
			}
		}
	}

	// remember the comment state for the next line
	_isInComment = (status == ParseStatus::Comment);

	// if the status is still a string then we are missing a end quote
	//TODO: check for string line seperate '\' to allow string on multiple lines
	if (!error && status == ParseStatus::String)
	{
		ParseFail* fail = new ParseFail(sLine, ParseErrors::MissingQuote);
		fail->setPosition(stringStart, sLine.length());
		fail->setLinePosition(linePos);
		fail->setFile(_currentFile.back());
		_errors.push_back(fail);
		error = true;
	}

	// we still have some data we need to classify so we add that now
	if (!error && !str.empty())
	{
		size_t len = str.length();
		auto parse = checkStatus(status, ParseStatus::Start, str, ' ', sLine);
		addParse(parse, parseList, sLine.length(), len, linePos, movePosition, _currentFile.back());
	}

	// any errors, we can cancel the rest of the processing and display the error messages
	//Additional errors/warnings found beyond this point will not be displayed until the previous ones are fixed
	if (error)
	{
		for (auto itr = parseList.begin(); itr != parseList.end(); itr++)
			delete* itr;
		return false;
	}

	// first check if we have a label define (we handled these differently)
	if (parseList.size() == 2)
	{
		if (parseList.front()->type() == ParseType::Keyword && parseList.back()->type() == ParseType::Symbol)
		{
			const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parseList.back());
			if (sym->symbol() == SymbolType::DefineLabel)
			{
				error = !_addLabel(dynamic_cast<const ParseKeyword*>(parseList.front()));
				delete sym;
				if (error)
				{
					_addError(ParseErrors::InvalidLabel, parseList.front());
					delete parseList.front();
				}
				parseList.clear();
			}
		}
	}

	if (error)
		return false;

	// if we have a preprocessor commands '#' then always add an end symbol after
	if (!parseList.empty())
	{
		if (parseList.front()->type() == ParseType::Symbol)
		{
			auto parse = dynamic_cast<const ParseSymbol*>(parseList.front());
			if (parse->symbol() == SymbolType::Preprocessor)
			{
				if (!_parsePreprocessor(parseList))
				{
					for (auto itr = parseList.begin(); itr != parseList.end(); itr++)
						delete* itr;
					parseList.clear();
					return false;
				}

				return true;
			}
		}
	}

	// prepare the processing list
	// We process everything upto an end line ';' or an end block '}'
	// This can include multiple lines
	for (auto itr = parseList.begin(); itr != parseList.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (!error && parse->type() == ParseType::Symbol)
		{
			const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(parse);
			// change the symbol type, the define label should have already been used
			if (sym->symbol() == SymbolType::DefineLabel)
				const_cast<ParseSymbol*>(sym)->switchSymbol();

			if (sym->symbol() == SymbolType::End || sym->symbol() == SymbolType::EndBlock)
			{
				// if the symbol is an end line ';' then we delete it, as we dont need it anymore
				if (sym->symbol() == SymbolType::End)
					delete sym;
				else
					_currentDataList.push_back(parse);

				bool success = _parseDataList(_currentDataList);
				if (success)
				{
					// if the processing was successful, we can now actually run it to convert it into actual script commands
					success = _runDataList(_currentDataList, true);

					if (success && !_deferredLists.empty())
					{
						for (auto& deferredList : _deferredLists)
						{
							std::vector<const BaseParse*> dl(deferredList);
							if (!_runDataList(dl, true))
							{
								success = false;
								break;
							}
							// Add to created data for cleanup
							for (auto* p : dl)
								_createdData.push_back(p);
						}
						_deferredLists.clear();
					}

					_currentScript->flushPostRun();

					// reset the generated variables so we dont have too many created
					if (_generatedVariables >= 10)
						_generatedVariables = 0;

					// do any remaining warnings now we have a fully processed list
					//NOTE: some processing is done while running the command, so we cant check everything until after the commands have been added
					_checkWarnings(_currentDataList);
				}

				// add the processed data so we can remove it later
				//NOTE: we cant remove it now as some of it is still needed when saving the actual script (IE some expressions)
				for (auto itr = _currentDataList.begin(); itr != _currentDataList.end(); itr++)
					_createdData.push_back(*itr);

				// clear the list ready for the next line
				_currentDataList.clear();

				if (!success)
					error = true;
				continue;
			}
		}

		// add each data to the list so it can be processed later
		_currentDataList.push_back(parse);
	}

	return !error;
}

bool CScriptParser::prePassLine(size_t linePos, const std::wstring& line)
{
	// Thin wrapper — just run parseLine in pre-pass mode.
	// The two-pass logic (reading the file twice) is handled in compileScriptFile.
	_prePassMode = true;
	bool result = parseLine(linePos, line);
	_prePassMode = false;
	return result;
}

void CScriptParser::resetForRealPass()
{
	// Called between pass 1 and pass 2.
	// _subVariables is preserved — that's the output of pass 1.
	// Everything else is reset so the real compile starts clean.
	_currentSubLabel.clear();
	_subEndedOnLine = false;
	_inLineContinuation = false;
	_continuationText.clear();
	_macroStack.clear();
	_prePassDepth = 0;
	_prePassMode = false;
	_isInComment = false;
	_generatedVariables = 0;
	_whileGeneratedVariables = 0;
	_variables.clear();
	_pVariables = &_variables;

	for (auto itr = _errors.begin(); itr != _errors.end(); itr++)
		delete* itr;
	_errors.clear();
	_warnings.clear();
	for (auto itr = _currentDataList.begin(); itr != _currentDataList.end(); itr++)
		delete* itr;
	for (auto itr = _createdData.begin(); itr != _createdData.end(); itr++)
		delete* itr;
	_currentDataList.clear();
	_createdData.clear();
	_deferredLists.clear();
	for (void* p : _syntheticConstants)
		delete static_cast<ConstantData*>(p);
	_syntheticConstants.clear();
	_ifDefStack.clear();
	_conditionStack.clear();
	_createdExpressions.clear();
}

bool CScriptParser::_checkExpressionValidity(const BaseParse* parse)
{
	// allow null parse
	if (!parse)
		return true;

	if (parse->type() == ParseType::Expression)
	{
		const ParseExpression* expr = dynamic_cast<const ParseExpression*>(parse);
		if (!_checkExpressionValidity(expr->list(), false))
			return false;
	}
	else if (parse->type() == ParseType::Brackets)
	{
		const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);
		if (!_checkExpressionValidity(brackets->constList(), false))
			return false;
	}
	else if (parse->type() == ParseType::Array)
	{
		const ParseArray* arr = dynamic_cast<const ParseArray*>(parse);
		if (!_checkExpressionValidity(arr->assign()))
			return false;
		if (!_checkExpressionValidity(arr->assignment()))
			return false;
		if (!_checkExpressionValidity(arr->value()))
			return false;
		if (!_checkExpressionValidity(arr->value2()))
			return false;
		if (!_checkExpressionValidity(arr->variable()))
			return false;
	}
	else if (parse->type() == ParseType::Function)
	{
		const ParseFunction* func = dynamic_cast<const ParseFunction*>(parse);
		if (!_checkExpressionValidity(func->arguments()))
			return false;
		if (!_checkExpressionValidity(func->condition()))
			return false;
		if (!_checkExpressionValidity(func->returnVariable()))
			return false;
	}
	else if (parse->type() == ParseType::Arguments)
	{
		const ParseArguments* args = dynamic_cast<const ParseArguments*>(parse);
		if (!_checkExpressionValidity(args->constList(), false))
			return false;
	}

	return true;
}

bool CScriptParser::_checkExpressionValidity(const std::vector<const BaseParse*>& list, bool topLevel)
{
	const BaseParse* previous = NULL;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (!_checkExpressionValidity(parse))
			return false;

		if (previous && previous->type() == ParseType::Brackets && parse->type() == ParseType::Brackets)
		{
			_addError(ParseErrors::InvalidExpressionBrackets, parse);
			return false;
		}
		if (!previous && parse->type() == ParseType::Operator)
		{
			const ParseOperator* oper = dynamic_cast<const ParseOperator*>(parse);
			if (!oper->isOperSingle())
			{
				_addError(ParseErrors::InvalidExpressionUnaryOperator, parse);
				return false;
			}
		}
		if (parse->type() == ParseType::Expression)
		{
			const ParseExpression* expr = dynamic_cast<const ParseExpression*>(parse);
			if (expr->size() == 0)
			{
				_addError(ParseErrors::InvalidExpressionEmpty, expr);
				return false;
			}
		}

		previous = *itr;
	}

	return true;
}

bool CScriptParser::finalise()
{
	bool error = false;

	if (!_currentDataList.empty())
	{
		_addError(ParseErrors::MissingSemiColonEnd, _currentDataList.back());
		return false;
	}

	// Check for unclosed #ifdef / #ifndef blocks
	if (!_ifDefStack.empty())
	{
		ParseKeyword stub(L"end of file", L"#endif");
		_addError(ParseErrors::MissingIf, &stub);
		return false;
	}

	// final error checks, now we have the complete script
	auto& list = _currentScript->functions();
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		// check for labels
		if (itr->id == _data->gotoCommand() || itr->id == _data->gosubCommand())
		{
			if (!itr->argumentCount())
			{

			}
			else if (itr->firstArg()->type() != ParseType::Label)
			{

			}
			else
			{
				const ParseLabel* label = dynamic_cast<const ParseLabel*>(itr->firstArg());
				if (!_currentScript->isLabelValid(label->label()))
				{
					_addError(ParseErrors::MissingLabel, label);
					error = true;
				}

			}
		}
	}

	if (error)
		return false;

	return _currentScript->finalise();
}

bool CScriptParser::_runProperty(ParseProperty* prop, InlineState inlineState, bool doAssignment)
{
	bool error = false;

	if (!error && prop->setter())
	{
		auto funcData = _data->findObjectPropertySetter(prop->property());
		if (funcData)
		{
			ParseFunction* func = new ParseFunction(prop->line(), funcData->name);
			func->setLinePosition(prop->linePos());
			func->setFile(_currentFile.back());
			func->setPosition(prop->startingPos(), prop->endingPos());

			ParseArguments* args = new ParseArguments(prop->line());
			args->setLinePosition(prop->linePos());
			args->setFile(_currentFile.back());
			args->setPosition(prop->startingPos(), prop->endingPos());
			func->setArguments(args);
			func->setObject(prop->object());
			args->addParse(prop->setter());

			if (!_doGlobalFunction(funcData, func, inlineState))
				error = true;
			prop->setSetterFunction(func);
		}
		else
		{
			_addError(ParseErrors::UnknownProperty, prop);
			error = true;
		}
	}

	if (!error && prop->getter())
	{
		auto funcData = _data->findObjectPropertyGetter(prop->property());
		if (funcData)
		{
			ParseFunction* func = new ParseFunction(prop->line(), funcData->name);
			func->setLinePosition(prop->linePos());
			func->setFile(_currentFile.back());
			func->setPosition(prop->startingPos(), prop->endingPos());

			ParseArguments* args = new ParseArguments(prop->line());
			args->setLinePosition(prop->linePos());
			args->setFile(_currentFile.back());
			args->setPosition(prop->startingPos(), prop->endingPos());
			func->setArguments(args);
			func->setObject(prop->object());

			if (prop->getter()->type() == ParseType::Variable)
			{
				ParseVariable* retVar = dynamic_cast<ParseVariable*>(prop->getter());
				func->setReturnVariable(retVar);

				// Inject the return datatype from the function prototype directly
				// into the ParseVariable so argument validation sees it as initialised,
				// matching what happens when the assignment is split across separate lines
				if (!funcData->returnValue.empty())
				{
					retVar->clearDataTypes();
					for (auto dt : funcData->returnValue)
						retVar->addDataType(dt);
					(*_pVariables)[retVar->name()] = funcData->returnValue;
				}
				else
				{
					retVar->clearDataTypes();
					retVar->addDataType(DataTypes::Unknown);
					std::unordered_set<DataTypes> dt;
					dt.insert(DataTypes::Unknown);
					(*_pVariables)[retVar->name()] = dt;
				}

				if (!_doGlobalFunction(funcData, func, inlineState))
					error = true;
				prop->setGetterFunction(func);
			}
		}
		else
		{
			_addError(ParseErrors::UnknownProperty, prop);
			error = true;
		}
	}

	// no getter or setter, must be part of an expression,  so generate a variable
	if (!prop->setter() && !prop->getter())
	{
		auto funcData = _data->findObjectPropertyGetter(prop->property());
		if (funcData)
		{
			ParseFunction* func = new ParseFunction(prop->line(), funcData->name);
			func->setLinePosition(prop->linePos());
			func->setFile(_currentFile.back());
			func->setPosition(prop->startingPos(), prop->endingPos());

			ParseArguments* args = new ParseArguments(prop->line());
			args->setLinePosition(prop->linePos());
			args->setFile(_currentFile.back());
			args->setPosition(prop->startingPos(), prop->endingPos());
			func->setArguments(args);
			func->setObject(prop->object());

			if (!_doGlobalFunction(funcData, func, inlineState))
				error = true;
			prop->setGetterFunction(func);
		}
		else
		{
			_addError(ParseErrors::UnknownProperty, prop);
			error = true;
		}
	}

	return !error;
}
bool CScriptParser::_runArrayFunction(ParseArray* arr, InlineState inlineState, bool doAssignment)
{
	ParseFunction* func = new ParseFunction(arr->line(), L"ARRAY");
	func->setLinePosition(arr->linePos());
	func->setFile(_currentFile.back());
	func->setPosition(arr->startingPos(), arr->endingPos());

	ParseArguments* args = new ParseArguments(arr->line());
	args->setLinePosition(arr->linePos());
	args->setFile(_currentFile.back());
	args->setPosition(arr->startingPos(), arr->endingPos());
	func->setArguments(args);

	args->addParse(arr->variable());
	args->addParse(arr->value());

	bool isDouble = false;
	if (arr->value2())
	{
		args->addParse(arr->value2());
		isDouble = true;
	}

	SpecialFunction specialFunc = (isDouble) ? SpecialFunction::GetArrayDouble : SpecialFunction::GetArray;
	if (arr->assign() && !doAssignment)
	{
		specialFunc = (isDouble) ? SpecialFunction::SetArrayDouble : SpecialFunction::SetArray;
		args->addParse(arr->assign());
	}
	else if (arr->assignment())
	{
		if (arr->assignment()->type() == ParseType::Condition)
			func->setCondition(dynamic_cast<ParseCondition*>(arr->assignment()));
		else if (arr->assignment()->type() == ParseType::Variable)
			func->setReturnVariable(dynamic_cast<ParseVariable*>(arr->assignment()));
		else if (arr->assignment()->type() == ParseType::Array)
		{
			ParseArray* toArray = dynamic_cast<ParseArray*>(arr->assignment());
			if (!isDouble && !toArray->value2())
			{
				specialFunc = SpecialFunction::SetArrayFromArray;
				args->insertParse(toArray->value());
				args->insertParse(toArray->variable());
			}
		}
	}
	else if (arr->variable()->type() == ParseType::Function)
	{
		auto varFunc = dynamic_cast<ParseFunction*>(arr->variable());
		if (varFunc->returnVariable())
			func->setReturnVariable(varFunc->returnVariable());
		else
			inlineState = InlineState::Inline;
	}

	auto funcData = _data->getSpecialGlobalFunction(specialFunc);
	bool ret = _doGlobalFunction(funcData, func, inlineState);

	if (arr->preRun()) {
		if (!_runParse(arr->preRun(), nullptr, nullptr, true, InlineState::Normal)) return false;
	}

	if (ret)
	{
		if (!arr->assignment() && func->returnVariable())
			arr->setAssignment(const_cast<ParseVariable*>(func->returnVariable()));

		// check data type of value
		if (arr->variable() && arr->value())
		{
			std::unordered_set<DataTypes> dataTypes = _getActualDataTypes(arr->variable());
			if (!dataTypes.empty())
			{
				if (dataTypes.find(DataTypes::Array) != dataTypes.end() && dataTypes.find(DataTypes::Table) == dataTypes.end())
				{
					const auto& actualDt = _getActualDataTypes(arr->value());
					if (!_isValidDataType(DataTypes::Number, actualDt))
					{
						Warnings& warn = _addWarning(ParseWarnings::InvalidArrayValue, arr->value());
						warn.data.push_back(arr->value()->data());
						warn.data.push_back(_getDataTypesString(actualDt));
					}
				}
			}
		}
	}

	arr->setFunction(func);

	if (ret && arr->assign() && arr->assignment() && !doAssignment)
		ret = _runArrayFunction(arr, inlineState, true);
	if (ret && arr->assignment() && arr->assignment()->type() == ParseType::Array)
	{
		const ParseArray* toArray = dynamic_cast<const ParseArray*>(arr->assignment());
		if (toArray->assignment())
			ret = _runArrayFunction(const_cast<ParseArray*>(toArray), inlineState, false);
	}

	return ret;
}
bool CScriptParser::_runFunction(ParseFunction* function, InlineState inlineState)
{
	if (function->object())
	{
		std::unordered_set<DataTypes> dt = _getActualDataTypes(function->object());
		if (!dt.empty())
		{
			if (dt.size() == 1)
			{
				if (*dt.begin() != DataTypes::Unknown)
				{
					const Function* func = _data->findObjectTypeFunction(*dt.begin(), function->function());
					if (func)
						return _doGlobalFunction(func, function, inlineState);
				}
			}
			else
			{
				const Function* func = NULL;
				int funcCount = 0;
				for (auto itr = dt.begin(); itr != dt.end(); itr++)
				{
					func = _data->findObjectTypeFunction(*itr, function->function());
					if (func)
						++funcCount;
				}

				if (func && funcCount == 1)
					return _doGlobalFunction(func, function, inlineState);

				else if (funcCount > 1)
				{
					ParseFail* fail = new ParseFail(function, ParseErrors::AmbiguousObjectFunction);
					fail->addData(function->function());
					fail->addData(_getDataTypesString(dt));
					_errors.push_back(fail);
					return false;
				}
			}
		}

		const Function* func = _data->findObjectFunction(function->function());
		if (func)
			return _doGlobalFunction(func, function, inlineState);

		ParseFail* fail = new ParseFail(function, ParseErrors::UnknownObjectFunction);
		fail->addData(function->function());
		_errors.push_back(fail);
		return false;
	}
	else
	{
		//first check for any internal functions
		InternalFunctions ifunc = _data->findInternalFunction(function->function());
		if (ifunc != InternalFunctions::Unknown)
			return _doInternalFunction(ifunc, function->arguments());

		// now check any global functions — use findBestGlobalFunction to resolve overloads
		int argCount = function->arguments() ? static_cast<int>(function->arguments()->count()) : 0;
		const Function* func = _data->findBestGlobalFunction(function->function(), argCount, function->arguments());
		if (func)
			return _doGlobalFunction(func, function, inlineState);

		ParseFail* fail = new ParseFail(function, ParseErrors::UnknownFunction);
		fail->addData(function->function());
		_errors.push_back(fail);
		return false;
	}
}
bool CScriptParser::_doInternalFunction(InternalFunctions func, const ParseArguments* arguments)
{
	switch (func)
	{
	case InternalFunctions::SetArguments:
		return _setArguments(arguments);
	case InternalFunctions::SetDescription:
		return _setDescription(arguments);
	case InternalFunctions::SetVersion:
		return _setVersion(arguments);
	case InternalFunctions::SetCommand:
		return _setCommand(arguments);
	}

	ParseFail* fail = new ParseFail(arguments, ParseErrors::InternalFunctionError);
	_errors.push_back(fail);
	return false;
}

bool CScriptParser::_doGlobalFunction(const Function* func, ParseFunction* functionData, InlineState inlineState)
{
	// first check if continue or break are inside a while loop
	if (func->id == _data->breakCommand() || func->id == _data->continueCommand())
	{
		if (!_prePassMode)
		{
			// Use the condition stack directly — more reliable than CScript::isInWhile()
			// which scans _functions and can miscount when if blocks precede the while.
			bool inWhile = false;
			for (const auto& entry : _conditionStack)
				if (entry.type == ConditionType::While) { inWhile = true; break; }

			if (!inWhile)
			{
				_addError(ParseErrors::MissingWhile, functionData);
				return false;
			}
		}

		// Before continue, re-emit the nearest while's temp var assignments
		// so the while condition re-evaluates with fresh values.
		if (func->id == _data->continueCommand())
			_emitWhileReEval();
	}

	// When we encounter a gosub during the real pass, merge that sub's
	// pre-collected variable types into _variables so that the code
	// following the gosub can use them without false "unknown type" warnings.
	if (!_prePassMode && func->id == _data->gosubCommand())
	{
		if (functionData->arguments() && functionData->arguments()->count() > 0)
		{
			const BaseParse* arg = functionData->arguments()->get(0);
			if (arg->type() == ParseType::Label)
			{
				const ParseLabel* label = dynamic_cast<const ParseLabel*>(arg);
				auto subItr = _subVariables.find(label->label());
				if (subItr != _subVariables.end())
				{
					// Sub variables always take precedence over the current global map —
					// they represent the actual runtime state after the sub executes.
					for (const auto& subVar : subItr->second)
					{
						if (!subVar.second.empty())
							(*_pVariables)[subVar.first] = subVar.second;
					}
				}
			}
		}
	}

	// During pre-pass, endsub/return at the top level (depth 0) ends the sub.
	// If we're inside a condition block (depth > 0) it's a conditional return —
	// the sub continues, so don't terminate the pre-scan yet.
	if (_prePassMode && _prePassDepth == 0 &&
		(func->id == _data->returnCommand() || func->id == _data->endCommand()))
	{
		_subEndedOnLine = true;
		_currentSubLabel.clear();
		_pVariables = &_variables; // redirect writes back to global map
	}

	ParseExpression* runExpr = NULL;

	// if we have a retvar and the function does
	if (func->returnValueType == RetVarType::None && functionData->returnVariable())
	{
		_addError(ParseErrors::FunctionWithoutRetVar, functionData);
		return false;
	}
	//check if the function requires a retvar, but we dont have one
	else if ((func->returnValueType == RetVarType::Return || func->returnValueType == RetVarType::If) && !functionData->returnVariable() && func->returnArgument <= 0 && !functionData->condition())
	{
		// generate a return value
		if (_isInline(inlineState))
		{
			ParseVariable* var = new ParseVariable(functionData->line(), _makeTempVarName(), &func->returnValue);
			(*_pVariables)[var->name()] = func->returnValue;
			functionData->setReturnVariable(var);
		}
		else
		{
			_addError(ParseErrors::FunctionRequiresRetVar, functionData);
			return false;
		}
	}
	// invalid start condition
	else if (func->returnValueType != RetVarType::NoIfStart && functionData->condition() && dynamic_cast<const ParseCondition*>(functionData->condition())->condition() == Conditions::Start)
	{
		_addError(ParseErrors::InvalidStartCondition, functionData);
		return false;
	}
	// return type not compatable with condition
	else if (func->returnValueType == RetVarType::Return && functionData->condition())
	{
		const ParseCondition* cond = dynamic_cast<const ParseCondition*>(functionData->condition());
		bool isWhileCondition = (cond->condition() == Conditions::While || cond->condition() == Conditions::WhileNot);

		if (!isWhileCondition)
		{
			// For functions like inc/dec where returnArgument > 0, the argument at
			// that index doubles as the return value — use it directly instead of
			// generating a temp variable.
			ParseVariable* var = nullptr;
			if (func->returnArgument > 0 && functionData->arguments() &&
				(int)functionData->arguments()->count() > func->returnArgument - 1)
			{
				const BaseParse* arg = functionData->arguments()->get(func->returnArgument - 1);
				if (arg && arg->type() == ParseType::Variable)
				{
					const ParseVariable* argVar = dynamic_cast<const ParseVariable*>(arg);
					// Create a fresh ParseVariable with the same name — owned by
					// functionData via setReturnVariable, not added to _createdData.
					// Do NOT pre-register in _pVariables here — the normal argument
					// processing path will handle it, preserving uninitialised warnings.
					var = new ParseVariable(functionData->line(), argVar->name(), &func->returnValue);
					var->setFromParse(functionData);
				}
				else if (arg && arg->type() == ParseType::Expression)
				{
					// Expression argument like inc($i + 10) — find the first variable
					// in the expression and use it as the assignment target.
					// The argument processing loop will set the assignment on the expression;
					// we just need to find the variable name for the retvar here.
					const ParseExpression* argExpr = dynamic_cast<const ParseExpression*>(arg);
					for (auto item : argExpr->list())
					{
						if (item->type() == ParseType::Variable)
						{
							const ParseVariable* argVar = dynamic_cast<const ParseVariable*>(item);
							var = new ParseVariable(functionData->line(), argVar->name(), &func->returnValue);
							var->setFromParse(functionData);
							functionData->setReturnVariable(var);
							break;
						}
					}
				}
			}
			if (!var)
			{
				var = new ParseVariable(functionData->line(), _makeTempVarName(), &func->returnValue);
				(*_pVariables)[var->name()] = func->returnValue;
			}
			functionData->setReturnVariable(var);

			ParseExpression* expr = new ParseExpression(functionData->line());
			expr->setFromParse(functionData);
			expr->setCondition(functionData->condition());

			ParseVariable* newVar = new ParseVariable(functionData->line(), var->name(), &func->returnValue);
			newVar->setFromParse(functionData);
			expr->addParse(newVar);

			_createdData.push_back(expr);
			functionData->setCondition(NULL);
			runExpr = expr;
			// Store the mapping so the StartBlock handler can find this expression
			// when '{' follows the function in the parse list.
			_createdExpressions[functionData] = expr;
		}
	}

	if (functionData->object())
	{
		// has a refobj, but the function doesn't require one
		if (func->refObjType.empty())
		{
			const_cast<ParseArguments*>(functionData->arguments())->insertParse(const_cast<BaseParse*>(functionData->object()));
			functionData->setObject(NULL);
		}
		//validate the refobj type
		else
		{
			// get the actual data type from the function (depending on what parse type it is)
			std::unordered_set<DataTypes> dt = _getActualDataTypes(functionData->object());
			if (dt.empty())
			{
				Warnings& warn = _addWarning(ParseWarnings::NullVariable, functionData->object());
				warn.data.push_back(functionData->object()->data());
			}
			if (!dt.empty() && dt.find(DataTypes::Unknown) == dt.end() && !func->refObjType.empty())
			{
				// match the different object types, ie "Object" should match with all object types
				if (!_isValidDataType(func->refObjType, dt))
				{
					Warnings& warn = _addWarning(ParseWarnings::InvalidObjectDataType, functionData->object());
					warn.data.push_back(functionData->object()->data());
					warn.data.push_back(_getDataTypesString(func->refObjType));
					warn.data.push_back(_getDataTypesString(dt));
				}
			}
		}
	}
	// validate function
	size_t argCount = func->arguments.size();
	if ((functionData->arguments()->count()) != argCount)
	{
		if (func->allowNull && functionData->arguments()->count() < argCount)
		{
			for (size_t i = functionData->arguments()->count(); i < argCount; i++)
				const_cast<ParseArguments*>(functionData->arguments())->addParse(new ParseNull(L""));
		}
		else if (func->undefinedCount && functionData->arguments()->count() > argCount)
		{
		}
		else
		{
			ParseFail* fail = new ParseFail(functionData, ParseErrors::InvalidArgumentCount);
			fail->addData(std::to_wstring(functionData->arguments()->count()));
			fail->addData(std::to_wstring(argCount));
			_errors.push_back(fail);
			return false;
		}
	}

	// validation argument datatypes
	std::map<int, const ParseInteger*> foldedNegatives; // index → folded negative integer
	for (int i = 0; i < func->arguments.size(); ++i)
	{
		const FunctionArgument& a = func->arguments[i];
		const BaseParse* arg = functionData->arguments()->get(i);
		const ParDefData* d = _data->getParDefData(a.pardef);

		//if the type is unknown, we will allow it
		if (arg->type() == ParseType::Function)
		{
			// check the return value of the function
			const ParseFunction* function = dynamic_cast<const ParseFunction*>(arg);
			if (!_runFunction(const_cast<ParseFunction*>(function), InlineState::Inline))
				return false;

			if (function->returnVariable() && static_cast<int>(d->flags & ParDefFlags::Variable))
			{
				std::unordered_set<DataTypes> dt = _getActualDataTypes(function->returnVariable());
				if (!_isValidDataType(dt, d->datatypes))
				{
					Warnings& warn = _addWarning(ParseWarnings::InvalidDataType, arg);
					warn.data.push_back(arg->data());
					warn.data.push_back(std::to_wstring(i));
					warn.data.push_back(_getDataTypesString(dt));
					warn.data.push_back(_getDataTypesString(d->datatypes));
				}
			}
		}
		else if (arg->type() == ParseType::Array)
		{
			const ParseArray* arr = dynamic_cast<const ParseArray*>(arg);
			if (!_runArrayFunction(const_cast<ParseArray*>(arr), InlineState::Inline, false))
				return false;
		}
		else if (arg->type() == ParseType::Property)
		{
			const ParseProperty* arr = dynamic_cast<const ParseProperty*>(arg);
			if (!_runProperty(const_cast<ParseProperty*>(arr), InlineState::Inline, false))
				return false;
		}
		else if (arg->type() == ParseType::Expression)
		{
			const ParseExpression* expr = dynamic_cast<const ParseExpression*>(arg);

			// If the expression is a simple unary negation of an integer literal
			// (e.g. -1 → [Operator(-), Integer(1)]), fold it into a single negative
			// ParseInteger and use it directly — no temp var needed.
			if (!expr->assignment() && expr->list().size() == 2)
			{
				const BaseParse* first = expr->list()[0];
				const BaseParse* second = expr->list()[1];
				if (first->type() == ParseType::Operator && second->type() == ParseType::Integer)
				{
					const ParseOperator* op = dynamic_cast<const ParseOperator*>(first);
					if (op->operType() == Operators::Subtract)
					{
						int negVal = -dynamic_cast<const ParseInteger*>(second)->value();
						ParseInteger* negInt = new ParseInteger(expr->line(), negVal);
						negInt->setFromParse(expr);
						_createdData.push_back(negInt);
						foldedNegatives[i] = negInt;
						continue;
					}
				}
			}

			// add the generated variable assignment
			if (!expr->assignment())
			{
				ParseVariable* var = nullptr;

				// For functions where this argument doubles as the return value
				// (returnArgument > 0), use the first variable in the expression
				// as the assignment target instead of generating a temp var.
				// e.g. inc($i + 10) → $i = $i + 10, not $VarGen.1 = $i + 10
				if (func->returnArgument > 0 && (i == func->returnArgument - 1))
				{
					for (auto item : expr->list())
					{
						if (item->type() == ParseType::Variable)
						{
							const ParseVariable* exprVar = dynamic_cast<const ParseVariable*>(item);
							var = new ParseVariable(functionData->line(), exprVar->name(), &func->returnValue);
							var->setFromParse(functionData);
							// Do NOT register in _pVariables here — register after the
							// expression runs so the uninitialised variable warning fires.
							break;
						}
					}
				}

				if (!var)
				{
					var = new ParseVariable(functionData->line(), _makeTempVarName(), &func->returnValue);
					(*_pVariables)[var->name()] = func->returnValue;
				}
				const_cast<ParseExpression*>(expr)->setAssignment(var);
			}

			if (!_runExpressionList(expr, false, nullptr, _isCondition(inlineState)))
				return false;

			// Register the return variable AFTER the expression runs so any
			// uninitialised warnings for variables inside the expression fire correctly.
			if (func->returnArgument > 0 && (i == func->returnArgument - 1) &&
				expr->assignment() && expr->assignment()->type() == ParseType::Variable)
			{
				const ParseVariable* assignVar = dynamic_cast<const ParseVariable*>(expr->assignment());
				const std::wstring& name = assignVar->name();
				// Only register named variables (not temp vars like $VarGen.N)
				if (name.size() < 8 || name.substr(0, 8) != L"$VarGen.")
					(*_pVariables)[name] = func->returnValue;
			}

			if (expr->dataType() != DataTypes::Unknown)
			{
				if (!_isValidDataType(expr->dataType(), d->datatypes))
				{
					Warnings& warn = _addWarning(ParseWarnings::InvalidDataType, arg);
					warn.data.push_back(expr->data());
					warn.data.push_back(std::to_wstring(i));
					warn.data.push_back(_data->getDataTypeName(expr->dataType()));
					warn.data.push_back(_getDataTypesString(d->datatypes));
				}
			}
		}
		else if (arg->type() == ParseType::Variable && static_cast<int>(d->flags & ParDefFlags::Variable))
		{
			const ParseVariable* vari = dynamic_cast<const ParseVariable*>(arg);
			auto dts = vari->currentDataTypes();
			if (dts.empty()) dts = (*_pVariables)[vari->name()];
			// not data types
			if (dts.empty())
			{
				Warnings& warn = _addWarning(ParseWarnings::NullVariable, arg);
				warn.data.push_back(arg->data());
			}
			else if (!d->datatypes.empty() && !_isValidDataType(dts, d->datatypes))
			{
				Warnings& warn = _addWarning(ParseWarnings::InvalidDataType, arg);
				warn.data.push_back(arg->data());
				warn.data.push_back(std::to_wstring(i));
				warn.data.push_back(_getDataTypesString(dts));
				warn.data.push_back(_getDataTypesString(d->datatypes));
			}
		}
		else if (arg->type() == ParseType::Constant && static_cast<int>(d->flags & ParDefFlags::Constant))
		{
			const ParseConstant* constant = dynamic_cast<const ParseConstant*>(arg);
			if (constant->dataType() == DataTypes::Unknown)
				continue;

			// add warnings about datatype mismatch
			if (constant->subType() != DataTypes::Unknown && !_isValidDataType(constant->subType(), d->datatypes))
			{
				Warnings& warn = _addWarning(ParseWarnings::InvalidDataType, arg);
				warn.data.push_back(arg->data());
				warn.data.push_back(std::to_wstring(i));
				warn.data.push_back(_data->getDataTypeName(constant->subType()));
				warn.data.push_back(_getDataTypesString(d->datatypes));
			}

			// check for constant group
			if (a.constGroup && a.constGroup != constant->constGroup())
			{
				Warnings& warn = _addWarning(ParseWarnings::InvalidConstantGroup, constant);
				warn.data.push_back(constant->data());
				warn.data.push_back(std::to_wstring(i));
				warn.data.push_back(a.constGroup->name);
			}

		}
		else if (arg->type() == ParseType::String && static_cast<int>(d->flags & ParDefFlags::String))
			continue;
		else if (arg->type() == ParseType::Integer && static_cast<int>(d->flags & ParDefFlags::Integer))
			continue;
		else if (!d->datatypes.empty() && arg->type() != ParseType::Null)
		{
			ParseFail* fail = new ParseFail(arg, ParseErrors::InvalidArgumentType);
			fail->addData(_formatString(arg->dataType(), arg->data()));
			fail->addData(std::to_wstring(i));
			fail->addData(_getPardefFlagString(d->flags));
			_errors.push_back(fail);
			return false;
		}
	}

	// store datatype of return value
	// if the command has an argument that counts as a return (ie Inc/Dec)
	if (func->returnValueType != RetVarType::None)
	{
		if (functionData->returnVariable())
		{
			functionData->returnVariable()->setDataTypes(func->returnValue);
			(*_pVariables)[functionData->returnVariable()->name()] = func->returnValue;
		}

		if (func->returnArgument > 0)
		{
			if (func->returnArgument <= (int)functionData->arguments()->count())
			{
				const BaseParse* arg = functionData->arguments()->get(func->returnArgument - 1);
				if (arg->type() == ParseType::Variable)
				{
					const ParseVariable* varArg = dynamic_cast<const ParseVariable*>(arg);
					(*_pVariables)[varArg->name()] = func->returnValue;

					if (!_prePassMode)
						_currentScript->addVariable(varArg->name());

					// once we have parsed the function, we can move it to the retvar
					if (!functionData->returnVariable())
						functionData->setReturnVariable(const_cast<ParseVariable*>(varArg));
				}
				else if (arg->type() == ParseType::Expression && functionData->returnVariable())
				{
					const ParseVariable* retVar = dynamic_cast<const ParseVariable*>(functionData->returnVariable());
					if (retVar)
					{
						(*_pVariables)[retVar->name()] = func->returnValue;
						if (!_prePassMode)
							_currentScript->addVariable(retVar->name());
						// Assignment on the expression is already set by the argument
						// processing loop above — no setAssignment needed here.
					}
				}
			}
		}
	}

	// In pre-pass mode we only want variable type tracking — skip all script output
	if (_prePassMode)
		return true;

	// When running as an inline function inside an else-if condition expression,
	// insert BEFORE the opening if block rather than appending after the end block.
	bool isInsideElseIf = false;
	if (_isCondition(inlineState) && !_conditionStack.empty() && !functionData->condition())
	{
		ConditionType top = _conditionStack.back().type;
		isInsideElseIf = (top == ConditionType::ElseIf);
	}

	if (isInsideElseIf)
		_currentScript->insertFunction(func->id, functionData);
	else
		_currentScript->addFunction(func->id, functionData, functionData->isPostRun(), _isInline(inlineState));

	// if the function has a refobj of null, add a null item — but only
	// when no object has already been provided by the caller.
	if (!functionData->object() && func->refObjType.size() > 1 && func->refObjType.find(DataTypes::Null) != func->refObjType.end())
	{
		auto constData = _data->findConstant(L"NULL");
		if (constData)
			functionData->setObject(new ParseConstant(functionData->line(), constData));
		else
			functionData->setObject(new ParseNull(functionData->line()));
	}

	for (auto itr = func->order.begin(); itr != func->order.end(); itr++)
	{
		int addArg = -1;
		std::wstring a = *itr;
		if (a == L"RetVar")
		{
			if (functionData->condition())
				_currentScript->addFunctionCondition(functionData->condition());
			else if (func->returnArgument > 0 && func->returnArgument <= (int)functionData->arguments()->count())
			{
				const BaseParse* arg = functionData->arguments()->get(func->returnArgument - 1);
				if (arg->type() == ParseType::Expression && functionData->returnVariable())
					_currentScript->addRetVar(functionData->returnVariable());
				else
					_currentScript->addRetVar(arg);
			}
			else if (functionData->returnVariable())
			{
				const ParseVariable* vari = dynamic_cast<const ParseVariable*>(functionData->returnVariable());
				if (func->returnValue.empty())
				{
					(*_pVariables)[vari->name()].clear();
					(*_pVariables)[vari->name()].insert(DataTypes::Unknown);
				}
				else
					(*_pVariables)[vari->name()] = func->returnValue;

				_currentScript->addRetVar(functionData->returnVariable());
			}
			else
			{
				functionData->setCondition(new ParseCondition(L"", Conditions::None));
				_currentScript->addFunctionCondition(functionData->condition());
			}
		}
		else if (a == L"RefObj" && functionData->object())
			_currentScript->addFunctionArgument(functionData->object(), ParDef::Unknown);
		else if (a == L"Arg0")
			addArg = 0;
		else if (a == L"Arg1")
			addArg = 1;
		else if (a == L"Arg2")
			addArg = 2;
		else if (a == L"Arg3")
			addArg = 3;
		else if (a == L"Arg4")
			addArg = 4;
		else if (a == L"Arg5")
			addArg = 5;
		else if (a == L"Arg6")
			addArg = 6;
		else if (a == L"Arg7")
			addArg = 7;
		else if (a == L"Arg8")
			addArg = 8;
		else if (a == L"Arg9")
			addArg = 9;

		if (addArg >= 0)
		{
			ParDef pardef = ParDef::Unknown;
			if (func->arguments.size() >= addArg)
				pardef = func->arguments[addArg].pardef;

			if (functionData->arguments()->count() >= addArg)
			{
				// Use folded negative integer if this argument was simplified
				auto foldedItr = foldedNegatives.find(addArg);
				const BaseParse* argToAdd = (foldedItr != foldedNegatives.end())
					? static_cast<const BaseParse*>(foldedItr->second)
					: functionData->arguments()->get(addArg);
				_currentScript->addFunctionArgument(argToAdd, pardef);
			}
		}
	}


	// if undefined count, then also add them after the rest
	if (func->undefinedCount)
	{
		_currentScript->setFunctionUndefinedCount(static_cast<unsigned int>(functionData->arguments()->count() - func->arguments.size()));
		for (size_t i = func->arguments.size(); i < functionData->arguments()->count(); i++)
		{
			auto foldedItr = foldedNegatives.find(static_cast<int>(i));
			const BaseParse* argToAdd = (foldedItr != foldedNegatives.end())
				? static_cast<const BaseParse*>(foldedItr->second)
				: functionData->arguments()->get(i);
			_currentScript->addFunctionArgument(argToAdd, ParDef::Unknown);
		}
	}

	if (runExpr)
	{
		if (!_runExpressionList(runExpr, true, nullptr, _isCondition(inlineState)))
			return false;
	}

	return true;
}

bool CScriptParser::_parseDataList(std::vector<const BaseParse*>& list)
{
	// replace the entries with brackets
	if (!parseListBrackets(list))
		return false;

	// replace the conditions
	if (!_parseAllConditions(list))
		return false;

	// replace the conditions
	if (!_parseNamespaces(list))
		return false;

	// replace the constant
	{
		std::vector<const BaseParse*> originalList(list);
		list.clear();
		if (!parseConstants(originalList, list))
			return false;
	}

	// Desugar ++, --, +=, -=, *=, /= before any other processing
	if (!_parseCompoundAssignment(list))
		return false;

	// find all properties		
	{
		std::vector<const BaseParse*> originalList(list);
		list.clear();
		if (!findProperties(originalList, list))
			return false;
	}

	// find all function calls
	{
		std::vector<const BaseParse*> originalList(list);
		list.clear();
		if (!parseFunctions(originalList, list))
			return false;
	}

	// find all arrays
	if (!_findArrays(list, true))
		return false;

	// parse all properties	
	{
		std::vector<const BaseParse*> originalList(list);
		list.clear();
		if (!parseProperties(originalList, list))
			return false;
	}


	// replace the array brackets
	if (!_parseArrays(list, true))
		return false;

	// check for errors
	if (!_checkListOrder(list, ParseErrors::MissingSemiColon))
		return false;

	// special case for empty if statment
	if (list.size() == 2 && list.front()->type() == ParseType::Condition && list.back()->type() == ParseType::Brackets)
	{
		ParseSymbol* start = new ParseSymbol(list.back()->line(), L"{");
		start->setFromParse(list.back());
		ParseSymbol* end = new ParseSymbol(list.back()->line(), L"}");
		end->setFromParse(list.back());
		list.push_back(start);
		list.push_back(end);
	}

	// create expression list
	{
		std::vector<const BaseParse*> originalList(list);
		list.clear();
		if (!_parseExpressions(originalList, list))
			return false;
	}

	// check for expression errors
	if (!_checkExpressionValidity(list, true))
		return false;

	return true;
}

void CScriptParser::_emitWhileReEval()
{
	// Find the nearest While entry on the condition stack and:
	// 1. Re-run inline functions that generated $VarGenWhile temp vars
	// 2. Run any captured post-run (inc/dec) nodes that were suppressed at the while level
	// 3. Duplicate any post-run entries that were flushed into _functions
	for (auto itr = _conditionStack.rbegin(); itr != _conditionStack.rend(); ++itr)
	{
		if (itr->type != ConditionType::While || !itr->whileExpression)
			continue;

		// Re-run inline functions that generated $VarGenWhile temp vars
		_emitWhileReEvalList(itr->whileExpression->list());

		// Run captured inc/dec nodes at continue/end.
		// For post-increment (isPostRun==true originally): clear flag so addFunction
		// writes immediately, then restore.
		// For pre-increment (isPostRun==false): just run directly, no flag change.
		for (const BaseParse* node : itr->whilePostRunNodes)
		{
			ParseFunction* fn = const_cast<ParseFunction*>(dynamic_cast<const ParseFunction*>(node));
			bool wasPostRun = fn->isPostRun();
			if (wasPostRun)
				fn->setPostRun(false);
			_runParse(node, nullptr, nullptr, false, InlineState::Normal);
			if (wasPostRun)
				fn->setPostRun(true);
		}

		// Duplicate any post-run entries flushed into _functions (other cases)
		for (size_t i = 0; i < itr->whilePostRunCount; i++)
			_currentScript->duplicateFunction(itr->whilePostRunStart + i);

		return; // innermost while only
	}
}

void CScriptParser::_emitWhileReEvalList(const std::vector<const BaseParse*>& list)
{
	// Recursively walk the parse list, re-running only nodes that produced
	// $VarGenWhile temp variables or are inline properties/arrays that may have.
	for (auto item : list)
	{
		if (item->type() == ParseType::Expression)
		{
			// Recurse into nested expressions
			const ParseExpression* expr = dynamic_cast<const ParseExpression*>(item);
			_emitWhileReEvalList(expr->list());
		}
		else if (item->type() == ParseType::Function)
		{
			// Only re-run if this function produced a $VarGenWhile temp var
			const ParseFunction* fn = dynamic_cast<const ParseFunction*>(item);
			if (fn->returnVariable() &&
				fn->returnVariable()->name().substr(0, 13) == L"$VarGenWhile.")
			{
				_runParse(item, nullptr, nullptr, false, InlineState::Inline);
			}
		}
		else if (item->type() == ParseType::Property ||
			item->type() == ParseType::Array)
		{
			// ParseProperty and ParseArray generate temp vars on internal ParseFunction
			// objects — we can't check returnVariable() directly.
			// Re-run them unconditionally if they appear in the while expression.
			_runParse(item, nullptr, nullptr, false, InlineState::Inline);
		}
	}
}

void CScriptParser::_addExpressionItem(const BaseParse* parse)
{
	if (parse->type() == ParseType::Brackets)
	{
		const ParseBrackets* bracket = dynamic_cast<const ParseBrackets*>(parse);
		auto item = bracket->singleItem();
		if (item)
		{
			if (item->type() == ParseType::Expression)
				_currentScript->addFunctionArgument(item, ParDef::Unknown);
			else
				_addExpressionItem(item);
		}
		else
		{
			for (auto itr = bracket->list().begin(); itr != bracket->list().end(); itr++)
				_addExpressionItem(*itr);
		}
	}
	else if (parse->type() == ParseType::Expression)
	{
		auto expression = dynamic_cast<const ParseExpression*>(parse);
		if (expression->assignment())
			_addExpressionItem(expression->assignment());
		else if (expression->list().size() == 1)
			_addExpressionItem(expression->list()[0]);
		else
			_currentScript->addFunctionArgument(expression, ParDef::Unknown);
	}
	else if (parse->type() == ParseType::Array)
	{
		auto arr = dynamic_cast<const ParseArray*>(parse);
		if (!arr->assignment())
		{
			_generatedVariables++;
			std::unordered_set<DataTypes> types;
			types.insert(DataTypes::Unknown);
			ParseVariable* var = new ParseVariable(arr->line(), _makeTempVarName(), &types);
			(*_pVariables)[var->name()] = types;
			const_cast<ParseArray*>(arr)->setAssignment(var);
		}
		_currentScript->addFunctionArgument(parse, ParDef::Unknown);
	}
	else
		_currentScript->addFunctionArgument(parse, ParDef::Unknown);
}

bool CScriptParser::_addLabel(const ParseKeyword* keyword)
{
	if (_prePassMode)
	{
		// During pre-pass, record which sub we're entering so variable
		// assignments can be stored in that sub's map.
		_currentSubLabel = keyword->keyword();
		_subVariables[_currentSubLabel]; // ensure entry exists
		_pVariables = &_subVariables[_currentSubLabel]; // redirect writes to sub map
		_prePassDepth = 0;
		return true;
	}

	if (_currentScript->addLabel(keyword))
	{
		_createdData.push_back(keyword);
		return true;
	}

	return false;
}

bool CScriptParser::_runExpressionList(const ParseExpression* expression, bool topLevel, const ParseExpression* previousExpr, bool isInCondition)
{
	auto& list = expression->list();

	const ScriptFunction* previousFunc = _currentScript->previousFunction();
	bool addEndBlock = false;
	if (previousExpr && previousExpr->condition() && !previousExpr->condition()->isBlock())
		addEndBlock = true;
	if (previousExpr && !previousExpr->condition() && !previousExpr->list().empty() && previousExpr->list().front()->type() == ParseType::Function) {
		const ParseFunction* fn = dynamic_cast<const ParseFunction*>(previousExpr->list().front());
		if (fn->condition() && !fn->condition()->isBlock())
			addEndBlock = true;
	}

	// Push the condition stack entry immediately so that any inline functions
	// processed in the first loop below know what condition context they're in.
	size_t functionCount = _currentScript->functionCount();
	bool isCurrentCondition = false;
	if (!_prePassMode && expression->condition())
	{
		const ParseCondition* cond = dynamic_cast<const ParseCondition*>(expression->condition());
		Conditions condType = cond->condition();
		bool isWhile = (condType == Conditions::While || condType == Conditions::WhileNot);
		isCurrentCondition = condType != Conditions::None;
		if (isWhile)
			_conditionStack.push_back(ConditionEntry(ConditionType::While, expression));
		else if (condType == Conditions::If || condType == Conditions::IfNot ||
			condType == Conditions::SkipIf || condType == Conditions::SkipIfNot)
			_conditionStack.push_back(ConditionEntry(ConditionType::If));
		else if (condType == Conditions::ElseIf || condType == Conditions::ElseIfNot)
			_conditionStack.push_back(ConditionEntry(ConditionType::ElseIf));
		else if (condType == Conditions::Else)
			_conditionStack.push_back(ConditionEntry(ConditionType::Else));
	}
	else if (!_prePassMode && !expression->condition() && !list.empty())
	{
		// else if: the condition may be on the first function in the list rather
		// than directly on the expression (set by parseFunctions line 2514).
		// Push the condition stack so inline functions know they're inside else-if.
		if (list.front()->type() == ParseType::Function)
		{
			const ParseFunction* fn = dynamic_cast<const ParseFunction*>(list.front());
			if (fn->condition())
			{
				const ParseCondition* cond = dynamic_cast<const ParseCondition*>(fn->condition());
				isCurrentCondition = cond->condition() != Conditions::None;
				if (cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot)
					_conditionStack.push_back(ConditionEntry(ConditionType::ElseIf));
				else if (cond->condition() == Conditions::Else)
					_conditionStack.push_back(ConditionEntry(ConditionType::Else));
			}
		}
	}

	// first do all the functions
	// For while conditions, collect inc/dec nodes that need re-emitting at continue/end.
	// Post-increment (isPostRun==true): skip in first loop entirely, emit only at continue/end.
	// Pre-increment (isPostRun==false, fn is inc/dec): runs normally in first loop (before while),
	// but ALSO needs re-emitting at continue/end — stored separately.
	if (!_prePassMode && expression->condition())
	{
		const ParseCondition* cond = dynamic_cast<const ParseCondition*>(expression->condition());
		Conditions condType = cond->condition();
		if ((condType == Conditions::While || condType == Conditions::WhileNot) &&
			!_conditionStack.empty() && _conditionStack.back().type == ConditionType::While)
		{
			for (auto itr = list.begin(); itr != list.end(); itr++)
			{
				if ((*itr)->type() == ParseType::Function)
				{
					const ParseFunction* fn = dynamic_cast<const ParseFunction*>(*itr);
					if (fn->isPostRun() ||
						fn->function() == L"inc" || fn->function() == L"dec")
					{
						_conditionStack.back().whilePostRunNodes.push_back(fn);
					}
				}
			}
		}
	}

	const BaseParse* previous = NULL;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		const BaseParse* parse = *itr;

		// Skip post-run nodes (post-increment) that have been stored for deferred
		// emission — they must not run here at all.
		// Pre-increment nodes ARE allowed to run here (they go before the while),
		// but are also stored in whilePostRunNodes for re-emission at continue/end.
		if (!_conditionStack.empty() && _conditionStack.back().type == ConditionType::While)
		{
			if (parse->type() == ParseType::Function)
			{
				const ParseFunction* fn = dynamic_cast<const ParseFunction*>(parse);
				if (fn->isPostRun()) // only skip post-run, not pre-increment
				{
					previous = parse;
					continue;
				}
			}
		}

		if (parse->type() == ParseType::Expression)
		{
			if (!_runExpressionList(dynamic_cast<const ParseExpression*>(parse), false, previousExpr, isInCondition))
				return false;
		}
		else if (parse->type() == ParseType::Brackets)
		{
			if (!_runDataList(dynamic_cast<const ParseBrackets*>(parse)->constList(), false))
				return false;
		}
		else
		{
			InlineState state = expression->size() > 1 || !topLevel ? InlineState::Inline : InlineState::Normal;
			// check for condition inline
			if (isInCondition || isCurrentCondition || (expression->condition() && expression->condition()->condition() != Conditions::None))
				state = state == InlineState::Inline ? InlineState::InlineCondition : InlineState::Condition;

			if (!_runParse(*itr, previous, expression->condition(), false, state))
				return false;
		}
		previous = *itr;
	}

	// only do the expression if it has more than just the function call
	bool anyExpression = false;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		if ((*itr)->type() == ParseType::Function || (*itr)->type() == ParseType::Array || (*itr)->type() == ParseType::Property)
			continue;
		else if ((*itr)->type() == ParseType::Symbol)
		{
			const ParseSymbol* sym = dynamic_cast<const ParseSymbol*>(*itr);
			if (sym->symbol() == SymbolType::StartBlock || sym->symbol() == SymbolType::EndBlock)
				continue;
		}

		anyExpression = true;
		break;
	}

	if (!anyExpression && !expression->assignment() && !expression->condition())
	{
		const ParseCondition* prevCond = nullptr;

		if (previousExpr && previousExpr->condition() && !previousExpr->condition()->isBlock())
			prevCond = dynamic_cast<const ParseCondition*>(previousExpr->condition());
		else if (previousExpr && !previousExpr->condition() && !previousExpr->list().empty() && previousExpr->list().front()->type() == ParseType::Function)
		{
			const ParseFunction* fn = dynamic_cast<const ParseFunction*>(previousExpr->list().front());
			if (fn->condition() && !fn->condition()->isBlock())
				prevCond = fn->condition();
		}

		if (prevCond)
		{
			Conditions prevType = prevCond->condition();
			bool prevIsWhile = (prevType == Conditions::While || prevType == Conditions::WhileNot);

			if (!_conditionStack.empty())
			{
				// For single-line while, explicitly add the end block so the parser
				// controls placement — finalise would insert it too late (after post-run).
				// addEndBlock calls flushPostRun() before the endCommand, so $i++ lands inside.
				if (prevIsWhile)
					_emitWhileReEval();

				_conditionStack.pop_back();
				bool anyWhile = false;
				for (auto& e : _conditionStack)
					if (e.type == ConditionType::While) { anyWhile = true; break; }
				if (!anyWhile)
					_whileGeneratedVariables = 0;

				if (prevIsWhile)
					_currentScript->addEndBlock(true);
				else if (prevType == Conditions::ElseIf || prevType == Conditions::Else || prevType == Conditions::ElseIfNot)
					_currentScript->addEndBlock(true);
				else if (addEndBlock)
				{
					size_t diff = _currentScript->functionCount() - functionCount;
					if (diff > 1)
						_currentScript->addEndBlock(true);
				}
			}

		}
		return true;
	}

	bool failed = false;

	// assign the expression variable
	// first search for any comparision operators
	if (expression->isComparison())
		const_cast<ParseExpression*>(expression)->setDataType(DataTypes::Number);
	else if (expression->list().size() == 1 && (expression->list().front()->type() == ParseType::Variable || expression->list().front()->type() == ParseType::Constant) && expression->assignment())
	{
		auto dt = _getActualDataTypes(expression->list().front());
		if (!dt.empty())
			const_cast<ParseExpression*>(expression)->setDataType(*dt.begin());
	}
	else if (expression->list().size() == 1 &&
		(expression->list().front()->type() == ParseType::Function ||
			expression->list().front()->type() == ParseType::Property))
	{
		// Single function/property call — use its actual return datatype directly
		// rather than the number/string heuristic, which would clobber the
		// precise type (e.g. DATATYPE_SECTOR) already stored by _doGlobalFunction.
		auto dt = _getActualDataTypes(expression->list().front());
		if (!dt.empty() && dt.find(DataTypes::Unknown) == dt.end())
			const_cast<ParseExpression*>(expression)->setDataType(*dt.begin());
		else
			const_cast<ParseExpression*>(expression)->setDataType(DataTypes::Unknown);
	}
	else
	{

		// check each data type, if everything is a number, the return will be a number too, otherwise, its always a string
		bool isNumber = true;
		for (auto itr = expression->list().begin(); itr != expression->list().end(); itr++)
		{
			// we cant determine the return value from an array
			if ((*itr)->type() == ParseType::Array)
				continue;
			auto dts = _getActualDataTypes(*itr);
			// unknown data type
			if (dts.empty() || dts.size() == 1 && dts.find(DataTypes::Unknown) != dts.end())
				continue;
			// ignore any operators
			if (dts.find(DataTypes::Operator) != dts.end())
				continue;
			// check for only a number
			if (dts.size() == 1 && dts.find(DataTypes::Number) != dts.end())
				continue;

			isNumber = false;
			break;
		}

		const_cast<ParseExpression*>(expression)->setDataType(isNumber ? DataTypes::Number : DataTypes::String);
	}

	const ParseVariable* retVar = expression->returnValue();
	if (retVar)
	{
		if (expression->list().size() == 1 && (expression->list().front()->type() == ParseType::Variable || expression->list().front()->type() == ParseType::Constant) && expression->assignment())
			(*_pVariables)[retVar->name()] = _getActualDataTypes(expression->list().front());
		else if (expression->list().size() == 1 &&
			(expression->list().front()->type() == ParseType::Function ||
				expression->list().front()->type() == ParseType::Property))
		{
			// For a single function/property, the type was already stored precisely
			// by _doGlobalFunction/_runProperty. Use that rather than the coarser
			// expression->dataType() which resolves everything to Number or String.
			auto dt = _getActualDataTypes(expression->list().front());
			if (!dt.empty() && dt.find(DataTypes::Unknown) == dt.end())
				(*_pVariables)[retVar->name()] = dt;
			else
			{
				std::unordered_set<DataTypes> fallback;
				fallback.insert(expression->dataType());
				(*_pVariables)[retVar->name()] = fallback;
			}
		}
		else
		{
			std::unordered_set<DataTypes> dt;
			dt.insert(expression->dataType());
			(*_pVariables)[retVar->name()] = dt;
		}
	}

	// check for warnings with the expression
	// only some operators are allowed in non number epxressions
	if (expression->dataType() != DataTypes::Number)
	{
		for (auto itr = expression->list().begin(); itr != expression->list().end(); itr++)
		{
			if ((*itr)->type() == ParseType::Operator && dynamic_cast<const ParseOperator*>(*itr)->isNumericOperator())
			{
				auto& warning = _addWarning(ParseWarnings::InvalidExpressionOperator, *itr);
				warning.data.push_back((*itr)->data());
				break;
			}
		}
	}

	// now create the actual expression
	if (_prePassMode)
		return true;

	if (expression->assignment())
	{
		if (isInCondition)
			_currentScript->insertNewExpression(expression->assignment());
		else
			_currentScript->addNewExpression(expression->assignment());
	}
	else if (expression->condition())
	{
		const ParseCondition* cond = dynamic_cast<const ParseCondition*>(expression->condition());
		Conditions condType = cond->condition();
		bool isWhile = (condType == Conditions::While || condType == Conditions::WhileNot);

		// Capture function count before so we can identify post-run entries after flush
		size_t funcCountBefore = isWhile ? _currentScript->functionCount() : 0;

		_currentScript->addNewExpression(cond);

		// After addNewExpression, any pending post-run has been flushed into _functions.
		// Store the index range so _emitWhileReEval can duplicate them before continue/}.
		if (isWhile && !_conditionStack.empty() &&
			_conditionStack.back().type == ConditionType::While)
		{
			size_t postRunStart = funcCountBefore + 1;
			size_t funcCountAfter = _currentScript->functionCount();
			if (postRunStart < funcCountAfter)
			{
				_conditionStack.back().whilePostRunStart = postRunStart;
				_conditionStack.back().whilePostRunCount = funcCountAfter - postRunStart;
			}
		}

		// Run expression items
		for (auto itr = expression->list().begin(); itr != expression->list().end(); itr++)
			_addExpressionItem(*itr);

		if (addEndBlock)
		{
			// Single-line condition — pop immediately, no StartBlock/EndBlock will fire
			if (!_conditionStack.empty())
			{
				_conditionStack.pop_back();
				bool anyWhile = false;
				for (auto& e : _conditionStack)
					if (e.type == ConditionType::While) { anyWhile = true; break; }
				if (!anyWhile)
					_whileGeneratedVariables = 0;
			}
			_currentScript->addEndBlock(true);
		}
		else if (previousExpr && previousExpr->condition() && !previousExpr->condition()->isBlock())
		{
			if (!_conditionStack.empty())
			{
				_conditionStack.pop_back();
				bool anyWhile = false;
				for (auto& e : _conditionStack)
					if (e.type == ConditionType::While) { anyWhile = true; break; }
				if (!anyWhile)
					_whileGeneratedVariables = 0;
			}
		}

		return true;
	}
	// not a top level, so we can assign a generated variable
	else if (!topLevel)
		return true;
	else if (expression->list().front()->type() == ParseType::Function && dynamic_cast<const ParseFunction*>(expression->list().front())->returnVariable())
		_currentScript->addNewExpression(dynamic_cast<const ParseFunction*>(expression->list().front())->returnVariable());
	else if (expression->list().front()->type() == ParseType::Array)
	{
		failed = true;
		const ParseArray* arr = dynamic_cast<const ParseArray*>(expression->list().front());
		if (arr->assignment())
		{
			if (arr->assignment()->type() == ParseType::Variable)
			{
				_currentScript->addNewExpression(dynamic_cast<const ParseVariable*>(arr->assignment()));
				failed = false;
			}
		}
	}
	else
	{
		_addError(ParseErrors::MissingAssignment, expression);
		failed = true;
	}

	if (failed)
		return false;

	for (auto itr = expression->list().begin(); itr != expression->list().end(); itr++)
		_addExpressionItem(*itr);

	if (addEndBlock)
	{
		if (!_conditionStack.empty())
		{
			// If this is a single-line while body, emit the re-eval before the end block.
			if (_conditionStack.back().type == ConditionType::While)
				_emitWhileReEval();

			_conditionStack.pop_back();
			bool anyWhile = false;
			for (auto& e : _conditionStack)
				if (e.type == ConditionType::While) { anyWhile = true; break; }
			if (!anyWhile)
				_whileGeneratedVariables = 0;
		}
		_currentScript->addEndBlock(true);
	}

	return true;
}

void CScriptParser::_checkWarnings(const std::vector<const BaseParse*>& list)
{
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		if ((*itr)->type() == ParseType::Brackets)
			_checkWarnings(dynamic_cast<const ParseBrackets*>(*itr)->constList());
		else if ((*itr)->type() == ParseType::Expression)
			_checkWarnings(dynamic_cast<const ParseExpression*>(*itr)->list());
		else if ((*itr)->type() == ParseType::Function)
		{
			const ParseFunction* func = dynamic_cast<const ParseFunction*>(*itr);
			if (func->arguments())
			{
				for (unsigned int i = 0; i < func->arguments()->count(); i++)
					_checkWarnings({ func->arguments()->get(i) });
			}
		}
		else if ((*itr)->type() == ParseType::Variable)
		{
			const ParseVariable* vari = dynamic_cast<const ParseVariable*>(*itr);
			auto dts = vari->currentDataTypes();
			if (dts.empty()) dts = (*_pVariables)[vari->name()];
			if (dts.empty())
			{
				Warnings& warn = _addWarning(ParseWarnings::NullVariable, vari);
				warn.data.push_back(vari->data());
			}
		}
	}
}

bool CScriptParser::_runParse(const BaseParse* parse, const BaseParse* previous, const ParseCondition* condition, bool topLevel, InlineState inlineState)
{
	if (parse->type() == ParseType::Function)
	{
		ParseFunction* function = const_cast<ParseFunction*>(dynamic_cast<const ParseFunction*>(parse));
		if (!_runFunction(function, inlineState))
			return false;
	}
	else if (parse->type() == ParseType::Property)
	{
		ParseProperty* prop = const_cast<ParseProperty*>(dynamic_cast<const ParseProperty*>(parse));
		if (!_runProperty(prop, inlineState, false))
			return false;
	}
	else if (parse->type() == ParseType::Array)
	{
		ParseArray* arr = const_cast<ParseArray*>(dynamic_cast<const ParseArray*>(parse));
		if (!_runArrayFunction(arr, inlineState, false))
			return false;
	}
	else if (parse->type() == ParseType::Symbol)
	{
		const ParseSymbol* symb = dynamic_cast<const ParseSymbol*>(parse);
		if (symb->symbol() == SymbolType::StartBlock)
		{
			if (_prePassMode)
				_prePassDepth++;

			if (previous && previous->type() == ParseType::Function)
			{
				const ParseFunction* func = dynamic_cast<const ParseFunction*>(previous);
				if (func->condition() && !func->condition()->isBlock())
				{
					const ParseCondition* cond = dynamic_cast<const ParseCondition*>(func->condition());
					const_cast<ParseCondition*>(cond)->setBlock(true);
				}
				else
				{
					// Function has no condition — check if it created a runExpr that carries one
					// (e.g. inc/dec where RetVarType::Return doesn't match the condition)
					auto it = _createdExpressions.find(previous);
					if (it != _createdExpressions.end())
					{
						const ParseExpression* createdExpr = it->second;
						if (createdExpr->condition() && !createdExpr->condition()->isBlock())
							const_cast<ParseCondition*>(createdExpr->condition())->setBlock(true);
					}
				}
			}
			else if (previous && previous->type() == ParseType::Expression)
			{
				const ParseExpression* expr = dynamic_cast<const ParseExpression*>(previous);
				if (expr->condition() && !expr->condition()->isBlock())
				{
					const ParseCondition* cond = dynamic_cast<const ParseCondition*>(expr->condition());
					const_cast<ParseCondition*>(cond)->setBlock(true);
				}
			}
			else if (previous && previous->type() == ParseType::Array)
			{
				const ParseArray* arr = dynamic_cast<const ParseArray*>(previous);
				if (arr->assignment() && arr->assignment()->type() == ParseType::Condition && !dynamic_cast<const ParseCondition*>(arr->assignment())->isBlock())
				{
					const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arr->assignment());
					const_cast<ParseCondition*>(cond)->setBlock(true);
				}
			}
			else
			{
				if (condition && !condition->isBlock())
					const_cast<ParseCondition*>(condition)->setBlock(true);
			}
		}
		else if (symb->symbol() == SymbolType::EndBlock)
		{
			if (!_prePassMode)
			{
				// Before closing the block, check if the top of the condition stack
				// is a while — if so, emit the re-eval before the end command.
				if (!_conditionStack.empty() && _conditionStack.back().type == ConditionType::While)
					_emitWhileReEval();

				if (!_currentScript->addEndBlock(false))
				{
					ParseFail* fail = new ParseFail(parse, ParseErrors::MissingStartBrace);
					_errors.push_back(fail);
					return false;
				}

				// Pop the condition that this block closed
				if (!_conditionStack.empty())
				{
					_conditionStack.pop_back();
					// Reset the while counter only when no while blocks remain on the stack
					{
						bool anyWhile = false;
						for (auto& e : _conditionStack)
							if (e.type == ConditionType::While) { anyWhile = true; break; }
						if (!anyWhile)
							_whileGeneratedVariables = 0;
					}
				}
			}
			if (_prePassMode && _prePassDepth > 0)
				_prePassDepth--;
		}
	}
	else if (parse->type() == ParseType::Expression)
	{
		if (!_runExpressionList(dynamic_cast<const ParseExpression*>(parse), topLevel, nullptr, _isCondition(inlineState)))
			return false;
	}
	else if (parse->type() == ParseType::Brackets)
	{
		if (!_runDataList(dynamic_cast<const ParseBrackets*>(parse)->constList(), topLevel))
			return false;
	}

	return true;
}

bool CScriptParser::_runDataList(const std::vector<const BaseParse*>& list, bool topLevel)
{
	const BaseParse* previous = NULL;
	for (auto itr = list.begin(); itr != list.end(); itr++)
	{
		const BaseParse* parse = *itr;
		if (parse->type() == ParseType::Expression)
		{
			if (!_runExpressionList(dynamic_cast<const ParseExpression*>(parse), topLevel, dynamic_cast<const ParseExpression*>(previous), false))
				return false;
		}
		else if (parse->type() == ParseType::Brackets)
		{
			if (!_runDataList(dynamic_cast<const ParseBrackets*>(parse)->constList(), topLevel))
				return false;
		}
		else if (parse->type() == ParseType::Function)
		{
			// Direct function nodes in the list — from array increment desugaring
			ParseFunction* func = const_cast<ParseFunction*>(
				dynamic_cast<const ParseFunction*>(parse));
			if (!_runFunction(func, InlineState::Normal))
				return false;
		}
		else
		{
			if (!_runParse(*itr, previous, NULL, topLevel, !topLevel ? InlineState::Inline : InlineState::Normal))
				return false;
		}

		previous = *itr;
	}

	return true;
}

bool CScriptParser::_setArguments(const ParseArguments* arguments)
{
	if (arguments->count() != 3)
	{
		ParseFail* fail = new ParseFail(arguments, ParseErrors::InvalidArgumentCount);
		fail->addData(std::to_wstring(arguments->count()));
		fail->addData(std::to_wstring(3));
		_errors.push_back(fail);
		return false;
	}

	const ParDefData* pardef = NULL;
	if (arguments->get(2)->type() == ParseType::Constant)
	{
		const ParseConstant* c = static_cast<const ParseConstant*>(arguments->get(2));
		if (c->dataType() == DataTypes::ParDef)
			pardef = _data->getParDefData(static_cast<ParDef>(c->id()));
		else
		{
			_errorArgumentDatatype(arguments, 2, DataTypes::ParDef);
			return false;
		}
	}
	else
	{
		_errorArgumentDatatype(arguments, 2, DataTypes::ParDef);
		return false;
	}

	std::wstring var;
	if (arguments->get(0)->type() == ParseType::Variable)
	{
		const ParseVariable* v = static_cast<const ParseVariable*>(arguments->get(0));
		var = v->name();
	}
	else
	{
		_errorArgumentDatatype(arguments, 0, DataTypes::Variable);
		return false;
	}
	std::wstring desc;
	if (arguments->get(1)->type() == ParseType::String)
	{
		const ParseString* s = static_cast<const ParseString*>(arguments->get(1));
		desc = s->stringData();
	}
	else
	{
		_errorArgumentDatatype(arguments, 1, DataTypes::String);
		return false;
	}

	(*_pVariables)[var] = pardef->datatypes;
	if ((*_pVariables)[var].empty())
		(*_pVariables)[var].insert(DataTypes::Unknown);
	_currentScript->addArgument(var, desc, pardef->id, pardef->name);

	return true;
}

bool CScriptParser::_setDescription(const ParseArguments* arguments)
{
	if (arguments->count() != 1)
	{
		ParseFail* fail = new ParseFail(arguments, ParseErrors::InvalidArgumentCount);
		fail->addData(std::to_wstring(arguments->count()));
		fail->addData(std::to_wstring(1));
		_errors.push_back(fail);
		return false;
	}

	std::wstring desc;
	if (arguments->get(0)->type() == ParseType::String)
	{
		const ParseString* s = static_cast<const ParseString*>(arguments->get(0));
		desc = s->stringData();
	}
	else
	{
		_errorArgumentDatatype(arguments, 0, DataTypes::String);
		return false;
	}

	_currentScript->setDescription(desc);
	return true;
}

bool CScriptParser::_setVersion(const ParseArguments* arguments)
{
	if (arguments->count() != 1)
	{
		ParseFail* fail = new ParseFail(arguments, ParseErrors::InvalidArgumentCount);
		fail->addData(std::to_wstring(arguments->count()));
		fail->addData(std::to_wstring(1));
		_errors.push_back(fail);
		return false;
	}

	int version = 0;
	if (arguments->get(0)->type() == ParseType::Integer)
	{
		const ParseInteger* s = dynamic_cast<const ParseInteger*>(arguments->get(0));
		version = s->value();
	}
	else
	{
		_errorArgumentDatatype(arguments, 0, DataTypes::Number);
		return false;
	}

	_currentScript->setVersion(version);
	return true;
}

bool CScriptParser::_setCommand(const ParseArguments* arguments)
{
	if (arguments->count() != 1)
	{
		ParseFail* fail = new ParseFail(arguments, ParseErrors::InvalidArgumentCount);
		fail->addData(std::to_wstring(arguments->count()));
		fail->addData(std::to_wstring(1));
		_errors.push_back(fail);
		return false;
	}

	const ParseConstant* constant = NULL;
	if (arguments->get(0)->type() == ParseType::Constant)
	{
		constant = static_cast<const ParseConstant*>(arguments->get(0));
		if (constant->dataType() == DataTypes::ObjectCommand)
		{
			_currentScript->setCommand(constant->id());
			return true;
		}
	}
	else if (arguments->get(0)->type() == ParseType::Integer)
	{
		const ParseInteger* integer = dynamic_cast<const ParseInteger*>(arguments->get(0));
		_currentScript->setCommand(integer->value());
		return true;
	}

	if (constant)
		_errorArgumentDatatype(arguments, 0, DataTypes::ParDef);
	else
		_errorArgumentDatatype(arguments, 0, DataTypes::ObjectCommand);
	return false;
}

bool CScriptParser::_isInline(InlineState state) const
{
	return state == InlineState::Inline || state == InlineState::InlineCondition;
}
bool CScriptParser::_isCondition(InlineState state) const
{
	return state == InlineState::Condition || state == InlineState::InlineCondition;
}

DataTypes CScriptParser::_getDataTypeFromParse(const BaseParse* parse) const
{
	switch (parse->type())
	{
	case ParseType::String:
		return DataTypes::String;
	case ParseType::Variable:
		return DataTypes::Variable;
	case ParseType::Integer:
		return DataTypes::Number;
	}

	return parse->dataType();
}

std::wstring CScriptParser::_getPardefFlagString(ParDefFlags flags) const
{
	auto addString = [](std::wstringstream& str, std::wstring s, ParDefFlags flags, ParDefFlags check)
		{
			if ((flags & check) == check)
			{
				if (!str.str().empty())
					str << ", ";
				str << s;
			}
		};

	std::wstringstream str;
	addString(str, L"Constant", flags, ParDefFlags::Constant);
	addString(str, L"Variable", flags, ParDefFlags::Variable);
	addString(str, L"Integer", flags, ParDefFlags::Integer);
	addString(str, L"String", flags, ParDefFlags::String);
	addString(str, L"Object", flags, ParDefFlags::Object);

	return str.str();
}

bool CScriptParser::_isDataTypeForObject(DataTypes dt) const
{
	if ((static_cast<int>(dt) & _SCRIPT_DATATYPE_FLAG_OBJ))
		return true;

	switch (dt)
	{
	case DataTypes::Dealer:
	case DataTypes::Merchant:
		return true;
	}
	return false;
}

DataTypes CScriptParser::_getActualDataType(const BaseParse* parse) const
{
	DataTypes dt = parse->dataType();
	if (parse->type() == ParseType::Variable)
	{
		const ParseVariable* var = dynamic_cast<const ParseVariable*>(parse);
		auto findItr = _variables.find(var->name());
		if (findItr != _variables.end() && !findItr->second.empty())
			dt = *findItr->second.begin();
		else
			dt = var->currentDataType();
	}
	else if (parse->type() == ParseType::Constant)
	{
		const ParseConstant* constant = dynamic_cast<const ParseConstant*>(parse);
		dt = constant->subType();
	}
	else
		dt = parse->dataType();

	return dt;
}

std::unordered_set<DataTypes> CScriptParser::_getActualDataTypes(const BaseParse* parse) const
{
	if (parse->type() == ParseType::Variable)
	{
		const ParseVariable* var = dynamic_cast<const ParseVariable*>(parse);
		auto findItr = _variables.find(var->name());
		if (findItr != _variables.end())
			return findItr->second;

		std::unordered_set<DataTypes> dt;
		return dt;
	}
	else if (parse->type() == ParseType::Function)
	{
		const ParseFunction* func = dynamic_cast<const ParseFunction*>(parse);
		if (func->returnVariable())
		{
			auto findItr = _variables.find(func->returnVariable()->name());
			if (findItr != _variables.end())
				return findItr->second;
		}

		std::unordered_set<DataTypes> dt;
		return dt;
	}
	else if (parse->type() == ParseType::Brackets)
	{
		const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);
		auto single = brackets->singleItem();
		if (single)
			return _getActualDataTypes(single);
	}
	else if (parse->type() == ParseType::Expression)
	{
		const ParseExpression* expr = dynamic_cast<const ParseExpression*>(parse);
		if (expr->dataType() == DataTypes::Unknown && expr->returnValue())
			return _getActualDataTypes(expr->returnValue());
	}
	else if (parse->type() == ParseType::Constant)
	{
		const ParseConstant* constant = dynamic_cast<const ParseConstant*>(parse);

		std::unordered_set<DataTypes> dt;
		// Use the subtype — the actual game datatype the constant refers to
		// (e.g. PLAYERSHIP has type=Constant but subtype=DATATYPE_SHIP).
		// Fall back to dataType() only if subtype is Unknown.
		DataTypes resolved = constant->subType();
		if (resolved == DataTypes::Unknown)
			resolved = constant->dataType();
		dt.insert(resolved);
		return dt;
	}

	std::unordered_set<DataTypes> dt;
	dt.insert(parse->dataType());

	return dt;
}

bool CScriptParser::_doesObjectTypeMatch(DataTypes parent, DataTypes child) const
{
	if (parent == child)
		return true;
	if (parent == DataTypes::Object && _isDataTypeForObject(child))
		return true;
	return false;
}

std::wstring CScriptParser::_getDataTypesString(std::unordered_set<DataTypes> datatypes) const
{
	std::wstringstream str;
	for (auto itr = datatypes.begin(); itr != datatypes.end(); itr++)
	{
		if (!str.str().empty())
			str << ", ";
		str << _data->getDataTypeName(*itr);
	}

	return str.str();
}

std::wstring CScriptParser::_makeTempVarName()
{
	// Check if we're currently inside a while block on the condition stack.
	// If so, use a separate while-scoped counter so while temp vars don't
	// get reused by subsequent loop iterations — $VarGenWhile.N stays alive
	// for the whole while loop, whereas $VarGen.N is recycled after 10 uses.
	bool insideWhile = false;
	for (auto itr = _conditionStack.rbegin(); itr != _conditionStack.rend(); ++itr)
	{
		if (itr->type == ConditionType::While)
		{
			insideWhile = true;
			break;
		}
	}

	if (insideWhile)
	{
		_whileGeneratedVariables++;
		return L"$VarGenWhile." + std::to_wstring(_whileGeneratedVariables);
	}
	else
	{
		_generatedVariables++;
		return L"$VarGen." + std::to_wstring(_generatedVariables);
	}
}

ParseVariable* CScriptParser::_generateTempVariable(const BaseParse* source)
{
	std::unordered_set<DataTypes> types;
	types.insert(DataTypes::Unknown);
	ParseVariable* var = new ParseVariable(
		source->line(),
		_makeTempVarName(),
		&types);
	var->setFromParse(source);
	(*_pVariables)[var->name()] = types;
	return var;
}

bool CScriptParser::_isValidDataType(DataTypes from, std::unordered_set<DataTypes> _to) const
{
	if (_to.empty())
		return true;

	for (auto itr = _to.begin(); itr != _to.end(); itr++)
	{
		DataTypes to = *itr;

		if (from == to)
			return true;
		if (from == DataTypes::Object && _isDataTypeForObject(to))
			return true;
		if (to == DataTypes::Object && _isDataTypeForObject(from))
			return true;
		if (from == DataTypes::Unknown || to == DataTypes::Unknown)
			return true;
	}
	return false;
}

bool CScriptParser::_isValidDataType(std::unordered_set<DataTypes> _from, std::unordered_set<DataTypes> _to) const
{
	if (_to.empty() || _from.empty())
		return true;

	for (auto fItr = _from.begin(); fItr != _from.end(); fItr++)
	{
		DataTypes from = *fItr;
		for (auto itr = _to.begin(); itr != _to.end(); itr++)
		{
			DataTypes to = *itr;

			if (from == to)
				return true;
			if (to == DataTypes::Unknown)
				return true;
			if (from == DataTypes::Unknown)
				return true;
			if (from == DataTypes::Object && _isDataTypeForObject(to))
				return true;
			if (to == DataTypes::Object && _isDataTypeForObject(from))
				return true;
		}
	}
	return false;
}


void CScriptParser::_errorArgumentDatatype(const ParseArguments* arguments, unsigned int i, DataTypes wanted)
{
	ParseFail* fail = new ParseFail(arguments->get(i), ParseErrors::InvalidArgumentDataType);
	fail->addData(_formatString(arguments->get(i)->dataType(), arguments->get(i)->data()));
	fail->addData(std::to_wstring(i));
	DataTypes got = _getDataTypeFromParse(arguments->get(i));
	if (got == DataTypes::Unknown)
		got = DataTypes::Invalid;
	fail->addData(std::to_wstring(static_cast<int>(got)));
	fail->addData(std::to_wstring(static_cast<int>(wanted)));
	_errors.push_back(fail);
}

ParseFail* CScriptParser::_addError(ParseErrors error, const BaseParse* parse)
{
	ParseFail* fail = new ParseFail(parse, error);
	_errors.push_back(fail);

	// add any data
	fail->addData(parse->data());

	return fail;
}

Warnings& CScriptParser::_addWarning(ParseWarnings type, const BaseParse* parse)
{
	// During pre-pass, suppress all warnings — they will be re-evaluated
	// and reported correctly during the real compile pass.
	static Warnings dummy{};
	if (_prePassMode)
		return dummy;

	// Check for duplicate warning — same type, same variable name, same line
	for (auto& existing : _warnings)
	{
		if (existing.warning == type &&
			existing.linePos == parse->linePos() &&
			existing.line == parse->line())
		{
			// For warnings that include a variable/object name in data[0],
			// also match on that to avoid suppressing different variables
			// that happen to be on the same line
			if (!existing.data.empty() && !parse->data().empty())
			{
				if (existing.data[0] == parse->data())
					return existing; // duplicate — return existing, don't add
			}
			else if (existing.data.empty() && parse->data().empty())
				return existing;
		}
	}

	_warnings.push_back({ type, parse->startingPos(), parse->endingPos(), parse->linePos(), parse->line(), parse->file() });
	return _warnings.back();
}

std::wstring CScriptParser::_formatString(DataTypes dt, const std::wstring& str)
{
	if (dt == DataTypes::String)
	{
		std::wstringstream strm;
		strm << "\"" << str << "\"";
		return strm.str();
	}
	return str;
}