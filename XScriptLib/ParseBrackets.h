#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseBrackets : public BaseParse
	{
	private:
		std::vector<BaseParse*> _list;

	public:
		ParseBrackets(const std::wstring& line);
		virtual ~ParseBrackets();

		void simplify() override;

		unsigned int lineCount() const override;

		void addParse(BaseParse* parse);
		void clear();

		size_t size() const;
		const std::vector<BaseParse*>& list() const;
		std::vector<const BaseParse*> constList() const;
		BaseParse* singleItem() const;
	};

}
