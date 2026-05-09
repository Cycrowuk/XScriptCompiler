#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseCondition : public BaseParse
	{
	private:
		Conditions _condition;
		bool _isBlock;
		unsigned int _blockCount;

	public:
		ParseCondition(const std::wstring& line, Conditions c);
		virtual ~ParseCondition();

		void setBlock(bool block);
		void setBlockCount(unsigned int count);

		bool isBlock() const;
		unsigned int blockCount() const;
		Conditions condition() const;
	};

};

