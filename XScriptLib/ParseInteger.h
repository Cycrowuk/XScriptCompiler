#pragma once

#include "BaseParse.h"

namespace XScript
{
	class ParseInteger : public BaseParse
	{
	private:
		int		_value;

	public:
		ParseInteger(const std::wstring& line, int i);
		~ParseInteger();

		DataTypes dataType() const override;
		std::wstring stringData() const override;

		int value() const;
	};

}

