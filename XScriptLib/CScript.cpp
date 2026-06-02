#include "pch.h"
#include "CScript.h"

#include "BaseParse.h"
#include "ParseFunction.h"
#include "ParseCondition.h"
#include "ParseBrackets.h"
#include "ParseSymbol.h"
#include "ParseOperator.h"
#include "ParseExpression.h"
#include "ParseArray.h"
#include "ParseVariable.h"
#include "ParseKeyword.h"
#include "ParseProperty.h"
#include "ParseLabel.h"
#include "ParseArguments.h"
#include "CScriptData.h"

#include <stack>
#include <algorithm>
#include <fstream>
#include <cassert>

using namespace XScript;

void ScriptFunction::addArgument(const BaseParse* parse, ParDef pardef)
{
    _arguments.push_back(parse);
    if (pardef != ParDef::Unknown)
        const_cast<BaseParse*>(parse)->setParDef(pardef);
}


CScript::CScript(const CScriptData* data) :
    _pScriptData(data),
    _version(0),
    _command(0)
{
}

CScript::~CScript()
{
    for (auto ptr : _createdItems)
        delete ptr;
    _createdItems.clear();
}


void CScript::addArgument(const std::wstring& variable, const std::wstring& description, ParDef parameterDefinition, const std::wstring& parDefName)
{
    _arguments.push_back(ScriptArguments({ variable, description, parameterDefinition, parDefName }));
    addVariable(variable);
}

void CScript::setDescription(const std::wstring& desc)
{
    _description = desc;
}

void CScript::setVersion(unsigned int version)
{
    _version = version;
}

void CScript::setCommand(unsigned int command)
{
    _command = command;
}

void CScript::addVariable(const std::wstring& variable)
{
    if (variable.empty())
        return;

    if (_variablesLookup.find(variable) == _variablesLookup.end())
    {
        _variablesLookup[variable] = static_cast<unsigned int>(_variables.size());

        std::wstring v = variable;
        if (!v.empty() && v[0] == L'$')
            v = v.erase(0, 1);
        _variables.push_back(v);
    }
}

void CScript::addNewExpression(const ParseCondition* cond)
{
    if (_pendingEnd)
        _addEndBlock(_forceEnd);

    _functions.push_back({ _pScriptData->expressionCommand(), nullptr });
    _lastAddedIndex = static_cast<int>(_functions.size() - 1);
    _lastAddedIsPost = false;
    flushPostRun();
    lastFunc()->retvarID = static_cast<int>(lastFunc()->argumentCount());
    lastFunc()->addArgument(cond, ParDef::Unknown);
}

void CScript::addNewExpression(const ParseVariable* vari)
{
    if (_pendingEnd)
        _addEndBlock(_forceEnd);

    _functions.push_back({ _pScriptData->expressionCommand(), nullptr });
    _lastAddedIndex = static_cast<int>(_functions.size() - 1);
    _lastAddedIsPost = false;
    flushPostRun();
    lastFunc()->retvarID = static_cast<int>(lastFunc()->argumentCount());
    addVariable(vari->name());
    lastFunc()->addArgument(vari, ParDef::Var);
}

void CScript::addFunction(unsigned int id, const ParseFunction* func, bool postRun, bool suppressFlush)
{
    if (_pendingEnd)
        _addEndBlock(_forceEnd);

    if (postRun)
    {
        // Store pending � don't add to _functions yet so _functions.back()
        // remains the current normal function for addRetVar/addFunctionArgument
        _pendingPostRun.push_back(ScriptFunction(id, func));
        _lastAddedIndex = static_cast<int>(_pendingPostRun.size() - 1);
        _lastAddedIsPost = true;
        lastFunc()->isPost = true;
        return;
    }

    _functions.push_back({ id, func });
    _lastAddedIndex = static_cast<int>(_functions.size() - 1);
    _lastAddedIsPost = false;

    if (!suppressFlush)
        flushPostRun();
}

void CScript::flushPostRun()
{
    for (auto& func : _pendingPostRun)
        _functions.push_back(func);
    if (_lastAddedIsPost)
        _lastAddedIndex = static_cast<int>(_functions.size() - 1);
    _lastAddedIsPost = false;
    _pendingPostRun.clear();
}


void CScript::setFunctionUndefinedCount(unsigned int count)
{
    if (!lastFunc())
        return;
    lastFunc()->undefinedArgs = count;
    lastFunc()->undefinedStart = static_cast<unsigned int>(lastFunc()->argumentCount());
}

void CScript::addRetVar(const BaseParse* arg)
{
    if (!lastFunc())
        return;
    lastFunc()->retvarID = static_cast<int>(lastFunc()->argumentCount());
    addFunctionArgument(arg, ParDef::Unknown);
}

void CScript::addFunctionArgument(const BaseParse* arg, ParDef pardef)
{
    if (!lastFunc())
        return;
    _addVariables(arg);
    if (arg->type() == ParseType::Function)
    {
        const ParseFunction* func = dynamic_cast<const ParseFunction*>(arg);
        if (func->returnVariable())
        {
            lastFunc()->addArgument(func->returnVariable(), pardef);
            return;
        }
    }
    else if (arg->type() == ParseType::Array)
    {
        const ParseArray* arr = dynamic_cast<const ParseArray*>(arg);
        if (arr->assignment() && arr->assignment()->type() == ParseType::Variable)
        {
            lastFunc()->addArgument(arr->assignment(), pardef);
            return;
        }
        else if (arr->assign())
        {
            addFunctionArgument(arr->assign(), pardef);
            return;
        }
    }
    else if (arg->type() == ParseType::Property)
    {
        const ParseProperty* prop = dynamic_cast<const ParseProperty*>(arg);
        if (prop->getterFunction())
        {
            addFunctionArgument(prop->getterFunction(), pardef);
            return;
        }
        else if (prop->setterFunction())
        {
            const ParseFunction* func = dynamic_cast<const ParseFunction*>(
                prop->setterFunction());
            if (func->arguments()->count() > 0)
            {
                addFunctionArgument(func->arguments()->get(0), pardef);
                return;
            }
        }
    }
    else if (arg->type() == ParseType::Expression)
    {
        const ParseExpression* expr = dynamic_cast<const ParseExpression*>(arg);
        if (expr->assignment())
        {
            lastFunc()->addArgument(expr->assignment(), pardef);
            return;
        }
    }
    lastFunc()->addArgument(arg, pardef);
}

