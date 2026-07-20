#pragma once

#include "WString.h"
#include "WStringList.h"
#include "../File_IO.h"
#include "../DirIO.h"

#include <bitset>
#include <cwctype>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlobj.h>
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#endif

namespace Utils
{
	namespace fs = std::filesystem;

	class CommandLine
	{
	private:
		std::shared_ptr<CFileIO> _file;
		Utils::WStringList		_options;
		CDirIO 					_myDoc;
		CDirIO 					_tempDir;
		std::vector<Utils::WString> _args;
		std::bitset<256>		_flags{};
		CPackages				_p;

	public:
		explicit CommandLine(int argc, char* argv[], bool doFlags = false)
		{
			_file = std::make_shared<CFileIO>(argv[0]);

			for (int i = 1; i < argc; ++i)
			{
				Utils::WString arg(argv[i]);
				doCommandLine(std::move(arg), doFlags);
			}

			finaliseCommandLine();
		}

		explicit CommandLine(int argc, wchar_t* argv[], bool doFlags = false)
		{
			_file = std::make_shared<CFileIO>(argv[0]);

			for (int i = 1; i < argc; ++i)
			{
				Utils::WString arg(argv[i]);
				doCommandLine(std::move(arg), doFlags);
			}

			finaliseCommandLine();
		}

		const std::shared_ptr<CFileIO>& file() const noexcept
		{
			return _file;
		}

		const Utils::WString& cmdName() const noexcept
		{
			return _file->filename();
		}

		const Utils::WString& cmdDir() const noexcept
		{
			return _file->dir();
		}

		const Utils::WString& tempDir() const noexcept
		{
			return _tempDir.dir();
		}

		const Utils::WString& myDoc() const noexcept
		{
			return _myDoc.dir();
		}

		const Utils::WStringList& options() const noexcept
		{
			return _options;
		}

		const std::vector<Utils::WString>& args() const noexcept
		{
			return _args;
		}

		size_t argCount() const noexcept
		{
			return _args.size();
		}

		Utils::WString arg(size_t i) const
		{
			if (i >= _args.size())
				return Utils::WString::Null();
			return _args[i];
		}

		bool hasSwitch(const Utils::WString& s) const noexcept
		{
			return _options.contains(s);
		}

		bool hasFlag(unsigned char s) const noexcept
		{
			return _flags.test(s);
		}

		Utils::WString switchData(const Utils::WString& s, const Utils::WString& defaultValue = Utils::WString::Null()) const
		{
			if (_options.contains(s))
				return _options[s]->data;
			return defaultValue;
		}

		const CPackages& packages() const noexcept
		{
			return _p;
		}

		CPackages& packages() noexcept
		{
			return _p;
		}

		const CDirIO& dirIO() const noexcept
		{
			return _file->dirIO();
		}

		Utils::WString fullFilename(const Utils::WString& s) const
		{
			// Keep simple cross-platform detection: recognize drive/protocols and path separators.
			if (s.contains(L":") || s.contains(L"/") || s.contains(L"\\"))
				return s;
			return _file->dirIO().file(s);
		}

	private:
		void doCommandLine(Utils::WString arg, bool doFlags)
		{
			if (arg.startsWith(L"--"))
			{
				arg = arg.substr(2);

				if (arg.contains(L":"))
				{
					Utils::WString a = arg.token(L":", 1);
					Utils::WString b = arg.tokens(L":", 2);
					_options.pushBack(a, b);
				}
				else
				{
					_options.pushBack(arg);
				}
			}
			else
			{
				if (doFlags)
				{
					if (arg.startsWith(L"-") && !arg.startsWith(L"--"))
					{
						Utils::WString flags = arg.substr(1);
						for (size_t is = 0; is < flags.length(); ++is)
						{
							wchar_t ch = flags[static_cast<int>(is)];
							wchar_t upper = std::towupper(ch);

							if (upper >= L'A' && upper <= L'Z')
							{
								unsigned char idx = static_cast<unsigned char>(upper & 0xFF);
								_flags.set(idx);
							}
						}
						return;
					}
				}
				_args.push_back(arg);
			}
		}

		void finaliseCommandLine()
		{
			// Ensure command directory is a usable path; prefer std::filesystem current_path.
			const Utils::WString fileDir = _file->dir();
			bool looksLikePath = (!fileDir.empty() && (fileDir.contains(L"/") || fileDir.contains(L"\\") || fileDir.contains(L":")));
			if (_file->dir().empty() || !looksLikePath)
			{
				Utils::WString d;
				try
				{
					fs::path cwd = fs::current_path();
#ifdef _WIN32
					d = Utils::WString(cwd.wstring().c_str());
#else
					d = Utils::WString(cwd.string().c_str());
#endif
				}
				catch (...)
				{
					// fallback
#ifdef _WIN32
					d = Utils::WString(_getcwd(NULL, 0));
#else
					d = Utils::WString(getcwd(NULL, 0));
#endif
				}

				if (d.empty())
					d = L"./";
				_file->setDir(d);
			}

			// my documents
			Utils::WString myDoc = L".";

#ifdef _WIN32
			TCHAR pszPath[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, pszPath)))
			{
				myDoc = pszPath;
			}
#else
			// on POSIX try HOME, else leave '.',
			const char* home = std::getenv("HOME");
			if (home && *home)
			{
				myDoc = Utils::WString(home);
			}
#endif
			_myDoc.setDir(myDoc);

			// temp dir via filesystem (fallbacks included)
			Utils::WString tmp = L".";
			try
			{
				fs::path t = fs::temp_directory_path();
#ifdef _WIN32
				tmp = Utils::WString(t.wstring().c_str());
#else
				tmp = Utils::WString(t.string().c_str());
#endif
			}
			catch (...)
			{
#ifdef _WIN32
				TCHAR szPath[MAX_PATH + 1];
				DWORD result = GetTempPath(MAX_PATH + 1, szPath);
				if (result > 0)
					tmp = Utils::WString(szPath);
#else
				// leave as "."
#endif
			}
			_tempDir.setDir(tmp);

			_p.startup(L".", _tempDir.dir(), _myDoc.dir());
		}
	};
}