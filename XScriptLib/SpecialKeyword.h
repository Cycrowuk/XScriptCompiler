#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseSpecialKeyword : public BaseParse
	{
	private:
		SpecialKeywords _keyword;
		std::wstring _str;

	public:
		ParseSpecialKeyword(const std::wstring& line, const std::wstring& str, SpecialKeywords keyword);

		std::wstring stringData() const override;

		const std::wstring& keyword() const;
		const SpecialKeywords specialType() const;
	};

}