void CScript::addFunctionCondition(const ParseCondition* c)
{
    if (!lastFunc())
        return;
    lastFunc()->retvarID = static_cast<int>(lastFunc()->argumentCount());
    lastFunc()->addArgument(c, ParDef::Unknown);
}

bool CScript::_addEndBlock(bool forceBlock)
{
    _pendingEnd = false;
    _forceEnd = false;

    if (!lastFunc())
        return false;
    int count = 0;
    for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++)
    {
        if (itr->endBlock == -1)
        {
            if (itr->id == _pScriptData->elseCommand())
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(
                    itr->firstArg());
                if (cond->condition() != Conditions::None && (cond->isBlock() || forceBlock))
                {
                    flushPostRun(); // flush before end so post-run appears inside the block
                    if (forceBlock && !cond->isBlock())
                        const_cast<ParseCondition*>(cond)->setBlock(true);
                    const_cast<ParseCondition*>(cond)->setBlockCount(count);
                    itr->endBlock = static_cast<int>(_functions.size());
                    _functions.push_back({ _pScriptData->endCommand(), nullptr });
                    _lastAddedIndex = static_cast<int>(_functions.size() - 1);
                    _lastAddedIsPost = false;
                    return true;
                }
            }
            else if (itr->retvarID >= 0)
            {
                const BaseParse* parse = itr->retvarArgument();
                if (parse && parse->type() == ParseType::Condition)
                {
                    const ParseCondition* cond = dynamic_cast<const ParseCondition*>(
                        parse);
                    if (cond->condition() != Conditions::None && (cond->isBlock() || forceBlock))
                    {
                        flushPostRun(); // flush before end so post-run appears inside the block
                        if (forceBlock && !cond->isBlock())
                            const_cast<ParseCondition*>(cond)->setBlock(true);
                        const_cast<ParseCondition*>(cond)->setBlockCount(count);
                        itr->endBlock = static_cast<int>(_functions.size());
                        _functions.push_back({ _pScriptData->endCommand(), nullptr });
                        _lastAddedIndex = static_cast<int>(_functions.size() - 1);
                        _lastAddedIsPost = false;
                        return true;
                    }
                }
            }
        }
        ++count;
    }
    return false;
}

void CScript::_addVariables(const BaseParse* arg)
{
    if (arg->type() == ParseType::Variable)
    {
        const ParseVariable* var = dynamic_cast<const ParseVariable*>(arg);
        addVariable(var->name());
    }
    else if (arg->type() == ParseType::Function)
    {
        const ParseFunction* func = dynamic_cast<const ParseFunction*>(arg);
        if (func->returnVariable())
            addVariable(func->returnVariable()->name());
    }
    else if (arg->type() == ParseType::Brackets)
    {
        const ParseBrackets* bracket = dynamic_cast<const ParseBrackets*>(arg);
        for (auto itr = bracket->list().begin(); itr != bracket->list().end(); itr++)
            _addVariables(*itr);
    }
    else if (arg->type() == ParseType::Expression)
    {
        const ParseExpression* expression = dynamic_cast<const ParseExpression*>(arg);
        for (auto itr = expression->list().begin(); itr != expression->list().end(); itr++)
            _addVariables(*itr);
    }
    else if (arg->type() == ParseType::Property)
    {
        const ParseProperty* prop = dynamic_cast<const ParseProperty*>(arg);
        if (prop->setter())
            _addVariables(prop->setter());
        if (prop->getter())
            _addVariables(prop->getter());
    }

}

bool CScript::addEndBlock(bool forceBlock)
{
    if (!lastFunc())
        return false;
    int count = 0;
    for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++)
    {
        if (itr->endBlock == -1)
        {
            if (itr->id == _pScriptData->elseCommand())
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(
                    itr->firstArg());
                if (cond->condition() != Conditions::None && (cond->isBlock() || forceBlock))
                {
                    flushPostRun(); // flush before end so post-run appears inside the block
                    _forceEnd = forceBlock;
                    _pendingEnd = true;
                    return true;
                }
            }
            else if (itr->retvarID >= 0)
            {
                const BaseParse* parse = itr->retvarArgument();
                if (parse && parse->type() == ParseType::Condition)
                {
                    const ParseCondition* cond = dynamic_cast<const ParseCondition*>(
                        parse);
                    if (cond->condition() != Conditions::None && (cond->isBlock() || forceBlock))
                    {
                        flushPostRun(); // flush before end so post-run appears inside the block
                        _forceEnd = forceBlock;
                        _pendingEnd = true;
                        return true;
                    }
                }
            }
        }
        ++count;
    }
    return false;
}

bool CScript::addLabel(const ParseKeyword* parse)
{
    if (_pendingEnd)
        _addEndBlock(_forceEnd);

    auto findItr = _labels.find(parse->keyword());
    if (findItr != _labels.end())
        return false;
    _labels[parse->keyword()] = _functions.size();
    _functions.push_back({ _pScriptData->defineLabelCommand(), nullptr });
    _lastAddedIndex = static_cast<int>(_functions.size() - 1);
    _lastAddedIsPost = false;
    flushPostRun();
    lastFunc()->addArgument(parse, ParDef::Label);
    return true;
}
bool CScript::isInWhile() const
{
    // Use a stack to track what each open block is (while or non-while).
    // Only pop the while counter when an endCommand closes a while block,
    // not when it closes an if/else block.
    std::vector<bool> blockStack; // true = while, false = if/else

    for (auto itr = _functions.begin(); itr != _functions.end(); itr++)
    {
        if (itr->retvarID >= 0)
        {
            auto arg = itr->retvarArgument();
            if (arg && arg->type() == ParseType::Condition)
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                if (cond->isBlock())
                {
                    bool isWhile = (cond->condition() == Conditions::While ||
                        cond->condition() == Conditions::WhileNot);
                    blockStack.push_back(isWhile);
                }
            }
        }
        if (itr->id == _pScriptData->endCommand())
        {
            if (!blockStack.empty())
                blockStack.pop_back();
        }
    }

    // We're in a while if any entry in the current block stack is a while
    for (bool isWhile : blockStack)
        if (isWhile) return true;

    return false;
}
bool CScript::isIfOpen() const
{
    for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++)
    {
        if (itr->retvarID >= 0)
        {
            auto arg = itr->retvarArgument();
            if (arg && arg->type() == ParseType::Condition)
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                if (cond->condition() == Conditions::If || cond->condition() == Conditions::IfNot || cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot || cond->condition() == Conditions::Else)
                {
                    if (!cond->isBlock())
                        return true;
                    if (itr->endBlock == -1)
                        return true;
                    if ((itr->endBlock + 1) == _functions.size())
                        return true;
                }
            }
        }
    }

    return false;
}

