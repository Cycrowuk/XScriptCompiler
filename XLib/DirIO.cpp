#include "pch.h"
#include "DirIO.h"

#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().
#include <fcntl.h>
#include "FileIO.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#define ACCESS _access
#else
#include <dirent.h>
#define ACCESS access
#endif

using namespace XLib;

/////////////////////////////////////////
// STATIC FUNCTIONS
bool DirIO::Exists(const XLib::String &dir)
{
	return DirIO(dir).exists();
}


////////////////////////////////////////
// Ctor/Dtor

DirIO::DirIO()
{
}

DirIO::~DirIO()
{
}

DirIO::DirIO(const XLib::String &dir)
{
	setDir(dir);
}

DirIO::DirIO(const FileIO &file)
{
	setDir(file.dir());
}

/////////////////////////////////////////////////////////
//

void DirIO::setDir(const XLib::String &dir)
{
	m_sCurrentDir = dir;
	m_sCurrentDir.toFilename();
}


bool DirIO::exists() const
{
	XLib::String dir = m_sCurrentDir;

	if (dir.empty())
		return false;

	if (ACCESS((const char *)dir, 0) == 0)
		return true;

	return false;
}

bool DirIO::exists(const XLib::String &dir) const
{
	XLib::String d = _parseDir(dir);

	if (d.empty())
		return false;

	if (ACCESS((const char *)d, 0) == 0)
		return true;

	return false;
}

XLib::String DirIO::_parseDir(const XLib::String &dir) const
{
	XLib::String sDir = dir.asFilename();
	if ( !m_sCurrentDir.empty() && !sDir.isin(":") )
	{
		if ( sDir.empty() )
			sDir = m_sCurrentDir;
		else
			sDir = m_sCurrentDir + "/" + sDir;
	}

	return sDir.asFilename();
}

bool DirIO::isDir(const XLib::String &sDir) const
{
	XLib::String dir = _parseDir(sDir);
	if (ACCESS(dir.n_str(), 0) != -1)
	{
		struct stat status;
		stat(dir.n_str(), &status);

		if (status.st_mode & S_IFDIR)
			return true;
	}
	else {
		return !dir.token("/", -1).isin(".");
	}

	return false;
}

bool DirIO::isDir() const
{
	return isDir(m_sCurrentDir);
}

bool DirIO::isFile() const
{
	return isFile(m_sCurrentDir);
}
bool DirIO::isFile(const XLib::String &sDir) const
{
	XLib::String dir = _parseDir(sDir);
	if (ACCESS(dir.n_str(), 0) != -1)
	{
		struct stat status;
		stat(dir.n_str(), &status);

		if (status.st_mode & S_IFDIR)
			return false;
		else
			return true;
	}
	else {
		return dir.token("/", -1).isin(".");
	}

	return false;
}

bool DirIO::create() const
{
	return create(XLib::String::Null());
}
bool DirIO::create(const XLib::String &sDir) const
{
	XLib::String dir = sDir;
	if ( dir.empty() )
		dir = m_sCurrentDir;
	dir = _parseDir(dir);

	// split up directorys
	StringList dirs = dir.findReplace( "/", "\\" ).findReplace( "\\\\", "\\" ).tokenise("\\");

	// check if full dir, or relative
	int start = 1;
	XLib::String curDir;
	
	if (!dirs.empty())
		curDir = dirs.front();

	if ( !curDir.isin(":") )
	{
		curDir = m_sCurrentDir;
		start = 0;
	}

	// process each dir
	for(auto itr = dirs.begin(); itr != dirs.end(); itr++)
	{
		if ( !curDir.empty() )
			curDir += "/";
		curDir += *itr;

		// check if the directory exists
		if ( !exists(curDir) )
		{
#ifdef _WIN32
			if ( _mkdir(curDir.n_str()) )
#else
			if ( mkdir(curDir.n_str(), 0755) )
#endif
			{
				return false;
			}
		}
	}

	return true;
}

bool DirIO::move(const XLib::String &sTo)
{
	XLib::String to = _parseDir(sTo);

	if (exists(to))
		return false;

	if (!rename(m_sCurrentDir.n_str(), to.n_str()))
	{
		m_sCurrentDir = to;
		return true;
	}

	return false;
}


bool DirIO::move(const XLib::String &sFrom, const XLib::String &sTo)
{
	if ( !exists(sTo) )
	{
		if ( !create(sTo) )
			return false;
	}

	if ( !rename(sFrom.n_str(), sTo.n_str()) )
		return true;

	return false;
}

