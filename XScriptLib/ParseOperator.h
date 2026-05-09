#pragma once
#include "BaseParse.h"

#define _SCRIPT_OP_FLAG_ONEOP	0x10000

namespace XScript
{
	enum class Operators
	{
		Unknown			= -1,
		Equals			= 0,
		NotEquals		= 1,
		Greater			= 2,
		Lesser			= 3,
		GreaterEquals	= 4,
		LesserEquals	= 5,
		And				= 6,
		Or				= 7,
		Xor				= 8,
		BoolAnd			= 9,
		BoolOr			= 10,
		Add				= 11,
		Subtract		= 12,
		Multiple		= 13,
		Divide			= 14,
		Modulus			= 15,
		OpenBracket		= 16,
		CloseBracket	= 17,
		Not				= (_SCRIPT_OP_FLAG_ONEOP | 18),
		Negate			= (_SCRIPT_OP_FLAG_ONEOP | 19),
		BoolNot			= (_SCRIPT_OP_FLAG_ONEOP | 20),
	};


	class ParseOperator : public BaseParse
	{
	private:
		Operators _operator;
		std::wstring _str;

	public:
		static Operators ConvertOperator(const std::wstring& symbol);

	public:
		ParseOperator(const std::wstring& line, const std::wstring &symbol);
		virtual ~ParseOperator();

		void switchType(Operators newType);

		DataTypes dataType() const override;
		std::wstring stringData() const override;
		const std::wstring& operString() const;
		Operators operType() const;
		bool isOperSingle() const;
		bool isComparison() const;
		bool isNumericOperator() const;
	};

}

