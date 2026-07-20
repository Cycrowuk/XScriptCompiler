#pragma once

#include "lists.h"
#include "Utils/WStringList.h"
#include "enums.h"

class C_File;
class CBaseFile;
class CProgressInfo;

namespace SPK {

class COriginalFiles
{
private:
	CLinkList<C_File>		_lFiles;

	Utils::WString			_sDir;

public:
	COriginalFiles(const Utils::WString &dir);
	~COriginalFiles(void);

	int count() const;
	bool isOriginal(C_File *f) const;

	void installed(CBaseFile *package);
	void backup(CBaseFile *package, Utils::WStringList *errors);
	bool backupFile(C_File *f, Utils::WStringList *errors);
	bool doBackup(C_File *f, Utils::WStringList *errors);
	int  restoreAll(CProgressInfo *info, int files, int max);
	bool restoreFile(C_File *f, Utils::WStringList *errors);

	void parse(const Utils::WString &data);
	void update(bool bForce, const CLinkList<C_File> *pFiles);
	bool writeData(Utils::WStringList &lines);

	void reset();

private:
	void _storeOverride(C_File *f);
	void _storeFiles(FileType filetype, const Utils::WString &searchPath, const CLinkList<C_File> *pFiles);
	void _add(FileType filetype, const Utils::WString &filename, const Utils::WString &searchPath, const CLinkList<C_File> *pFiles);
	C_File *_getFile(C_File *file) const;
};

}