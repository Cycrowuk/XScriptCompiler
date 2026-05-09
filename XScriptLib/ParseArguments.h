#pragma once

#include "BaseParse.h"

namespace XScript
{
	class ParseBrackets;
	class ParseFail;
	class ParseSymbol;
	class ParseArguments : public BaseParse
	{
	protected:
		std::vector<BaseParse*> _list;

	public:
		ParseArguments(const std::wstring& l);
		virtual ~ParseArguments();

		void simplify() override;

		void addParse(BaseParse* parse);
		void insertParse(BaseParse* parse);
		void clear();

		ParseFail *addArguments(ParseBrackets* brackets);

		size_t count() const;
		const BaseParse* get(size_t i) const;
		const std::vector<BaseParse*>& list() const;
		std::vector<const BaseParse*> constList() const;

	private:
		ParseFail* _processList(std::vector<BaseParse*>& list, ParseSymbol* comma);
	};

}