const ScriptFunction* CScript::previousFunction()
{
    return lastFunc();
}

const std::vector<ScriptFunction>& CScript::functions() const
{
    return _functions;
}

size_t CScript::functionCount() const
{
    return _functions.size();
}

void CScript::duplicateFunction(size_t index)
{
    if (index < _functions.size())
        _functions.push_back(_functions[index]);
}

void CScript::insertFunction(unsigned int id, const ParseFunction* func)
{
    // Find the position of the opening 'if' that starts this if/else-if chain.
    // Walk backwards past any end blocks and conditions to find the first if
    // at depth 0 — insert the new function just before it.
    int insertPos = static_cast<int>(_functions.size()); // fallback: append
    int depth = 0;

    for (int i = static_cast<int>(_functions.size()) - 1; i >= 0; i--)
    {
        if (_functions[i].id == _pScriptData->endCommand())
        {
            depth++;
        }
        else if (_functions[i].retvarID >= 0)
        {
            const BaseParse* arg = _functions[i].retvarArgument();
            if (arg && arg->type() == ParseType::Condition)
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                if (cond->condition() == Conditions::None)
                    continue;

                if (depth == 0)
                {
                    insertPos = i;
                    // Keep walking — if this is else/else-if, the opening if
                    // is further back. Insert must go before the whole chain.
                    Conditions c = cond->condition();
                    if (c != Conditions::ElseIf && c != Conditions::ElseIfNot &&
                        c != Conditions::Else)
                        break; // found the opening if — done
                    // else: continue searching (don't decrement depth at 0)
                }
                else
                    depth--;
            }
        }
    }

    ScriptFunction sf(id, func);
    _functions.insert(_functions.begin() + insertPos, sf);
    _lastAddedIndex = insertPos;
    _lastInsertPos = insertPos; // remember for insertNewExpression
    _lastAddedIsPost = false;
}

void CScript::insertNewExpression(const ParseVariable* vari)
{
    // Same insert-position logic as insertFunction — find the opening if block
    int insertPos = static_cast<int>(_functions.size());
    int depth = 0;

    for (int i = static_cast<int>(_functions.size()) - 1; i >= 0; i--)
    {
        if (_functions[i].id == _pScriptData->endCommand())
        {
            depth++;
        }
        else if (_functions[i].retvarID >= 0)
        {
            const BaseParse* arg = _functions[i].retvarArgument();
            if (arg && arg->type() == ParseType::Condition)
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                if (cond->condition() == Conditions::None)
                    continue;
                if (depth == 0)
                {
                    insertPos = i;
                    // Keep walking — if this is else/else-if, the opening if
                    // is further back. Insert must go before the whole chain.
                    Conditions c = cond->condition();
                    if (c != Conditions::ElseIf && c != Conditions::ElseIfNot &&
                        c != Conditions::Else)
                        break;
                    // else: continue searching
                }
                else
                    depth--;
            }
        }
    }

    // The expressionCommand must come BEFORE the function call that was just
    // inserted by insertFunction. Use _lastInsertPos which points to where
    // insertFunction placed the function — insert the expressionCommand there,
    // pushing the function one position forward.
    ScriptFunction sf(_pScriptData->expressionCommand(), nullptr);
    int exprInsertPos = (_lastInsertPos >= 0 && _lastInsertPos < static_cast<int>(_functions.size()))
        ? _lastInsertPos : insertPos;
    _functions.insert(_functions.begin() + exprInsertPos, sf);
    _lastAddedIndex = exprInsertPos;
    _lastInsertPos = -1; // reset
    _lastAddedIsPost = false;

    // Configure the inserted entry
    ScriptFunction& inserted = _functions[exprInsertPos];
    inserted.retvarID = static_cast<int>(inserted.argumentCount());
    addVariable(vari->name());
    inserted.addArgument(vari, ParDef::Var);
}

bool CScript::isLabelValid(const std::wstring& label) const
{
    auto findItr = _labels.find(label);
    return (findItr != _labels.end());
}

