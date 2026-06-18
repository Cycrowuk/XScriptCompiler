#pragma once

namespace XScript
{
	class BaseParse;
	class ParseCondition;
	class ParseVariable;
	class ParseOperator;
	class ParseKeyword;
	class ParseFunction;
	class ScriptFunction
	{
	public:
		unsigned int id;
		const ParseFunction* function;
		int retvarID = -1;
		int endBlock = -1;
		size_t endLine = 0;
		size_t startLine = 0;
		unsigned int undefinedArgs = 0;
		int undefinedStart = -1;
		bool isPost = false;

		ScriptFunction() : id(0), function(NULL), retvarID(-1), endBlock(-1), endLine(0), startLine(0), isPost(false)
		{

		}
		ScriptFunction(unsigned int id_, const ParseFunction* function_) : id(id_), function(function_), retvarID(-1), endBlock(-1), endLine(0), startLine(0)
		{

		}
		ScriptFunction(unsigned int id_, const ParseFunction* function_, int retvarID_, int endBlock_, size_t endLine_) : id(id_), function(function_), retvarID(retvarID_), endBlock(endBlock_), endLine(endLine_), startLine(0)
		{

		}
		ScriptFunction(unsigned int id_, const ParseFunction* function_, int retvarID_, int endBlock_, size_t endLine_, size_t startLine_) : id(id_), function(function_), retvarID(retvarID_), endBlock(endBlock_), endLine(endLine_), startLine(startLine_)
		{

		}

		void addArgument(const BaseParse* parse, ParDef pardef);

		size_t argumentCount() const
		{
			return _arguments.size();
		}

		const BaseParse* firstArg() const
		{
			if (_arguments.empty())
				return NULL;
			return _arguments.front();
		}

		const BaseParse* retvarArgument() const
		{
			if (retvarID >= 0)
			{
				if (_arguments.size() > retvarID)
					return _arguments[retvarID];
			}
			return NULL;
		}

		void clearArguments()
		{
			_arguments.clear();
		}

		const std::vector<const BaseParse*> &arguments() const
		{
			return _arguments;
		}

	private:
		std::vector<const BaseParse*> _arguments;
	};

	class CScriptData;
	class CScript
	{
	private:
		std::vector<ScriptFunction> _functions;
		std::vector<ScriptArguments> _arguments;
		std::wstring		_description;
		unsigned int		_version;
		unsigned int		_command;

		const CScriptData* _pScriptData;

		std::map<std::wstring, size_t> _labels;
		std::vector<std::wstring> _variables;
		std::map<std::wstring, unsigned int> _variablesLookup;
		std::vector<const BaseParse*> _createdItems;
		std::vector<ScriptFunction> _pendingPostRun;
		int _lastAddedIndex = -1;
		bool _lastAddedIsPost = false;
		int _lastInsertPos = -1;
		bool _pendingEnd = false; // proposed end block — flushed lazily, skipped before else/else-if
		bool _forceEnd = false; // force end block, even if not strictly necessary (e.g. after return)
		bool _lastEnsureReturnInserted = false; // set by ensureReturn() — true if a synthetic return(null) was added

	public:
		CScript(const CScriptData *data);
		~CScript();

		void addArgument(const std::wstring& variable, const std::wstring& description, ParDef parameterDefinition, const std::wstring &parDefName);
		void setDescription(const std::wstring& desc);
		void setVersion(unsigned int version);
		void setCommand(unsigned int command);

		void addNewExpression(const ParseVariable* vari);
		void addNewExpression(const ParseCondition* cond);
		void addVariable(const std::wstring& variable);
		void addFunction(unsigned int id, const ParseFunction *func, bool postRun, bool suppressFlush = false);
		void addRetVar(const BaseParse* arg);
		void addFunctionArgument(const BaseParse* arg, ParDef pardef);
		void addFunctionCondition(const ParseCondition *c);
		bool addEndBlock(bool forceBlock);
		bool addLabel(const ParseKeyword *label);
		void setFunctionUndefinedCount(unsigned int count);
		void flushPostRun();

		bool isIfOpen() const;
		bool isInWhile() const;
		const ScriptFunction* previousFunction();
		size_t functionCount() const;
		void duplicateFunction(size_t index);
		void insertFunction(unsigned int id, const ParseFunction* func); // insert before last if block
		void insertNewExpression(const ParseVariable* vari); // insert before last if block
		const std::vector<ScriptFunction>& functions() const;
		bool isLabelValid(const std::wstring& label) const;

		bool finalise();
		bool save(const std::wstring& file, const std::vector<Function> &functionData);
		void writeArguments(std::wofstream& out, const ScriptFunction& func, const BaseParse* parse, bool isRetvar) const;
		void ensureReturn(); // append a synthetic "return(null)" if the last entry isn't already a top-level return; returns true if one was inserted
		bool ensureReturnWasInserted() const { return _lastEnsureReturnInserted; }
		bool isLabelAvailable(const std::wstring& name) const; // true if this label name hasn't been used yet

	private:
		bool _addEndBlock(bool forceBlock);
		bool _needsAutoReturn() const; // shared logic: does the function/script need a synthetic return appended?
		void _addVariables(const BaseParse* arg);
		void _simplifyExpression(const BaseParse* parse, std::vector<const BaseParse*>& newArgs);
		void _parseExpressionList(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>::const_iterator ignoreItem, std::vector<const BaseParse*>&output);
		int _operatorPriority(const ParseOperator* oper) const;
		std::vector<const BaseParse*> _convertToPostfixExpression(const std::vector<const BaseParse*>& list) const;
		bool _isDatatypeString(DataTypes dt) const;
		ScriptFunction* lastFunc();
	};

}

