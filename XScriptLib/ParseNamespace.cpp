#include "pch.h"
#include "ParseNamespace.h"

using namespace XScript;

ParseNamespace::ParseNamespace(const std::wstring& line) : BaseParse(line, ParseType::Namespace)
{

}

const std::wstring& ParseNamespace::namespaceString() const
{
	return _namespace;
}

const std::wstring& ParseNamespace::keyword() const
{
	return _keyword;
}

void ParseNamespace::setNamespace(const std::wstring& str)
{
	_namespace = str;
	std::wstringstream strm;
	strm << _namespace << L"::" << _keyword;
	_data = strm.str();
}

void ParseNamespace::setKeyword(const std::wstring& str)
{
	_keyword = str;
}