bool CScript::finalise()
{
    // Flush any pending end block that wasn't consumed by else/else-if
    if (_pendingEnd)
        _addEndBlock(_forceEnd);

    bool doneAny = false;
    do
    {
        doneAny = false;
        bool hasElse = false;
        int debug = 0;
        for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++, debug++)
        {
            if (itr->id == _pScriptData->elseCommand())
            {
                hasElse = true;

                if (itr->endBlock == -1 && itr->argumentCount())
                {
                    const ParseCondition* cond = dynamic_cast<const ParseCondition*>(itr->firstArg());
                    if (!cond->isBlock())
                    {
                        const_cast<ParseCondition*>(cond)->setBlock(true);
                        const_cast<ParseCondition*>(cond)->setBlockCount(1);
                        itr->endBlock = static_cast<int>(_functions.size() - std::distance(_functions.rbegin(), itr));
                        _functions.insert(_functions.begin() + itr->endBlock + 1, { _pScriptData->endCommand(), nullptr });
                        doneAny = true;
                    }
                    break;
                }
            }
            else if (itr->id == _pScriptData->endCommand() && hasElse)
                hasElse = false;
            if (itr->retvarID >= 0)
            {
                auto arg = itr->retvarArgument();
                if (arg && arg->type() == ParseType::Condition)
                {
                    const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                    if (cond->condition() == Conditions::If || cond->condition() == Conditions::IfNot)
                    {
                        if (hasElse && !cond->isBlock())
                        {
                            const_cast<ParseCondition*>(cond)->setBlock(true);
                            const_cast<ParseCondition*>(cond)->setBlockCount(1);
                        }
                        hasElse = false;
                    }
                    else if (cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot || cond->condition() == Conditions::Else)
                    {
                        if (!hasElse && itr->endBlock == -1 && !cond->isBlock())
                        {
                            const_cast<ParseCondition*>(cond)->setBlock(true);
                            const_cast<ParseCondition*>(cond)->setBlockCount(1);
                            itr->endBlock = static_cast<int>(_functions.size() - std::distance(_functions.rbegin(), itr));
                            _functions.insert(_functions.begin() + itr->endBlock + 1, { _pScriptData->endCommand(), nullptr });
                            doneAny = true;
                            break;
                        }
                        else if (hasElse && !cond->isBlock())
                        {
                            const_cast<ParseCondition*>(cond)->setBlock(true);
                            const_cast<ParseCondition*>(cond)->setBlockCount(1);
                        }

                        hasElse = true;
                    }
                    else if (cond->condition() == Conditions::While || cond->condition() == Conditions::WhileNot)
                    {
                        if (itr->endBlock == -1 && !cond->isBlock())
                        {
                            const_cast<ParseCondition*>(cond)->setBlock(true);
                            const_cast<ParseCondition*>(cond)->setBlockCount(1);
                            itr->endBlock = static_cast<int>(_functions.size() - std::distance(_functions.rbegin(), itr));
                            _functions.insert(_functions.begin() + itr->endBlock + 1, { _pScriptData->endCommand(), nullptr });
                            doneAny = true;
                            break;
                        }
                    }
                }
            }
        }
    } while (doneAny);

    // remove "end" where there is an else
    bool previousIsElse = false;
    std::vector<size_t> remove;
    for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++)
    {
        if (previousIsElse)
        {
            if (itr->id == _pScriptData->endCommand())
                remove.push_back(_functions.size() - std::distance(_functions.rbegin(), itr) - 1);
        }
        previousIsElse = false;
        if (itr->id == _pScriptData->elseCommand())
            previousIsElse = true;
        else if (itr->retvarID >= 0)
        {
            auto arg = itr->retvarArgument();
            if (arg && arg->type() == ParseType::Condition)
            {
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                if (cond->condition() == Conditions::Else || cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot)
                    previousIsElse = true;
            }
        }
    }

    for (auto itr = remove.begin(); itr != remove.end(); itr++)
        _functions.erase(_functions.begin() + *itr);

    //insert hiddengoto before each else, and at end of a while
    {
        size_t start = 0;
        std::stack<size_t> inWhile;
        auto itr = _functions.begin();
        while (itr != _functions.end())
        {
            if (itr->retvarID >= 0)
            {
                auto arg = itr->retvarArgument();
                if (arg && arg->type() == ParseType::Condition)
                {
                    const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                    if (cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot)
                    {
                        itr = _functions.insert(itr, { _pScriptData->hiddenGotoCommand(), nullptr, -1, -1, itr->endLine });
                        start++;
                        ++itr;
                    }
                    else if (cond->condition() == Conditions::While || cond->condition() == Conditions::WhileNot)
                        inWhile.push(start);
                }
            }
            if (!inWhile.empty() && itr->id == _pScriptData->endCommand())
            {
                itr = _functions.insert(itr, { _pScriptData->hiddenGotoCommand(), nullptr, -1, -1, static_cast<size_t>(inWhile.top()), static_cast<unsigned int>(start++) });
                ++itr;
                inWhile.pop();
            }

            if (itr->id != _pScriptData->endCommand())
                ++start;
            ++itr;
        }
    }

    // fill the end line position for each block
    size_t lines = 0;
    for (auto itr = _functions.begin(); itr != _functions.end(); itr++)
    {
        if (itr->id != _pScriptData->endCommand())
            ++lines;
    }

    std::vector<size_t> endPosition;
    std::vector<size_t> gotoPosition;
    for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++)
    {
        itr->startLine = lines;
        if (itr->id == _pScriptData->endCommand())
        {
            endPosition.push_back(lines);
            gotoPosition.push_back(lines);
        }
        else if (itr->id == _pScriptData->elseCommand())
        {
            itr->endLine = gotoPosition.back();
            gotoPosition[gotoPosition.size() - 1] = lines;
        }
        else if (itr->id == _pScriptData->defineLabelCommand())
        {
            if (itr->firstArg() && itr->firstArg()->type() == ParseType::Keyword)
            {
                auto keyword = dynamic_cast<const ParseKeyword*>(itr->firstArg());
                _labels[keyword->keyword()] = itr->startLine;
            }
        }
        else if (itr->id == _pScriptData->breakCommand())
            itr->endLine = gotoPosition.back();
        else if (itr->id == _pScriptData->hiddenGotoCommand() && itr->endLine == -1)
            itr->endLine = endPosition.back();
        else if (itr->retvarID >= 0 && itr->endBlock != -1)
        {
            auto arg = itr->retvarArgument();
            if (arg && arg->type() == ParseType::Condition)
            {
                itr->endLine = gotoPosition.back();
                const ParseCondition* cond = dynamic_cast<const ParseCondition*>(arg);
                if (cond->condition() == Conditions::If || cond->condition() == Conditions::IfNot || cond->condition() == Conditions::While || cond->condition() == Conditions::WhileNot)
                {
                    gotoPosition.pop_back();
                    endPosition.pop_back();
                }
                else if (cond->condition() == Conditions::Else)
                    gotoPosition[gotoPosition.size() - 1] = lines;
                else if (cond->condition() == Conditions::ElseIf || cond->condition() == Conditions::ElseIfNot)
                    gotoPosition[gotoPosition.size() - 1] = lines - 1;
            }
        }

        if (itr->id != _pScriptData->endCommand())
            --lines;
    }

    // simplify expression by removing brackets where theres only 1 item    
    /*
    for (auto itr = _functions.rbegin(); itr != _functions.rend(); itr++)
    {
        if (itr->id == _pScriptData->expressionCommand())
        {
            std::vector<const BaseParse*> newArgs;
            for (auto aItr = itr->arguments().begin(); aItr != itr->arguments().end(); aItr++)
                _simplifyExpression(*aItr, newArgs);

            itr->clearArguments();
            for (auto aItr = newArgs.begin(); aItr != newArgs.end(); aItr++)
                itr->addArgument(*aItr, ParDef::Unknown);
        }
    }
    */

    return true;
}

