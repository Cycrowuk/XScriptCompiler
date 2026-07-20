#ifndef __CATFILE_H__
#define __CATFILE_H__

#include "File.h"
#include "File_IO.h"
#include "lists.h"
#include "spkdll.h"

enum {CATERR_NONE, CATERR_NODATFILE, CATERR_NOCATFILE, CATERR_FILEEMPTY, CATERR_READCAT, CATERR_DECRYPT, CATERR_MISMATCH, CATERR_NOFILE, CATERR_CANTREAD, CATERR_CANTCREATEDIR, CATERR_INVALIDDEST,
		CATERR_CREATED, CATERR_MALLOC};
enum {CATFILE_NONE, CATFILE_READ, CATFILE_DECYPTED};
enum {CATREAD_JUSTCONTENTS, CATREAD_CAT, CATREAD_CATDECRYPT, CATREAD_DAT};

typedef struct SInCatFile {
	SInCatFile () { lSize = 0; sData = 0; lOffset = 0; bDelete = false; bDecrypted = false; }
	Utils::WString sFile;
	size_t lSize;
	unsigned char  *sData;
	size_t lOffset;
	bool	bDelete;
	bool	bDecrypted;
} SInCatFile;

unsigned char SPKEXPORT *PCKData ( unsigned char *data, size_t oldsize, size_t *newsize, bool bXor = true );

class SPKEXPORT CCatFile
{
public:
	enum CatFiletype {
		FILETYPE_PLAIN,
		FILETYPE_DEFLATE,
		FILETYPE_PCK,
	};

public:
	static Utils::WString PckChangeExtension(const Utils::WString &f);
	static Utils::WString RenameFileExtension(SInCatFile *f);
	static bool IsAddonDir(const Utils::WString &dir);
	static bool CheckPackedExtension(const Utils::WString &sFilename);

public:
	CCatFile ();
	~CCatFile ();

	int  open(const Utils::WString &catfile, const Utils::WString &addon, int readtype = CATREAD_CATDECRYPT, bool create = true);
	bool DecryptData ();
	bool DecryptData ( unsigned char *data, size_t size );
	void DecryptDAT(unsigned char *buffer, size_t size);
	void DecryptDAT(SInCatFile *pFile);
	bool readFiles ();
	void LoadDatFile ();

	bool checkExtensionPck(const Utils::WString &filename) const;

	unsigned char *readData(const Utils::WString &filename, size_t *size);
	unsigned char *readData(SInCatFile *c, size_t *size);
	SInCatFile *findData(const Utils::WString &filename) const;
	bool readFileToData(const Utils::WString &filename);
	bool readFileToData(SInCatFile *c);

	bool markRemoveFile(SInCatFile *f);
	bool markRemoveFile(const Utils::WString &filename);

	size_t endOffset() const;
	size_t GetNumFiles() const;
	SInCatFile *GetFile(size_t num) const;

	unsigned char *UnpackFile ( SInCatFile *c, size_t *size);

	static bool Opened(int error, bool allowCreate = true);
	bool removeFile(SInCatFile *f);
	bool removeFile(const Utils::WString &filename);
	bool WriteCatFile ();
	bool appendFile(const Utils::WString &sFilename, const Utils::WString &to, bool pck = true, bool bXor = true, Utils::WString *sChangeTo = NULL);
	bool appendData (unsigned char *data, size_t size, const Utils::WString &to, bool pck = true, bool bXor = true );
	bool addData(const Utils::WString &catfile, unsigned char *data, size_t size, const Utils::WString &to, bool pck = true, bool create = true );

	std::vector<SInCatFile *> *GetFiles() const;

	bool extractFile(const Utils::WString &filename, const Utils::WString &to = Utils::WString::Null(), bool preserve = false);
	bool extractFile(SInCatFile* f, const Utils::WString &to = Utils::WString::Null(), bool preserve = false);
	bool extractAll(const Utils::WString &dir);

	void clearError () { m_iError = CATERR_NONE; _sError.clear(); }
	int error () const { return m_iError; }
	const Utils::WString &errorString() const { return _sError; }
	Utils::WString getErrorString () const;

	const Utils::WString &internalDatFilename() const;
	const Utils::WString &catFilename() const { return m_fCatFile.fullFilename(); }
	const Utils::WString &datFilename() const { return m_fDatFile.fullFilename(); }

	bool writeFromCat(const Utils::WString &catfile, const Utils::WString &file);
	bool writeFromCat(CCatFile* fcat, const Utils::WString &file);

	void findFiles(Utils::WStringList &list, const Utils::WString &filemask) const;

private:
	void _clearFiles();
	int _checkFiletype(const unsigned char *pBuffer, int iSize);
	void RemoveData ();

	CFileIO m_fCatFile;
	CFileIO m_fDatFile;

	unsigned char *m_sData;
	size_t m_lSize;
	char m_iDataType;

	std::vector<SInCatFile *> *_lFiles;
	Utils::WString _sError;
	Utils::WString _sAddonDir;

	Utils::WString _sReadFilename;
	int m_iError;

	bool m_bCreate;
	bool m_bCatChanged;
};

#endif //__CATFILE_H__