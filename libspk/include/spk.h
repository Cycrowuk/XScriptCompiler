#ifndef __SPK_H__
#define __SPK_H__

#include "Logging/log.h"
#include "File.h"
#include "SpkFile.h"
#include "MultiSpkFile.h"
#include "CatFile.h"
#include "lists.h"
#include "File_IO.h"
#include "DirIO.h"
#include "XspFile.h"
#include "Packages.h"
#include "spkdef.h"
#include "ModDiff.h"
#include "VirtualFileSystem.h"
#include "GameDirectories.h"
#include "Languages.h"
#include "Utils/List.h"

//#include "StringList.h"

typedef int s_int;

#include "time.h"
#define ERRORLOG(n) Utils::WString::Number(n) + " " + Utils::WString::Number((long)time(NULL))

#define LIBRARYVERSION 2.52f

namespace SPK {
	void			SPKEXPORT AssignAutomaticFiletypes(const Utils::WStringList &list);
	s_int			SPKEXPORT GetAutomaticFiletype(const Utils::WString &file, Utils::WString *extradir, bool bUseSpecial);
	Utils::WString	SPKEXPORT GetSizeString ( unsigned long size );
	bool			SPKEXPORT WriteScriptStyleSheet(const Utils::WString &dest);
	Utils::WString	SPKEXPORT ConvertTimeString(time_t time);
	Utils::WString	SPKEXPORT FormatTextName(int id, int lang, bool newstyle);
};

#endif //__SPK_H__