void CScript::writeArguments(std::wofstream& out, const ScriptFunction& func, const BaseParse* parse, bool isRetvar) const
{
    bool hasDatatype = true;
    if (parse->pardef() == ParDef::CallName)
        hasDatatype = false;

    // special handling for variables, as we need to find the variable id from the lookup table
    if (parse->type() == ParseType::Variable)
    {
        const ParseVariable* arg = dynamic_cast<const ParseVariable*>(parse);

        // if its the return value, then we dont add the datatype, as its always a variable
        if (isRetvar)
            hasDatatype = false;

        if (hasDatatype)
            out << L"      <sval type=\"int\" val=\"" << static_cast<int>(arg->dataType()) << L"\"/>" << std::endl;

        auto find_itr = _variablesLookup.find(arg->stringData());
        if (find_itr != _variablesLookup.end())
            out << L"      <sval type=\"int\" val=\"" << find_itr->second << L"\"/>" << std::endl;
        else
            out << L"      <sval type=\"int\" val=\"" << 0 << L"\"/>" << std::endl;
    }
    else if (parse->type() == ParseType::Condition)
    {
        const ParseCondition* cond = dynamic_cast<const ParseCondition*>(parse);
        int condId = 0;
        auto currentCondition = cond->condition();

        if (!cond->isBlock())
        {
            switch (currentCondition)
            {
            case Conditions::If:
                currentCondition = Conditions::SkipIfNot;
                break;
            case Conditions::IfNot:
                currentCondition = Conditions::SkipIf;
                break;
            default:
                break;
            }
        }

        switch (currentCondition)
        {
        case Conditions::If:
            condId = SCRIPT_VARIDX_IF;
            break;
        case Conditions::IfNot:
            condId = SCRIPT_VARIDX_IFNOT;
            break;
        case Conditions::SkipIf:
            condId = SCRIPT_VARIDX_SKIPIF;
            break;
        case Conditions::SkipIfNot:
            condId = SCRIPT_VARIDX_SKIPIFNOT;
            break;
        case Conditions::ElseIf:
            condId = SCRIPT_VARIDX_ELSEIF;
            break;
        case Conditions::ElseIfNot:
            condId = SCRIPT_VARIDX_ELSEIFNOT;
            break;
        case Conditions::While:
            condId = SCRIPT_VARIDX_WHILE;
            break;
        case Conditions::WhileNot:
            condId = SCRIPT_VARIDX_WHILENOT;
            break;
        case Conditions::None:
            condId = SCRIPT_VARIDX_NORETVAR;
            break;
        case Conditions::Start:
            condId = SCRIPT_VARIDX_START;
            break;
        default:
            break;
        }

        // add the end position
        if (func.endLine > 0)
            condId |= (func.endLine << _SCRIPT_VARIDX_SHIFT_JUMPIDX) & _SCRIPT_VARIDX_MASK_JUMPIDX;

        out << L"      <sval type=\"int\" val=\"" << condId << L"\"/>" << std::endl;
    }
    // for labels, we dont need any datatype, just the label name string
    else if (func.id == _pScriptData->defineLabelCommand())
        out << L"      <sval type=\"string\" val=\"" << parse->stringData() << L"\"/>" << std::endl;
    else
    {
        if (hasDatatype)
            out << L"      <sval type=\"int\" val=\"" << static_cast<int>(parse->dataType()) << L"\"/>" << std::endl;
        if (_isDatatypeString(parse->dataType()))
            out << L"      <sval type=\"string\" val=\"" << parse->stringData() << L"\"/>" << std::endl;
        else
            out << L"      <sval type=\"int\" val=\"" << parse->stringData() << L"\"/>" << std::endl;
    }
}


