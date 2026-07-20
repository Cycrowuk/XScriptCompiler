#pragma once

#include "VirtualFileSystem.h"
#include "lists.h"

using namespace SPK;

class CCatFile;

enum {DIFFTYPE_ADDITION, DIFFTYPE_REMOVAL, DIFFTYPE_CHANGE};

tstruct SDiffEntry {
	int			iType;
	int			iID;
} SDiffEntry;

tstruct SDiffEntryAddition : SDiffEntry {
	Utils::WString	sEntry;
	SDiffEntryAddition() { iType = DIFFTYPE_ADDITION; }
} SDiffEntryAddition;

tstruct SDiffEntryRemoval : SDiffEntry {
	SDiffEntryRemoval() { iType = DIFFTYPE_REMOVAL; }
} SDiffEntryRemoval;

tstruct SDiffEntryChange : SDiffEntry {
	SDiffEntryChange() : iPos(0) { iType = DIFFTYPE_CHANGE; }
	int			iPos;
	Utils::WString	sFrom;
	Utils::WString	sEntry;
} SDiffEntryChange;

tstruct SDiffFile {
	Utils::WString sFile;
	CLinkList<SDiffEntry> m_lEntries;
} SDiffFile;

enum {MDERR_NONE, MDERR_FILENOTFOUND, MDERR_CANTOPEN, MDERR_CANTOPENMOD};

tclass CModDiff
{
private:
	enum {
		MERGETYPE_NONE,
		MERGETYPE_TSHIPS,
	};

public:
	// static
	static bool CanBeDiffed(const Utils::WString &file);

	// constructors
	CModDiff(const Utils::WString &dir, const Utils::WString &sAddon, int maxPatch = 0);
	~CModDiff(void);

	// public functions
	void SetTempDirectory(const Utils::WString &temp) { m_sTempDir = temp; }
	bool LoadDirectory(const Utils::WString &dir);
	bool IsLoaded() { return m_bLoaded; }
	const Utils::WString &GetDirectory() const { return m_sCurrentDir; }
	bool CreateDiff(const Utils::WString &mod);
	bool startDiff(const Utils::WString &sModFile);
	bool doDiff(const Utils::WString &sModFile);
	SDiffFile *diffFile(const Utils::WString &baseFile, const  Utils::WString &modFile, const Utils::WString &fileType);
	void Clean();
	bool WriteDiff(const Utils::WString &file);
	bool ReadDiff(const Utils::WString &file);
	bool ApplyDiff(const Utils::WString &mod);
	bool ApplyMod(const Utils::WString &mod);
	void SetMaxPatch(int patch) { m_iMaxPatch = patch; }

	CLinkList<SDiffFile> &GetDiffFiles() { return m_lFiles; }

	void ClearError() { m_iError = MDERR_NONE; }
	int Error() { return m_iError; }

private:
	// private functions
	int _amountPosition(const Utils::WString &fileType);
	bool _isLineComplete(const Utils::WString &line, const Utils::WString &fileType, bool first);
	bool _readGameFile(const Utils::WString &file, Utils::WStringList &lines, int *id);
	void _compareLine(const Utils::WString &line1, const Utils::WString &line2, int type, int id, SDiffFile *diffFile);
	bool _validFile(const Utils::WString &file);

	void _adjustFile(const Utils::WString &sFile, SDiffFile *pDiff, bool bReverse);
	bool _adjustTShips(SDiffFile *pDiff, bool bReserve);
	int _specialType(const Utils::WString &sFile) const;
	Utils::WString _extractFile(const Utils::WString &sFile, const Utils::WString &sTo);

	// Variables
	Utils::WString m_sAddon;
	Utils::WString m_sCurrentDir;		// the current game directory (that the VFS is opened too)
	Utils::WString m_sTempDir;				// temporary dir (used to write temporary files to)

	CVirtualFileSystem m_fileSystem;	// the VFS of the game directory (for finding the files to use)
	CLinkList<SDiffFile> m_lFiles;		// list of files that have changed

	bool	m_bLoaded;					// if the directory is loaded and ready to be diffed
	int		m_iError;					// error id of process
	int		m_iMaxPatch;				// The max fake patch to check to

	CCatFile		*m_pCatFile;
};
