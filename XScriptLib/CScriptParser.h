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
		unsigned int _tabSize;
		bool		 _isInComment;
		bool		 _prePassMode;
		bool		 _subEndedOnLine;
		int			 _prePassDepth;  // brace depth during pre-pass, endsub/return only terminates sub at depth 0
		std::vector<std::wstring> _currentFile;
		std::vector<std::vector<const BaseParse*>> _deferredLists;

		// Per-label variable type maps built during pass 1 (prePassLine).
		// Key: label name. Value: map of variable name → DataTypes set.
		// Merged into _variables during pass 2 when gosub is encountered.
		std::map<std::wstring, std::map<const std::wstring, std::unordered_set<DataTypes>>> _subVariables;
		std::wstring _currentSubLabel; // which sub we are currently collecting for in pass 1

		mutable std::vector<void*> _syntheticConstants;

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

		bool _runProperty(ParseProperty* property, bool isInline, bool doAssignment);
		bool _runArrayFunction(ParseArray* function, bool isInline, bool doAssignment);
		bool _runFunction(ParseFunction* function, bool isInline);
		bool _doInternalFunction(InternalFunctions func, const ParseArguments* arguments);
		bool _doGlobalFunction(const Function* func, ParseFunction* functionData, bool isInline);
		bool _runDataList(const std::vector<const BaseParse*>& list, bool topLevel);
		void _checkWarnings(const std::vector<const BaseParse*>& list);
		bool _runExpressionList(const ParseExpression* expression, bool topLevel);
		bool _runParse(const BaseParse* parse, const BaseParse* previous, const ParseCondition* condition, bool topLevel, bool isInline);
		bool _parseDataList(std::vector<const BaseParse*>& list);
		void _addExpressionItem(const BaseParse* parse);
		bool _addLabel(const ParseKeyword* keyword);

		bool _setArguments(const ParseArguments* arguments);
		bool _setDescription(const ParseArguments* arguments);
		bool _setVersion(const ParseArguments* arguments);
		bool _setCommand(const ParseArguments* arguments);

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

		Warnings &_addWarning(ParseWarnings type, const BaseParse *parse);
		ParseFail* _addError(ParseErrors error, const BaseParse* parse);
		void _errorArgumentDatatype(const ParseArguments *arguments, unsigned int i, DataTypes wanted);
		std::wstring _formatString(DataTypes dt, const std::wstring& str);
	};

}
