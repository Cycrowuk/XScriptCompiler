#pragma once

/*
* 
*	Format for datatypes
*		$variable
*		[Constant]
*		TYPE			(internal types, like pardef)
*		#OBJECT
*		{Ware}			(Lookup data)
*		"String"
*       123				(number)
*/

#include "BaseParse.h"

// Forward declarations for types used by pointer only in this header
namespace XScript { struct MacroData; }

namespace XScript {
	class CScript;
	class CScriptData;
	//class BaseParse;
	class ParseKeyword;
	class ParseFail;
	class ParseArguments;
	class ParseBrackets;
	class ParseFunction;
	class ParseExpression;
	class ParseArray;
	class ParseProperty;
	class ParseDefine;

	typedef std::vector<Argument> ArgumentsList;

	enum class ParseStatus
	{
		Start,
		KeyWord,
		Argument,
		String,
		EndString,
		Symbol,
		Brackets,
		Key,
		Whitespace,
		WhitespaceKeyWord,
		WhitespaceSymbol,
		WhitespaceInteger,
		Integer,
		ObjectFunction,
		Function,
		Arguments,
		Finished,
		Comment,
		End,
	};

	enum class KeyWordStatus
	{
		Unknown,
		Object,
		Function,
		Argument,
	};


	enum class ParseWarnings
	{
		InvalidDataType,
		InvalidReturnType,
		InvalidObjectDataType,
		InvalidArrayValue,
		InvalidConstantGroup,
		NullVariable,
		InvalidExpressionOperator,
	};

	struct Warnings
	{
		ParseWarnings warning;
		size_t		  colPos;
		size_t		  endPos;
		size_t		  linePos;
		std::wstring  line;
		std::wstring  file;
		std::vector<std::wstring> data;
	};

	class CScriptParser
	{
	private:
		CScript*				_currentScript;
		const CScriptData*		_data;

		std::map<const std::wstring, std::unordered_set<DataTypes>> _variables;
		std::map<const std::wstring, std::unordered_set<DataTypes>>* _pVariables; // points to _variables or current sub map
		std::vector<Warnings> _warnings;

		std::vector<const ParseFail*> _errors;
		std::vector<const BaseParse*> _currentDataList;
		std::vector<const BaseParse*> _createdData;
		std::map<const std::wstring, const ParseDefine*> _defines;

		unsigned int _generatedVariables;
		unsigned int _whileGeneratedVariables; // separate counter for while-scoped temp vars
		unsigned int _tabSize;
		bool		 _isInComment;
		bool		 _prePassMode;
		bool		 _subEndedOnLine;
		bool		 _inLineContinuation;  // true when current line ends with \ (define continuation)
		std::wstring _continuationText;    // accumulated text across continuation lines

		// Macro expansion state
		struct MacroCallState
		{
			const MacroData*         macro;
			std::vector<std::wstring> args;    // substituted argument strings
			std::vector<const BaseParse*> argNodes; // cloned single-token arg nodes (or nullptr), owned
			std::vector<std::wstring> body;    // captured body lines (for BlockCommands)
			int                       depth;   // brace depth counter for body capture
			bool                      inBody;  // currently capturing body lines
			size_t                    linePos; // original source line number for error reporting
			std::wstring              sourceLine; // original source text for error reporting
		};
		std::vector<MacroCallState> _macroStack; // supports nested macros

		// Function definition state
		// When parsing a function definition body, _inFunctionDef is true.
		// _functionDefDepth tracks brace nesting so we know when the body ends.
		bool         _inFunctionDef      = false;
		int          _functionDefDepth   = 0;
		std::wstring _functionDefName;   // e.g. "main"
		std::unordered_set<DataTypes> _functionReturnTypes; // empty = no declared return type (no checking)

		// True once any label (whether from "name:" or "sub name() {" sugar) has
		// been defined in the current function body. Used to validate that a
		// bare "endsub" call only appears after at least one sub/label exists.
		bool _anyLabelSeen = false;

		// "sub name() { ... }" sugar state — tracks brace depth so the matching
		// "}" can be replaced with an automatic "endsub;" call.
		bool _inSubDef    = false;
		int  _subDefDepth = 0;

		// True once "function main() {...}" has been fully closed. Subs that
		// appear in the source BEFORE main closes have their actions held in
		// _pendingSubActions rather than run immediately, so that main's
		// compiled commands always come first in the output regardless of
		// where in the source file the sub physically appears.
		bool _mainCompleted = false;

