#pragma once
#include "BaseParse.h"

namespace XScript
{
	class ParseVariable;
	class ParseCondition;
	class ParseExpression : public BaseParse
	{
	private:
		ParseVariable* _assignment;
		ParseCondition* _condition;
		DataTypes	_dataType;

		std::vector<const BaseParse*> _list;

	public:
		ParseExpression(const std::wstring& line);
		virtual ~ParseExpression();

		void simplify() override;

		void addParse(BaseParse* parse);
		void removeLastParse();

		void clearList();
		void setAssignment(ParseVariable* variable);
		void setCondition(ParseCondition* condition);
		void setDataType(DataTypes dt);

		DataTypes dataType() const override;
		ParseVariable* returnValue() const;
		ParseVariable* assignment() const;
		ParseCondition* condition() const;
		const std::vector<const BaseParse*> &list() const;
		size_t size() const;
		bool isComparison() const;

		unsigned int lineCount() const;

	private:
		bool _isComparison(const std::vector<const BaseParse*>& list) const;
	};

}