bool DirIO::isEmptyDir(const StringList &dirList) const
{
	// not found any files, most likly empty
	if ( !dirList.size() )
		return true;
	
	// check for any valid files
	for (auto itr = dirList.begin(); itr != dirList.end(); itr++)
	{
		if (*itr == "." || *itr == "..")
			continue;
		return false;
	}

	return true;
}

bool DirIO::removeDir(const XLib::String &dir, bool doFiles, bool recursive, StringList *errors)
{
	StringList files;
	dirList(files, dir);
	if (isEmptyDir(files))
	{
		XLib::String remDir = _parseDir(dir);
#ifdef _WIN32
		if (_rmdir(remDir.n_str()) == 0)
#else
		if (rmdir(remDir.n_str()) == 0)
#endif
		{
			if (errors)
				errors->push_back(remDir);
		}
		return true;
	}

	// not empty
	if ( doFiles || recursive )
	{
		for (auto itr = files.begin(); itr != files.end(); itr++)
		{
			if (*itr == "." || *itr == "..")
				continue;

			XLib::String fullFile = (dir + "\\" + *itr);
			if (doFiles && isFile(fullFile))
			{
				XLib::String remFile = _parseDir(fullFile);
				if (remove(remFile.n_str()) == 0)
				{
					if (errors)
						errors->push_back(remFile);
				}
			}
			else if (recursive && isDir(fullFile))
				removeDir(fullFile, doFiles, recursive, errors);
		}
	}

	// now check if its empty
	files.clear();
	dirList(files, dir);
	if (isEmptyDir(files))
	{
		XLib::String remDir = _parseDir(dir);
		#ifdef _WIN32
		if ( _rmdir(remDir.n_str()) == 0 )
		#else
		if ( rmdir(remDir.n_str()) == 0 )
		#endif
		{
			if ( errors )
				errors->push_back(remDir);
		}
		return true;
	}

	return false;
}

StringList DirIO::dirList(XLib::String dir, XLib::String filePattern) const
{
	StringList files;
	dirList(files, dir, filePattern);
	return files;
}
bool DirIO::dirList(StringList &files, XLib::String dir, XLib::String filePattern) const
{
	dir = _parseDir(dir);
	if ( dir.empty() )
		return false;

	dir = dir.findReplace("\\", "/");
	if (filePattern.empty())
		dir += "/*";
	else
	{
		dir += "/";
		dir += filePattern;
	}
	dir = dir.findReplace("//", "/");

#ifdef _WIN32
	dir = dir.findReplace("/", "\\");

	WIN32_FIND_DATA data;
	TCHAR buf[5000];
	wsprintf(buf, L"%hs", dir.c_str());

	HANDLE h = FindFirstFile(buf, &data);
	if ( h != INVALID_HANDLE_VALUE)
	{
		std::wstring ws(data.cFileName);

		XLib::String checkFile(ws);
		if ( !checkFile.Compare(".") && !checkFile.Compare("..") )
			files.push_back(checkFile);

		while ( FindNextFile(h, &data) )
		{
			std::wstring ws(data.cFileName);
			XLib::String checkFile(ws);
			if ( checkFile != "." && checkFile != ".." )
				files.push_back(checkFile);
		}

		FindClose(h);
		return true;
	}
	return false;
#else
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(dir.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			XLib::String checkFile(ent->d_name);
			if (checkFile != "." && checkFile != "..")
				files.pushBack(checkFile);
		}
		closedir(dir);
	}
	return true;
#endif//_WIN32

}

XLib::String DirIO::file(const XLib::String &filename) const
{
	if (m_sCurrentDir.empty())
		return filename;

	return m_sCurrentDir + "/" + filename;
}

XLib::String DirIO::dir(const XLib::String &sDir) const
{
	return _parseDir(sDir);
}

const XLib::String &DirIO::dir() const
{
	return m_sCurrentDir;
}

bool DirIO::cd(const XLib::String &sDir)
{
	if ( m_sCurrentDir.empty() )
		m_sCurrentDir = sDir;
	else
		m_sCurrentDir = m_sCurrentDir + "/" + sDir;
	m_sCurrentDir = m_sCurrentDir.asFilename();

	return exists();
}

bool DirIO::createAndChange(const XLib::String &dir)
{
	if ( create(dir) )
		return cd(dir);
	return false;
}


XLib::String DirIO::topDir() const
{
	if ( m_sCurrentDir.empty() )
		return XLib::String("");

	return m_sCurrentDir.token("/", -1);
}

const XLib::String &DirIO::moveBack()
{
	m_sCurrentDir = m_sCurrentDir.tokens("/", 1, -2);
	return m_sCurrentDir;
}
XLib::String DirIO::back() const
{
	return m_sCurrentDir.tokens("/", 1, -2);
}
