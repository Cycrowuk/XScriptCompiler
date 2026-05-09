#pragma once

#include <vector>
#include <stdarg.h>
#include <fstream>
#include <time.h>
#include <sstream>

#include "String.h"

namespace XLib
{
	typedef struct {
		XLib::String	sText;
		int				iLevel;
		int				iType;
	} SLog;

	class SPKEXPORT CLog
	{
	public:
		enum class LogType {
			All = -1,
			None = 0,
			Normal = 1,
			IO = 2,
			EditPackage = 4,
			File = 8,
			Read = 16,
			Install = 32,
			Uninstall = 64,
			Directory = 128,
		};

	protected:
		// private constructor
		CLog(void);
		virtual ~CLog(void);

	private:
		std::vector<SLog> _lLogs;
		int		_iLevel;
		int		_iFilter;
		bool	_bSaveLog;

	public:
		static CLog* m_pInstance;
		static CLog* create();
		static void release();

		static void log(LogType iType, int iLevel, const XLib::String& sLogText);
		static void logf(LogType iType, int iLevel, const char* sLogText, ...);
		void _log(LogType iType, int iLevel, const XLib::String& sLogText);
		void _logf(LogType iType, int iLevel, const char* sLogText, ...);

		virtual void displayLog(LogType iType, int iLevel, const XLib::String& sLogText) const;
		void setLevel(int iLevel);
		void addFilter(int iType);
		void setFilter(int iFilter);
		void removeFilter(int iType);
		void clearFilter();

		void clear();
		const SLog* firstLog() const;
		size_t count() const;
	};

	//TODO: move this
	class SPKEXPORT CConsoleLog : public CLog
	{
	public:
		static CLog* create()
		{
			if (!CLog::m_pInstance) {
				CLog::m_pInstance = new CConsoleLog();
			}
			return CLog::m_pInstance;
		}

		virtual ~CConsoleLog(void)
		{
		}
		virtual void displayLog(LogType iType, int iLevel, const XLib::String& sLogText) const
		{
			wprintf(L"%s\n", sLogText.c_str());
		}
	};

	class SPKEXPORT CFileLog : public CLog
	{
	private:
		XLib::String _sFilename;

	public:
		static CLog* create()
		{
			if (!CLog::m_pInstance) {
				CLog::m_pInstance = new CFileLog();
			}
			return CLog::m_pInstance;
		}

		void setFile(const XLib::String& sFileName)
		{
			_sFilename = sFileName;
		}

		virtual ~CFileLog(void)
		{
		}
		virtual void displayLog(LogType iType, int iLevel, const XLib::String& sLogText) const
		{
			_writeLog(iType, sLogText);
		}

	private:
		void _writeLog(LogType iType, const XLib::String& sLogText) const;
		XLib::String _timeStamp() const;
		XLib::String _typeName(LogType iType) const;
	};

}