		// A single deferred sub action — either defining a label (the sub's
		// own "name:" entry) or running a fully-parsed statement (the sub's
		// body content, including the synthetic "endsub;" call at its close).
		// Stored in source order so subs that appeared before main still
		// compile out in the same relative order, just after main's commands.
		struct PendingSubAction
		{
			const ParseKeyword* label = nullptr;        // non-null = "add this label"
			std::vector<const BaseParse*> statement;     // non-empty = "run this statement"
		};
		std::vector<PendingSubAction> _pendingSubActions;
		std::unordered_set<std::wstring> _deferredLabelNames; // tracks label names that have been deferred but not yet written to _currentScript

		// When inside macro expansion, points to a synthetic node representing the
		// original macro call site (for error/warning location), and the list of
		// argument strings (e.g. "$myArr") + their original parse nodes (if a
		// single-token argument) — used to redirect warnings about a specific
		// macro argument to its exact position at the call site.
		const BaseParse*           _macroCallNode = nullptr;
		std::vector<std::wstring>  _macroCallArgNames;
		std::vector<const BaseParse*> _macroCallArgNodes;
		int			 _prePassDepth;  // brace depth during pre-pass, endsub/return only terminates sub at depth 0
		std::vector<std::wstring> _currentFile;
		std::vector<std::vector<const BaseParse*>> _deferredLists;

		// Per-label variable type maps built during pass 1 (prePassLine).
		// Key: label name. Value: map of variable name → DataTypes set.
		// Merged into _variables during pass 2 when gosub is encountered.
		std::map<std::wstring, std::map<const std::wstring, std::unordered_set<DataTypes>>> _subVariables;
		std::wstring _currentSubLabel; // which sub we are currently collecting for in pass 1

		mutable std::vector<void*> _syntheticConstants;

		// Maps a ParseFunction* to the ParseExpression* that was created for it
		// in _doGlobalFunction (when RetVarType::Return doesn't match the condition).
		// Used by the StartBlock handler to find the correct expression when '{' follows.
		std::map<const BaseParse*, const ParseExpression*> _createdExpressions;

		// #ifdef / #ifndef / #else / #elseif / #endif conditional stack.
		struct IfDefEntry
		{
			bool active;    // is the current branch being compiled?
			bool anyActive; // has any branch in this chain already been active?
		};
		std::vector<IfDefEntry> _ifDefStack;

		// Condition depth stack — one entry per open block.
		// Pushed when a condition with a block opens (if, while, etc.)
		// Popped when the matching '}' or end block is reached.
		// Used to determine what re-evaluation is needed before 'continue'.
		enum class ConditionType { If, ElseIf, Else, While };
		struct ConditionEntry
		{
			ConditionType type;
			const ParseExpression* whileExpression; // only set for While entries
			size_t whilePostRunStart; // index of first post-run function in _functions
			size_t whilePostRunCount; // number of post-run functions to duplicate
			std::vector<const BaseParse*> whilePostRunNodes; // ParseFunction* nodes for inc/dec
			ConditionEntry(ConditionType t, const ParseExpression* expr = nullptr)
				: type(t), whileExpression(expr), whilePostRunStart(0), whilePostRunCount(0) {}
		};
		std::vector<ConditionEntry> _conditionStack;

	public:
		static BaseParse* CopyParse(const BaseParse* parse);

	public:
		CScriptParser(const CScriptData *data);
		~CScriptParser();

		CScript* currentScript() const;
		const std::vector<const ParseFail *>& errorData() const;
		bool hasWarnings() const;
		const std::vector<Warnings>& warnings() const;

		void addCurrentFile(const std::wstring& file);
		void removeCurrentFile();
		void addDefine(const std::wstring& name); // add a pre-defined symbol (e.g. from command line)
		bool includeFile(const std::wstring& filename); // process an #include file inline
		bool _expandMacro(const MacroData* macro, const std::vector<std::wstring>& args, const std::vector<const BaseParse*>& argNodes, const std::vector<std::wstring>& body, size_t linePos, const std::wstring& sourceLine);
		const BaseParse* _resolveMacroReportNode(const BaseParse* parse) const;
		bool _parseFunctionDefinition(const std::vector<const BaseParse*>& list, size_t linePos);
		bool _parseSubDefinition(const std::vector<const BaseParse*>& list, size_t linePos);
		BaseParse* parseCondition(const std::wstring& line) const;
		BaseParse* parseConstant(const std::wstring& line) const;
		bool parseLine(size_t linePos, const std::wstring &line);
		bool prePassLine(size_t linePos, const std::wstring &line);
		void resetForRealPass();
		bool finalise();

