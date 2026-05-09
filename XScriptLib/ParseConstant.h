#pragma once

#include "BaseParse.h"

namespace XScript
{
	class ConstantData;
	class ParseConstant : public BaseParse
	{
	private:
		const ConstantData* _constant;

	public:
		ParseConstant(const std::wstring& l, const ConstantData* c);
		virtual ~ParseConstant();

		ConstGroup* constGroup() const;

		unsigned int id() const;
		DataTypes subType() const;
		DataTypes dataType() const override;
		std::wstring stringData() const override;
	};

}

