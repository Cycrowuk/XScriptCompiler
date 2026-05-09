#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseString : public BaseParse
	{
	private:
		std::wstring _str;

	public:
		ParseString(const std::wstring& l, const std::wstring& s);
		virtual ~ParseString();

		std::wstring stringData() const override;
		DataTypes dataType() const override;
	};

}
