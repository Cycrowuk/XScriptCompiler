#include "pch.h"
#include "ParseLabel.h"
#include "ParseKeyword.h"

using namespace XScript;

ParseLabel::ParseLabel(const ParseKeyword* keyword) : BaseParse(keyword->line(), ParseType::Label)
{
	this->setFromParse(keyword);
	_label = keyword->keyword();
}

const std::wstring& ParseLabel::label() const
{
	return _label;
}
