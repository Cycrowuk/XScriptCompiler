#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseKeyword;
	class ParseLabel : public BaseParse
	{
	private:
		std::wstring _label;

	public:
		ParseLabel(const ParseKeyword* keyword);

		const std::wstring& label() const;
	};

}

