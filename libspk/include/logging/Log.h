#pragma once

#include "../Utils/WStringList.h"
#include <vector>
#include <stdarg.h>
#include <fstream>
#include <time.h>
#include <sstream>

typedef struct {
	Utils::WString	sText;
	int				iLevel;
	int				iType;
} SLog;

class SPKEXPORT CLog
{
public:
	typedef CLinkList<SLog> LogList;
	typedef CListNode<SLog>* LogNode;

	enum LogType {
		Log_All			= -1,
		Log_None		= 0,
		Log_Normal		= 1,
		Log_IO			= 2,
		Log_EditPackage = 4,
		Log_File		= 8,
		Log_Read		= 16,
		Log_Install		= 32,
		Log_Uninstall	= 64,
		Log_Directory	= 128,
	};

protected:
	// private constructor
	CLog(void);
	virtual ~CLog(void);

private:
	LogList	m_lLogs;
	int		_iLevel;
	int		_iFilter;
	bool	_bSaveLog;

public:
	static CLog *m_pInstance;
	static CLog *create();
	static void release();

	static void log(LogType iType, int iLevel, const Utils::WString& sLogText);
	static void logf(LogType iType, int iLevel, const char* sLogText, ...);
	static void logf(LogType iType, int iLevel, const wchar_t* sLogText, ...);
	void _log(LogType iType, int iLevel, const Utils::WString& sLogText);
	void _logf(LogType iType, int iLevel, const char* sLogText, ...);
	void _logf(LogType iType, int iLevel, const wchar_t* sLogText, ...);

	virtual void displayLog(LogType iType, int iLevel, const Utils::WString &sLogText) const;
	void setLevel(int iLevel);
	void addFilter(int iType);
	void setFilter(int iFilter);
	void removeFilter(int iType);
	void clearFilter();

	void clear();
	const SLog *firstLog() const;
	int count() const;
};

//TODO: move this
class SPKEXPORT CConsoleLog : public CLog 
{
public:
	static CLog *create()
	{
		if ( !CLog::m_pInstance ) {
			CLog::m_pInstance = new CConsoleLog();
		}
		return CLog::m_pInstance;
	}

	virtual ~CConsoleLog(void)
	{
	}
	virtual void displayLog(LogType iType, int iLevel, const Utils::WString &sLogText) const
	{
		wprintf(L"%s\n", sLogText.c_str());
	}
};

class SPKEXPORT CFileLog : public CLog 
{
private:
	Utils::WString _sFilename;

public:
	static CLog *create()
	{
		if ( !CLog::m_pInstance ) {
			CLog::m_pInstance = new CFileLog();
		}
		return CLog::m_pInstance;
	}

	void setFile(const Utils::WString &sFileName)
	{
		_sFilename = sFileName;
	}

	virtual ~CFileLog(void)
	{
	}
	virtual void displayLog(LogType iType, int iLevel, const Utils::WString &sLogText) const
	{
		_writeLog(iType, sLogText);
	}

private:
	void _writeLog(int iType, const Utils::WString &sLogText) const;
	Utils::WString _timeStamp() const;
	Utils::WString _formatTime(int iValue) const;
	Utils::WString _typeName(int iType) const;
};
