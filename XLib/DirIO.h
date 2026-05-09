#ifndef __DIRIO_H__
#define __DIRIO_H__

#include "String.h"

namespace XLib
{
	class FileIO;
	class SPKEXPORT DirIO
	{
	public:
		static bool Exists(const XLib::String& dir);

	public:
		DirIO();
		DirIO(const XLib::String &dir);
		DirIO(const FileIO &file);
		~DirIO();

		void setDir(const XLib::String &dir);

		// access functions
		bool exists() const;
		bool exists(const XLib::String& dir) const;
		bool isDir() const;
		bool isDir(const XLib::String& dir) const;
		bool isFile() const;
		bool isFile(const XLib::String& dir) const;

		// directory handling
		bool create() const;
		bool create(const XLib::String& dir) const;
		bool createAndChange(const XLib::String &dir);

		bool move(const XLib::String& to);
		bool move(const XLib::String& from, const XLib::String& to);
		bool removeDir(const XLib::String &dir, bool doFiles = false, bool recursive = false, StringList *error = NULL);
		bool cd(const XLib::String& dir);

		bool dirList(StringList& files, XLib::String dir = XLib::String::Null(), XLib::String filePattern = XLib::String::Null()) const;
		StringList dirList(XLib::String dir = XLib::String::Null(), XLib::String filePattern = XLib::String::Null()) const;
		bool isEmptyDir(const StringList &dirList) const;

		XLib::String file(const XLib::String& filename) const;
		XLib::String dir(const XLib::String& sDir) const;
		const XLib::String& dir() const;
		XLib::String topDir() const;
		const XLib::String& moveBack();
		XLib::String back() const;

	private:
		XLib::String _parseDir(const XLib::String& dir) const;

		XLib::String m_sCurrentDir;
	};

}
#endif //__DIRIO_H__