bool CScript::save(const std::wstring& file, const std::vector<Function>& functionData)
{
    std::wofstream out;
    out.open(file);

    if (!out.is_open())
        return false;

    out << L"<?xml version=\"1.0\" standalone=\"yes\" ?>" << std::endl;
    out << L"<?xml-stylesheet href=\"x2script.xsl\" type=\"text/xsl\" ?>" << std::endl;
    out << L"<script>" << std::endl;

    std::wstring name = file;
    size_t pos = name.find_last_of(L'\\');
    if (pos != std::wstring::npos)
        name = name.substr(pos + 1);
    else
    {
        pos = name.find_last_of(L'/');
        if (pos != std::wstring::npos)
            name = name.substr(pos + 1);
    }
    pos = name.find_last_of(L'.');
    if (pos != std::wstring::npos)
        name = name.substr(0, pos);

    out << L"<name>" << name << L"</name>" << std::endl;
    out << L"<version>" << _version << L"</version>" << std::endl;
    out << L"<engineversion>" << _pScriptData->gameData().engineMax << L"</engineversion>" << std::endl;
    out << L"<description>" << _description << L"</description>" << std::endl;
    out << L"<arguments>" << std::endl;
    for (size_t i = 0; i < _arguments.size(); ++i)
        out << L"<argument index=\"" << i + 1 << L"\" name=\"" << _arguments[i].variable << L"\" type=\"" << _arguments[i].parDefName << L"\" desc=\"" << _arguments[i].description << L"\"/>" << std::endl;
    out << L"</arguments>" << std::endl;

    // add the plain text as empty fields
    out << L"<sourceplaintext>" << std::endl;
    out << L"</sourceplaintext>" << std::endl;
    out << L"<sourcetext>" << std::endl;
    out << L"</sourcetext>" << std::endl;

    // now do the code array
    out << L"<codearray>" << std::endl;
    out << L"<sval type=\"array\" size=\"10\">" << std::endl;

    // script parameters
    out << L"  <sval type=\"string\" val=\"" << name << L"\"/>" << std::endl;
    out << L"  <sval type=\"int\" val=\"" << _pScriptData->gameData().engineMax << L"\"/>" << std::endl;
    out << L"  <sval type=\"string\" val=\"" << _description << L"\"/>" << std::endl;
    out << L"  <sval type=\"int\" val=\"" << _version << L"\"/>" << std::endl;
    out << L"  <sval type=\"int\" val=\"0\"/>" << std::endl;

    // script variables
    if (_variables.empty())
        out << L"  <sval type=\"int\" val=\"0\"/>" << std::endl;
    else
    {
        out << L"  <sval type=\"array\" size=\"" << _variables.size() << L"\">" << std::endl;
        for (auto const& v : _variables)
            out << L"    <sval type=\"string\" val=\"" << v << L"\"/>" << std::endl;
        out << L"  </sval>" << std::endl;
    }

    // Calculate once whether we need to append a final return null.
    // Must be determined BEFORE cmdCount is written to the XML — changing it
    // afterwards would cause a size/entry mismatch.
    // Suppress the auto-return only when the last entry is a returnCommand AND
    // it is not inside a conditional (with or without a block).
    // Cases where the auto-return IS still needed:
    //   [..., conditional, return]    — single-line condition (no end block)
    //   [..., return, end]            — return inside a block
    const bool shouldIncludeReturn = [&]() -> bool
        {
            if (_functions.empty()) return true;

            // If the last entry is not a return, we always need the auto-return
            if (_functions.back().id != _pScriptData->returnCommand()) return true;

            // Last entry is a return — check if it's inside a conditional.
            // Case 1: [..., end] — return before end means it was inside a block
            // (handled: back() would be end, not return, so we'd have returned true above)

            // Case 2: [..., conditional, return] — single-line condition, no end block.
            // Check the second-to-last entry for a function with a condition retvar.
            if (_functions.size() >= 2)
            {
                const auto& prev = _functions[_functions.size() - 2];
                if (prev.retvarID >= 0)
                {
                    const auto* arg = prev.retvarArgument();
                    if (arg && arg->type() == ParseType::Condition)
                        return true; // return is inside a conditional — auto-return still needed
                }
            }

            return false; // last entry is a top-level return — no auto-return needed
        }();

    size_t cmdCount = _functions.size();
    if (shouldIncludeReturn)
        ++cmdCount;

    for (auto const& f : _functions)
    {
        if (f.id == _pScriptData->endCommand())
            --cmdCount;
    }

    out << L"  <sval type=\"array\" size=\"" << cmdCount << L"\">" << std::endl;

    unsigned int line = 0;
    std::vector<ScriptFunction> gotos;
    for (auto itr = _functions.begin(); itr != _functions.end(); itr++, line++)
    {
        if (itr->id == _pScriptData->endCommand())
        {
            gotos.push_back(*itr);
            continue;
        }
        else if (itr->id == _pScriptData->elseCommand() || itr->id == _pScriptData->continueCommand() || itr->id == _pScriptData->breakCommand())
        {
            gotos.push_back(*itr);

            out << L"    <sval type=\"array\" size=\"2\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << _pScriptData->hiddenGotoCommand() << L"\"/>" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << itr->endLine << L"\"/>" << std::endl;
        }
        else if (itr->id == _pScriptData->hiddenGotoCommand())
        {
            out << L"    <sval type=\"array\" size=\"2\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << _pScriptData->hiddenGotoCommand() << L"\"/>" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << itr->endLine << L"\"/>" << std::endl;
        }
        else if (itr->id == _pScriptData->gotoCommand() || itr->id == _pScriptData->gosubCommand())
        {
            size_t pos = 0;
            if (itr->firstArg() && itr->firstArg()->type() == ParseType::Label)
            {
                auto label = dynamic_cast<const ParseLabel*>(itr->firstArg());
                auto findItr = _labels.find(label->label());
                if (findItr != _labels.end())
                    pos = findItr->second - 1;
            }

            out << L"    <sval type=\"array\" size=\"2\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << itr->id << L"\"/>" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << pos << L"\"/>" << std::endl;
        }
        else if (itr->id == _pScriptData->expressionCommand())
        {
            std::vector<const BaseParse*> newItems;

            // compute the infix expression
            std::vector<const BaseParse*> args;
            _parseExpressionList(itr->arguments(), (itr->retvarID >= 0) ? (itr->arguments().begin() + itr->retvarID) : itr->arguments().end(), args);

            // check for negate
            for (auto nItr = args.begin(); nItr != args.end(); nItr++)
            {
                if ((*nItr)->type() == ParseType::Operator)
                {
                    const ParseOperator* oper = dynamic_cast<const ParseOperator*>(*nItr);
                    if (oper->operType() == Operators::Subtract)
                    {
                        const_cast<ParseOperator*>(oper)->switchType(Operators::Negate);
                        break;
                    }
                    else if (oper->operType() != Operators::OpenBracket && oper->operType() != Operators::CloseBracket)
                        break;
                }
            }

            // compute the postfix expression
            std::vector<const BaseParse*> postfix = _convertToPostfixExpression(args);
            size_t size = (postfix.size() * 2) + args.size() + 4;

            out << L"    <sval type=\"array\" size=\"" << size << L"\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << _pScriptData->expressionCommand() << L"\"/>" << std::endl;

            // write the first argument
            writeArguments(out, *itr, itr->firstArg(), true);

            // write the postfix order
            out << L"      <sval type=\"int\" val=\"" << postfix.size() << L"\"/>" << std::endl;
            for (auto aItr = postfix.begin(); aItr != postfix.end(); aItr++)
                writeArguments(out, *itr, *aItr, false);

            // write the infix order, this is just the operators, and positions of operands in the postfix list
            // the operands positions are negated
            out << L"      <sval type=\"int\" val=\"" << args.size() << L"\"/>" << std::endl;
            for (auto aItr = args.begin(); aItr != args.end(); aItr++)
            {
                const BaseParse* parse = *aItr;
                if (parse->type() == ParseType::Operator)
                    out << L"      <sval type=\"int\" val=\"" << parse->stringData() << L"\"/>" << std::endl;
                else
                {
                    auto findItr = std::find(postfix.begin(), postfix.end(), parse);
                    if (findItr != postfix.end())
                        out << L"      <sval type=\"int\" val=\"" << (-1 - std::distance(postfix.begin(), findItr)) << L"\"/>" << std::endl;
                    // shouldn't happen (TODO: throw an error)
                    else
                        out << L"      <sval type=\"int\" val=\"" << -1 << L"\"/>" << std::endl;
                }
            }
        }
        else
        {
            auto retItr = (itr->retvarID >= 0) ? (itr->arguments().begin() + itr->retvarID) : itr->arguments().end();
            int size = 1;
            for (auto aItr = itr->arguments().begin(); aItr != itr->arguments().end(); aItr++)
            {
                if (aItr == retItr)
                    ++size;
                else if ((*aItr)->pardef() == ParDef::CallName)
                    ++size;
                else
                    size += 2;
            }

            if (itr->undefinedArgs)
                ++size;

            // special handling for "call", we need an extra item for number of "optional" arguments if no arguments are added
            bool hasCallNoArguments = itr->id == 102 && itr->arguments().size() == 3;
            if (hasCallNoArguments)
                ++size;

            out << L"    <sval type=\"array\" size=\"" << size << L"\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << itr->id << L"\"/>" << std::endl;
            unsigned int count = 0;
            for (auto aItr = itr->arguments().begin(); aItr != itr->arguments().end(); aItr++, count++)
            {
                if (itr->undefinedArgs > 0 && count == itr->undefinedStart)
                    out << L"      <sval type=\"int\" val=\"" << itr->undefinedArgs << L"\"/>" << std::endl;

                writeArguments(out, *itr, *aItr, aItr == retItr);
            }

            if (hasCallNoArguments)
                out << L"      <sval type=\"int\" val=\"" << 0 << L"\"/>" << std::endl;
        }

        out << L"    </sval>" << std::endl;
    }

    // add return null at the end always
    if (shouldIncludeReturn)
    {
        out << L"    <sval type=\"array\" size=\"3\">" << std::endl;
        out << L"      <sval type=\"int\" val=\"" << _pScriptData->returnCommand() << L"\"/>" << std::endl;
        out << L"      <sval type=\"int\" val=\"0\"/>" << std::endl;
        out << L"      <sval type=\"int\" val=\"0\"/>" << std::endl;
        out << L"    </sval>" << std::endl;
    }

    out << L"  </sval>" << std::endl;

    // script argument definitions
    if (_arguments.empty())
        out << L"  <sval type=\"int\" val=\"0\"/>" << std::endl;
    else
    {
        out << L"  <sval type=\"array\" size=\"" << _arguments.size() << L"\">" << std::endl;
        for (auto itr = _arguments.begin(); itr != _arguments.end(); itr++)
        {
            out << L"    <sval type=\"array\" size=\"2\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << static_cast<unsigned int>(itr->parDef) << L"\"/>" << std::endl;
            out << L"      <sval type=\"string\" val=\"" << itr->description << L"\"/>" << std::endl;
            out << L"    </sval>" << std::endl;
        }

        out << L"  </sval>" << std::endl;
    }

    std::wstring comment = L"Compiled by XScript Compiler v0.6 - https://www.xpluginmanager.co.uk/xscript/";

    // add goto positions
    if (gotos.size())
    {
        out << L"  <sval type=\"array\" size=\"" << (gotos.size() + 1) << L"\">" << std::endl;
        out << L"    <sval type=\"array\" size=\"3\">" << std::endl;
        out << L"      <sval type=\"int\" val=\"0\"/>" << std::endl;
        out << L"      <sval type=\"int\" val=\"1\"/>" << std::endl;
        out << L"      <sval type=\"string\" val=\"" << comment << "\"/>" << std::endl;
        out << L"    </sval>" << std::endl;

        for (auto itr = gotos.begin(); itr != gotos.end(); itr++)
        {
            out << L"    <sval type=\"array\" size=\"2\">" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << itr->startLine << L"\"/>" << std::endl;
            out << L"      <sval type=\"int\" val=\"" << itr->id << L"\"/>" << std::endl;
            out << L"    </sval>" << std::endl;
        }

        out << L"  </sval>" << std::endl;
    }
    else
    {
        out << L"  <sval type=\"array\" size=\"1\">" << std::endl;
        out << L"    <sval type=\"array\" size=\"3\">" << std::endl;
        out << L"      <sval type=\"int\" val=\"0\"/>" << std::endl;
        out << L"      <sval type=\"int\" val=\"1\"/>" << std::endl;
        out << L"      <sval type=\"string\" val=\"" << comment << "\"/>" << std::endl;
        out << L"    </sval>" << std::endl;
        out << L"  </sval>" << std::endl;
    }

    // script command
    out << L"  <sval type=\"int\" val=\"" << _command << L"\"/>" << std::endl;

    out << L"</sval>" << std::endl;
    out << L"</codearray>" << std::endl;
    out << L"<nosignature>0</nosignature>" << std::endl;
    out << L"</script>" << std::endl;

    out.close();

    return true;
}

