#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseNull : public BaseParse
	{
	private:

	public:
		ParseNull(const std::wstring& line);

		DataTypes dataType() const override;
		std::wstring stringData() const override;
	};

}

