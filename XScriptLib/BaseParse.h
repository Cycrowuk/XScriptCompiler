#pragma once

#include <windows.h>
#include <sstream>
#define DBOUT( s )            \
{                             \
   std::wstringstream os_;    \
   os_ << s << L"\n";         \
   OutputDebugString( os_.str().c_str() );  \
}

namespace XScript
{
	enum class ParseType
	{
		Failed,
		Function,
		Expression,
		Variable,
		String,
		Constant,
		Arguments,
		Integer,
		Condition,
		Keyword,
		Object,
		Assignment,
		Brackets,
		Symbol,
		Operator,
		Array,
		Null,
		Label,
		Namespace,
		Property,
		Define,
		Include,
		Unknown,
	};
	enum class ParseErrors
	{
		Success,			// No Errors
		MissingQuote,		// Missing matching " for string
		MissingBracket,		// Missing matching ) for function
		MissingStartBracket,
		MissingEndBrace,	// Missing matching }
		MissingStartBrace,
		MissingSemiColon,	// Missing ; at end of line
		MissingSemiColonEnd,	// Missing ; at end of script file
		MissingAssignment,
		MissingIf,
		MissingEndArray,
		MissingStartArray,
		MissingArrayVariable,
		MissingArraySubscript,
		MissingArrayAssignment,
		MissingSpecialArgument,
		MissingFunctionArgument,
		MissingWhile,
		MissingObjectFunction,
		MissingLabel,
		MissingPreprocessor,
		MissingDefine,
		DataAfterFunction,
		DataBeforeEndArray,
		TooMuchData,
		InternalFunctionError,		// Missing internal function
		InvalidKeyword,
		InvalidComma,
		InvalidString,
		InvalidLabel,
		InvalidAssignmentVariable,
		InvalidDoubleCondition,
		InvalidArgumentCount,
		InvalidArgumentType,
		InvalidArgumentDataType,
		InvalidSymbol,
		InvalidNumber,
		InvalidNamespace,
		InvalidVariable,
		InvalidVariableName,
		InvalidReturnValue,
		InvalidObject,
		InvalidObjectDataType,
		InvalidSpecialArgument,
		InvalidKeywordIntStart,
		InvalidKeywordDollar,
		InvalidArray,
		InvalidExpressionBrackets,
		InvalidExpressionUnaryOperator,
		InvalidExpressionEmpty,
		InvalidStartCondition,
		InvalidCondition,
		InvalidAssignment,
		InvalidPreprocessor,
		InvalidDefine,
		FunctionWithoutRetVar,
		FunctionRequiresRetVar,
		UnknownFunction,
		UnknownProperty,
		UnknownConstant,
		UnknownObjectFunction,
		UnknownPreprocessor,
		AmbiguousObjectFunction,
		IncompleteLine,
		InvalidFunctionDefinition,	// Generic/fallback — malformed function definition header
		CodeOutsideFunction,		// Code found outside a function definition block
		NestedFunctionDefinition,	// "function" keyword found while already inside a function
		MissingFunctionBodyBrace,	// Content found before the opening { of a function body
		MissingFunctionName,		// No valid function name after "function" / return type
		MissingFunctionParameterList,	// No "(...)" parameter list after the function name
		InvalidFunctionParameterType,	// Parameter type is not a recognised pardef/datatype
		MissingFunctionParameterVariable,	// Parameter type not followed by a $variable
		DuplicateFunctionParameterName,	// Same $variable name used more than once
		EndsubWithoutLabel,			// "endsub" used before any label/sub has been defined
		DuplicateLabel,				// Label or sub name already exists
		MissingSubName,				// "sub" keyword not followed by a valid name
		MissingSubParameterList,		// "sub name" not followed by "()"
		SubParametersNotAllowed,		// "sub name(...)" — subs cannot take parameters
		NestedSubDefinition,			// "sub" found while already inside a sub block
		MissingSubBodyBrace,			// Content found before the opening { of a sub body
		UserFunctionCallNotStandalone,	// User function call used inside a larger expression (not supported)
		UserFunctionArgumentCountMismatch,	// Wrong number of arguments passed to a user function call
		ReturnValueNotAllowed,			// "return($x)" used inside main/a sub where it isn't valid
		DuplicateUserFunctionName,		// A "function name(...)" definition's name collides with an existing label/sub/function
		UserFunctionNameConflict,		// Function name collides with an existing script command or constant
	};
	enum class Conditions
	{
		None,
		Start,
		If,
		IfNot,
		Else,
		ElseIf,
		ElseIfNot,
		SkipIf,
		SkipIfNot,		//doif
		While,
		WhileNot,
		Not,
	};

	class ParseVariable;
	class ParseCondition;

	class BaseParse
	{
	protected:
		size_t			_startPos;
		size_t			_lastPos;
		size_t			_linePos;
		bool			_whitespaceBefore;
		bool			_whitespaceAfter;
		ParDef			_parDef;
		ParseType		_type;
		std::wstring	_line;
		std::wstring	_data;
		std::wstring	_file;

	public:
#ifdef _DEBUG
		static int _DEBUG_COUNT;
#endif

		BaseParse(const std::wstring& l, ParseType type_);
		virtual ~BaseParse();

		virtual DataTypes dataType() const;
		virtual std::wstring stringData() const;
		virtual unsigned int lineCount() const;
		virtual void simplify();

		void setData(const std::wstring& data);
		void setPosition(size_t start, size_t end);
		void setLinePosition(size_t pos);
		void setFile(const std::wstring& file);
		void setFromParse(const BaseParse* parse);
		void setWhitespaceBefore(bool b);
		void setWhitespaceAfter(bool b);
		void setParDef(ParDef pardef);

		ParDef pardef() const;
		size_t startingPos() const;
		size_t endingPos() const;
		size_t linePos() const;
		bool hasWhitespaceBefore() const;
		bool hasWhitespaceAfter() const;

		const std::wstring& line() const;
		const std::wstring& file() const;
		const std::wstring& data() const;

		ParseType type() const;
	};

}