void CScript::_simplifyExpression(const BaseParse* parse, std::vector<const BaseParse*>& newArgs)
{
    if (parse->type() == ParseType::Brackets)
    {
        const ParseBrackets* brackets = dynamic_cast<const ParseBrackets*>(parse);

        std::vector<const BaseParse*> newList;

        for (auto itr = brackets->list().begin(); itr != brackets->list().end(); itr++)
            _simplifyExpression(*itr, newList);

        const_cast<ParseBrackets*>(brackets)->clear();
        if (newList.size() == 1)
        {
            newArgs.push_back(newList.front());
            delete brackets;
            return;
        }
        else if (newList.size() < 1)
        {
            delete brackets;
            return;
        }

        for (auto itr = newList.begin(); itr != newList.end(); itr++)
            const_cast<ParseBrackets*>(brackets)->addParse(const_cast<BaseParse*>(*itr));
        newList.clear();
    }
    else if (parse->type() == ParseType::Expression)
    {
        const ParseExpression* expression = dynamic_cast<const ParseExpression*>(parse);

        std::vector<const BaseParse*> newList;

        for (auto itr = expression->list().begin(); itr != expression->list().end(); itr++)
            _simplifyExpression(*itr, newList);

        const_cast<ParseExpression*>(expression)->clearList();
        if (newList.size() == 1)
        {
            newArgs.push_back(newList.front());
            delete expression;
            return;
        }
        else if (newList.size() < 1)
        {
            delete expression;
            return;
        }

        for (auto itr = newList.begin(); itr != newList.end(); itr++)
            const_cast<ParseExpression*>(expression)->addParse(const_cast<BaseParse*>(*itr));
        newList.clear();
    }

    newArgs.push_back(parse);
}