	private:
		std::wstring _parseDefine(const std::wstring& line);
		bool _checkListOrder(const BaseParse *parse, ParseErrors errorType);
		bool _checkListOrder(const std::vector<const BaseParse*>& list, ParseErrors errorType);
		bool findProperties(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>& newList);
		bool parseProperties(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>& newList);
		bool parseConstants(const std::vector<const BaseParse*>& list, std::vector<const BaseParse*>& newList);
		BaseParse* checkStatus(ParseStatus oldStatus, ParseStatus newStatus, std::wstring& str, wchar_t c, const std::wstring& line);
		bool parseListBrackets(std::vector<const BaseParse*>& list);
		std::vector<const BaseParse*>::iterator _parseEndArray(std::vector<const BaseParse*>& list, std::vector<const BaseParse*>::iterator startItr, ParseArray* arr);
		bool _parseArrays(std::vector<const BaseParse*>& list, bool topLevel);
		bool _findArrays(std::vector<const BaseParse*>& list, bool topLevel);
		bool _parseNamespaces(std::vector<const BaseParse*>& list);
		bool _parsePreprocessor(std::vector<const BaseParse*>& list);
		bool _parseAllConditions(std::vector<const BaseParse*>& list);
		bool _parseCompoundAssignment(std::vector<const BaseParse*>& list);
		bool parseFunctions(const std::vector<const BaseParse*>& originalList, std::vector<const BaseParse*>& parseList);
		bool _parseExpressions(const std::vector<const BaseParse*>& originalList, std::vector<const BaseParse*>& parseList);
		bool _checkExpressionValidity(const std::vector<const BaseParse*>& list, bool topLevel);
		bool _checkExpressionValidity(const BaseParse* parse);

		std::vector<const BaseParse*>::iterator addBracket(std::vector<const BaseParse*>::iterator startItr, const std::vector<const BaseParse*> &list, ParseBrackets* currentBracket);

		void _clearData();

		// Inline state for _doGlobalFunction and related calls.
		// Normal:          not inline — top-level statement
		// Inline:          inline within an expression (e.g. nested function call)
		// InlineCondition: inline within condition brackets of an else-if — temp vars
		//                  must be inserted before the opening if block, not appended
		enum class InlineState { Normal, Condition, Inline, InlineCondition };

		bool _runProperty(ParseProperty* property, InlineState inlineState, bool doAssignment);
		bool _runArrayFunction(ParseArray* function, InlineState inlineState, bool doAssignment);
		bool _runFunction(ParseFunction* function, InlineState inlineState);
		bool _doInternalFunction(InternalFunctions func, const ParseArguments* arguments);
		bool _doGlobalFunction(const Function* func, ParseFunction* functionData, InlineState inlineState);
		bool _runDataList(const std::vector<const BaseParse*>& list, bool topLevel);
		void _checkWarnings(const std::vector<const BaseParse*>& list);
		bool _runExpressionList(const ParseExpression* expression, bool topLevel, const ParseExpression* previous, bool isInCondition);
		bool _runParse(const BaseParse* parse, const BaseParse* previous, const ParseCondition* condition, bool topLevel, InlineState inlineState);
		bool _parseDataList(std::vector<const BaseParse*>& list);
		void _addExpressionItem(const BaseParse* parse);
		bool _addLabel(const ParseKeyword* keyword);
		void _emitWhileReEval(); // re-emit temp var assignments for the innermost while
		void _emitWhileReEvalList(const std::vector<const BaseParse*>& list);

		bool _setArguments(const ParseArguments* arguments);
		bool _setDescription(const ParseArguments* arguments);
		bool _setVersion(const ParseArguments* arguments);
		bool _setCommand(const ParseArguments* arguments);

		bool _isInline(InlineState state) const;
		bool _isCondition(InlineState state) const;
		DataTypes _getDataTypeFromParse(const BaseParse* parse) const;
		std::wstring _getPardefFlagString(ParDefFlags flags) const;
		bool _isDataTypeForObject(DataTypes dt) const;
		DataTypes _getActualDataType(const BaseParse* parse) const;
		std::unordered_set<DataTypes> _getActualDataTypes(const BaseParse* parse) const;
		bool _doesObjectTypeMatch(DataTypes parent, DataTypes child) const;
		bool _isValidDataType(DataTypes from, std::unordered_set<DataTypes> _to) const;
		bool _isValidDataType(std::unordered_set<DataTypes> _from, std::unordered_set<DataTypes> _to) const;
		std::wstring _getDataTypesString(std::unordered_set<DataTypes> datatypes) const;
		ParseVariable* _generateTempVariable(const BaseParse* source);
		std::wstring _makeTempVarName(); // returns $VarGenWhile.N inside a while, else $VarGen.N

		Warnings &_addWarning(ParseWarnings type, const BaseParse *parse);
		ParseFail* _addError(ParseErrors error, const BaseParse* parse);
		void _errorArgumentDatatype(const ParseArguments *arguments, unsigned int i, DataTypes wanted);
		std::wstring _formatString(DataTypes dt, const std::wstring& str);
	};

}
