#ifndef __MUTLISPKFILE_H__
#define __MUTLISPKFILE_H__

#include "SpkFile.h"

typedef struct SPKEXPORT SMultiSpkFile
{
	SMultiSpkFile() : bOn(false), iPos(0), pFile(nullptr) { iType = 0; bRemove = false; sData = 0; lSize = 0; }
	Utils::WString	sName;
	char   *sData;
	long    lSize;
	bool	bOn;
	Utils::WString  sScriptName;
	Utils::WString  sScriptVersion;
	Utils::WString  sScriptAuthor;
	CBaseFile *pFile;
	int		iType;
	int		iPos;
	bool	bRemove;
} SMultiSpkFile;

typedef struct SPKEXPORT SMultiHeader
{
	SMultiHeader() : fVersion(0.0f), iCompression(0), lUncomprLen(0), lComprLen(0), bSelection(false) { }
	float	fVersion;
	int		iCompression;
	long	lUncomprLen;
	long	lComprLen;
	bool	bSelection;
} SMultiHeader;

class SPKEXPORT CMultiSpkFile
{
public:
	CMultiSpkFile ()
	{
		m_bChanged = false;
	}
	~CMultiSpkFile ();

	SMultiSpkFile *addFileEntry(const Utils::WString &filename);
	bool addFile(SMultiSpkFile *file);
	bool addFile(const Utils::WString &file, bool on = true);
	bool addFileNow(const Utils::WString &file, bool = true );
	bool removeFile ( int id );
	bool removeFile(const SMultiSpkFile *ms);
	bool removeFile(const Utils::WString &file);
	bool markRemoveFile ( int id );
	bool markRemoveFile ( SMultiSpkFile *ms );
	bool markRemoveFile(const Utils::WString &file);
	void markRemoveAll();

	void ClearFiles ();
	void CreatePackage(SMultiSpkFile *p);

	bool readFile(const Utils::WString &filename, bool = true );
	bool ReadAllFilesToMemory (CProgressInfo *progress = NULL);
	bool ReadFileToMemory ( SMultiSpkFile *ms );
	bool readAllPackages(const Utils::WString &filename, int type = SPKREAD_NODATA, CLinkList<CBaseFile> *addToList = NULL ) { _sFilename = filename; return readAllPackages(type, addToList); }
	bool readAllPackages(int type = SPKREAD_NODATA, CLinkList<CBaseFile> *addToList = NULL );
	bool ReadSpk ( SMultiSpkFile *ms, int type = SPKREAD_NODATA );

	const SMultiSpkFile *findFile (const Utils::WString &name) const;

	bool extractAll(const Utils::WString &dir);
	bool extractData(SMultiSpkFile *ms);
	bool extractFile(const SMultiSpkFile *ms, const Utils::WString &dir);
	bool extractFile(const Utils::WString &filename, const Utils::WString &dir);
	bool splitMulti(const Utils::WString &filename, const Utils::WString &destdir);
	
	bool writeFile(const Utils::WString &filename, CProgressInfo *progress = NULL);

	void setName(const Utils::WString &n) { _sName = n; m_bChanged = true; }
	void setSelection ( bool on ) { m_SHeader.bSelection = on; m_bChanged = true; }
	void setCompression ( int c ) { m_SHeader.iCompression = c; m_bChanged = true; }
	void setFilename(const Utils::WString &s) { _sFilename = s; }
	void setChanged(bool b) { m_bChanged = b; }

	const Utils::WString name() const { return _sName; }
	const Utils::WString& filename() const { return _sFilename; }
	bool isSelection() const { return m_SHeader.bSelection; }
	bool isChanged() const { return m_bChanged; }

	size_t numFiles() const { return m_lFiles.size(); }
	unsigned long GetFileSize();
	unsigned int GetAvailableFiles();
	CLinkList<SMultiSpkFile> *GetFileList () { return &m_lFiles; }

	void UpdatedPackage(CBaseFile *p);
	SMultiSpkFile *findPackage(CBaseFile *p);
	SMultiSpkFile *findPackage(const Utils::WString &name, const Utils::WString &author);
	void RemovePackages();

protected:
	Utils::WString _createData() const;
	bool _parseHeader(const Utils::WString &header);
	bool _parseValueLine(const Utils::WString &line);
	void _readValues(const Utils::WString &values);

private:
	CLinkList<SMultiSpkFile> m_lFiles;
	Utils::WString _sName, _sFilename;

	SMultiHeader m_SHeader;
	bool		m_bChanged;
};

#endif //__MUTLISPKFILE_H__
