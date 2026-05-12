#include "pch.h"
#include "BaseParse.h"
#include "CScriptData.h"

using namespace XScript;

#ifdef _DEBUG
int BaseParse::_DEBUG_COUNT = 0;
#endif

BaseParse::BaseParse(const std::wstring& l, ParseType type_) : 
	_startPos(0), 
	_lastPos(0), 
	_linePos(0),
	_type(type_), 
	_line(l),
	_data(l),
	_whitespaceAfter(false),
	_whitespaceBefore(false),
	_parDef(ParDef::Unknown)
{
#ifdef _DEBUG
	_DEBUG_COUNT++;
#endif
}

BaseParse::~BaseParse()
{
#ifdef _DEBUG
	_DEBUG_COUNT--;
#endif
}

void BaseParse::setData(const std::wstring& data)
{
	_data = data;
}
void BaseParse::setPosition(size_t start, size_t end)
{
	_startPos = start;
	_lastPos = end;
}

void BaseParse::setLinePosition(size_t pos)
{
	_linePos = pos;
}

void BaseParse::setFile(const std::wstring& file)
{
	_file = file;
}


void BaseParse::setFromParse(const BaseParse* parse)
{
	_data = parse->data();
	_startPos = parse->startingPos();
	_lastPos = parse->endingPos();
	_linePos = parse->linePos();
	_line = parse->line();
	_parDef = parse->pardef();
	_whitespaceAfter = parse->hasWhitespaceAfter();
	_whitespaceBefore = parse->hasWhitespaceBefore();
	_file = parse->file();
}

void BaseParse::setWhitespaceBefore(bool b)
{
	_whitespaceBefore = b;
}
void BaseParse::setWhitespaceAfter(bool b)
{
	_whitespaceAfter = b;
}

void BaseParse::setParDef(ParDef pardef)
{
	_parDef = pardef;
}

ParDef BaseParse::pardef() const
{
	return _parDef;
}

size_t BaseParse::startingPos() const
{
	return _startPos;
}
size_t BaseParse::endingPos() const
{
	return _lastPos;
}

size_t BaseParse::linePos() const
{
	return _linePos;
}

bool BaseParse::hasWhitespaceBefore() const
{
	return _whitespaceBefore;
}
bool BaseParse::hasWhitespaceAfter() const
{
	return _whitespaceAfter;
}

DataTypes BaseParse::dataType() const
{
	return DataTypes::Unknown;
}

std::wstring BaseParse::stringData() const
{
	return _line;
}

unsigned int BaseParse::lineCount() const
{
	return 0;
}

void BaseParse::simplify()
{

}

const std::wstring& BaseParse::data() const
{
	return _data;
}
const std::wstring& BaseParse::line() const
{
	return _line;
}
const std::wstring& BaseParse::file() const
{
	return _file;
}

ParseType BaseParse::type() const
{
	return _type;
}
