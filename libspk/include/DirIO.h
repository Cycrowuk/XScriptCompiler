#ifndef __DIRIO_H__
#define __DIRIO_H__

#include "Utils/WStringList.h"
#include "spkdll.h"

class CFileIO;

class SPKEXPORT CDirIO
{
public:
	static bool Exists(const Utils::WString &dir);
	static bool IsEmptyDir(const Utils::WStringList& dirList);

public:
	CDirIO();
	CDirIO(const Utils::WString &dir);
	CDirIO(CFileIO *file);
	~CDirIO();

	void setDir(const Utils::WString& dir);

	// access functions
	bool exists() const;
	bool exists(const Utils::WString &dir) const;
	bool isDir() const;
	bool isDir(const Utils::WString &dir) const;
	bool isFile() const;
	bool isFile(const Utils::WString &dir) const;

	// directory handling
	bool create() const;
	bool create(const Utils::WString &dir) const;
	bool createAndChange(const Utils::WString &dir);

	bool move(const Utils::WString &to);
	bool move(const Utils::WString& from, const Utils::WString& to);
	bool removeDir(const Utils::WString &dir, bool doFiles = false, bool recursive = false, Utils::WStringList* errors = NULL);
	bool cd(const Utils::WString &dir);

	bool dirList(Utils::WStringList& files, const Utils::WString &dir = Utils::WString::Null(), const Utils::WString &filePattern = Utils::WString::Null(), bool absolutePath = false) const;
	bool checkEmptyDir(const Utils::WStringList& dirList) const;

	Utils::WString file(const Utils::WString &filename) const;
	Utils::WString dir(const Utils::WString &sDir) const;
	const Utils::WString &dir() const;
	Utils::WString topDir() const;
	const Utils::WString &moveBack();
	Utils::WString back() const;
	Utils::WString relativePath(const Utils::WString& filename) const;

private:
	Utils::WString _parseDir(const Utils::WString &dir) const;

	Utils::WString m_sCurrentDir;
};

#endif //__DIRIO_H__
