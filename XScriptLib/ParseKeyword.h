#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseKeyword : public BaseParse
	{
	private:
		std::wstring	_str;

	public:
		ParseKeyword(const std::wstring& line, const std::wstring& str);
		virtual ~ParseKeyword();

		std::wstring stringData() const override;
		DataTypes dataType() const override;

		const std::wstring& keyword() const;
	};

}

