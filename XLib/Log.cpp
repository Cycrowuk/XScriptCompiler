#include "pch.h"
#include "log.h"

using namespace XLib;

CLog *CLog::m_pInstance = 0;

CLog::CLog() : _iLevel(0), _iFilter(0), _bSaveLog(0)
{
}

CLog::~CLog()
{
}

CLog *CLog::create()
{
	if ( !m_pInstance ) {
		m_pInstance = new CLog();
	}
	return m_pInstance;
}

void CLog::release()
{
	if ( m_pInstance ) {
		delete m_pInstance;
	}
	m_pInstance = 0;
}

void CLog::log(LogType iType, int iLevel, const XLib::String &sLogText)
{
	CLog *pLogger = CLog::create();
	pLogger->_log(iType, iLevel, sLogText);
}

void CLog::addFilter(int iType)
{
	_iFilter |= iType;
}

void CLog::setFilter(int iFilter)
{
	_iFilter = iFilter;
}

void CLog::removeFilter(int iType)
{
	_iFilter &= ~(iType);
}

void CLog::clearFilter()
{
	_iFilter = 0;
}

void CLog::displayLog(LogType iType, int iLevel, const XLib::String &sLogText) const
{
}

void CLog::_log(LogType iType, int iLevel, const XLib::String &sLogText)
{
	if ( iLevel > _iLevel ) return;
	if ( !(_iFilter & static_cast<int>(iType)) ) return;

	if ( _bSaveLog ) {
		_lLogs.push_back({ sLogText, iLevel, static_cast<int>(iType) });
	}

	this->displayLog(iType, iLevel, sLogText);
}

void CLog::logf(LogType iType, int iLevel, const char *sLogText, ...)
{
	char buffer[10000];
	va_list args;
	va_start (args, sLogText);
#ifdef _WIN32
	vsprintf_s(buffer, sLogText, args);
#else
	vsprintf(buffer, sLogText, args);
#endif
	va_end (args);

	CLog::log(iType, iLevel, buffer);
}
void CLog::_logf(LogType iType, int iLevel, const char *sLogText, ...)
{
	char buffer[10000];
	va_list args;
	va_start (args, sLogText);
#ifdef _WIN32
	vsprintf_s(buffer, sLogText, args);
#else
	vsprintf(buffer, sLogText, args);
#endif
	va_end (args);

	this->_log(iType, iLevel, buffer);
}

void CLog::setLevel(int iLevel)
{
	_iLevel = iLevel;
}

void CLog::clear()
{
	_lLogs.clear();
}
	
const SLog *CLog::firstLog() const
{
	if (_lLogs.empty()) return NULL;
	return &_lLogs.front();
}

size_t CLog::count() const
{
	return _lLogs.size();
}

/////////////////////////////////////////////////////////////////////////////
//	File Log
///////////////////

void CFileLog::_writeLog(LogType iType, const XLib::String &sLogText) const
{
	std::wofstream outFile;
	outFile.open(_sFilename, std::ios::out | std::ios::app);
	if ( outFile.is_open() ) {
		outFile << _timeStamp() << " (" << _typeName(iType) << ") " << sLogText << std::endl;
		outFile.close();
	}
}

XLib::String CFileLog::_timeStamp() const
{
	std::wstringstream strm;
	time_t T = time(NULL);
#ifdef _WIN32
	struct tm TM;
	localtime_s(&TM, &T);
	strm << "[" << XLib::String::PadNumber(TM.tm_mday, 2) << "/" << XLib::String::PadNumber(TM.tm_mon + 1, 2) << "/" << TM.tm_year + 1900 << " - " << XLib::String::PadNumber(TM.tm_hour, 2) << ":" << XLib::String::PadNumber(TM.tm_min, 2) << ":" << XLib::String::PadNumber(TM.tm_sec, 2) << "]";
#else
	struct tm* TM = localtime(&T);
	strm << "[" << XLib::String::PadNumber(TM->tm_mday, 2) << "/" << XLib::String::PadNumber(TM->tm_mon + 1, 2) << "/" << TM->tm_year + 1900 << " - " << XLib::String::PadNumber(TM->tm_hour, 2) << ":" << XLib::String::PadNumber(TM->tm_min, 2) << ":" << XLib::String::PadNumber(TM->tm_sec, 2) << "]";
#endif
	return strm.str();
}

XLib::String CFileLog::_typeName(LogType iType) const
{
	switch(iType) {
	case LogType::Install:			return "INSTALL";
		case LogType::Uninstall:	return "UNINSTALL";
		case LogType::IO:			return "IO";
		case LogType::File:			return "FILE";
		case LogType::Directory:	return "DIRECTORY";
	}
	return "";
}