void CScript::_parseExpressionList(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>::const_iterator ignoreItem, std::vector<const BaseParse*>& output)
{
    for (auto itr = list.begin(); itr != list.end(); itr++)
    {
        if (itr == ignoreItem)
            continue;

        if ((*itr)->type() == ParseType::Brackets)
        {
            const ParseBrackets* bracket = dynamic_cast<const ParseBrackets*>(*itr);
            if (bracket->singleItem())
            {
                if (bracket->singleItem()->type() == ParseType::Expression)
                {
                    ParseOperator* open = new ParseOperator(bracket->line(), L"(");
                    _createdItems.push_back(open);
                    output.push_back(open);

                    auto bList = dynamic_cast<const ParseExpression*>(bracket->singleItem())->list();
                    _parseExpressionList(bList, bList.end(), output);

                    ParseOperator* close = new ParseOperator(bracket->line(), L")");
                    _createdItems.push_back(close);
                    output.push_back(close);

                }
                else if (bracket->singleItem()->type() == ParseType::Function)
                    output.push_back(dynamic_cast<const ParseFunction*>(bracket->singleItem())->returnVariable());
                else
                    output.push_back(bracket->singleItem());
            }
            else
            {
                // add the brackets list, but include the open and close brackets surrounding it
                ParseOperator* open = new ParseOperator(bracket->line(), L"(");
                _createdItems.push_back(open);
                output.push_back(open);

                auto l = bracket->constList();
                _parseExpressionList(l, l.end(), output);

                ParseOperator* close = new ParseOperator(bracket->line(), L")");
                _createdItems.push_back(close);
                output.push_back(close);
            }
        }
        else if ((*itr)->type() == ParseType::Function)
            output.push_back(dynamic_cast<const ParseFunction*>(*itr)->returnVariable());
        else if ((*itr)->type() == ParseType::Array)
        {
            const ParseArray* arr = dynamic_cast<const ParseArray*>(*itr);
            if (arr->assignment())
                output.push_back(arr->assignment());
        }
        else if ((*itr)->type() == ParseType::Expression)
        {
            const ParseExpression* expression = dynamic_cast<const ParseExpression*>(*itr);
            auto l = expression->list();
            if (expression->list().size() == 1)
                _parseExpressionList(l, l.end(), output);
            else
            {
                if (!output.empty())
                {
                    if (output.back()->type() == ParseType::Operator)
                    {
                        const ParseOperator* oper = dynamic_cast<const ParseOperator*>(output.back());
                        if (oper->operType() == Operators::CloseBracket)
                        {
                            _parseExpressionList(l, l.end(), output);
                            continue;
                        }
                    }
                }
                ParseOperator* open = new ParseOperator(expression->line(), L"(");
                _createdItems.push_back(open);
                output.push_back(open);

                _parseExpressionList(l, l.end(), output);

                ParseOperator* close = new ParseOperator(expression->line(), L")");
                _createdItems.push_back(close);
                output.push_back(close);
            }
        }
        else
            output.push_back(*itr);
    }
}

int CScript::_operatorPriority(const ParseOperator* oper) const
{
    switch (oper->operType())
    {
    case Operators::Add:
    case Operators::Subtract:
        return 1;
    case Operators::Multiple:
    case Operators::Divide:
        return 2;
    case Operators::Xor:
        return 3;
    case Operators::OpenBracket:
    case Operators::CloseBracket:
        return -1;
    }

    return 0;
}

std::vector<const BaseParse*> CScript::_convertToPostfixExpression(const std::vector<const BaseParse*>& list) const
{
    std::vector<const BaseParse*> newList;
    std::stack<const ParseOperator*> s;

    for (auto itr = list.begin(); itr != list.end(); itr++)
    {
        const BaseParse* parse = *itr;
        if (parse->type() != ParseType::Operator)
            newList.push_back(parse);
        else
        {
            const ParseOperator* oper = dynamic_cast<const ParseOperator*>(parse);
            // add open bracket to stack
            if (oper->operType() == Operators::OpenBracket)
                s.push(oper);
            // push stack to the list until next open bracket
            else if (oper->operType() == Operators::CloseBracket)
            {
                while (!s.empty() && s.top()->operType() != Operators::OpenBracket)
                {
                    newList.push_back(s.top());
                    s.pop();
                }
                if (!s.empty() && s.top()->operType() == Operators::OpenBracket)
                    s.pop();
                // if stack empty here, there was a mismatched close bracket; continue safely
            }
            else
            {
                while (!s.empty() && _operatorPriority(oper) <= _operatorPriority(s.top()))
                {
                    newList.push_back(s.top());
                    s.pop();
                }
                s.push(oper);
            }
        }
    }

    while (!s.empty())
    {
        // if there are unmatched brackets left, skip them rather than crash
        if (s.top()->operType() == Operators::OpenBracket || s.top()->operType() == Operators::CloseBracket)
        {
            s.pop();
            continue;
        }
        newList.push_back(s.top());
        s.pop();
    }

    return newList;
}

bool CScript::_isDatatypeString(DataTypes dt) const
{
    if (dt == DataTypes::String)
        return true;

    auto custom = _pScriptData->getCustomDatatype(dt);
    if (custom && custom->isStringData)
        return true;

    return false;
}

ScriptFunction* CScript::lastFunc()
{
    return (_lastAddedIndex >= 0) ? (_lastAddedIsPost ? &_pendingPostRun[_lastAddedIndex] : &_functions[_lastAddedIndex]) : nullptr;
}