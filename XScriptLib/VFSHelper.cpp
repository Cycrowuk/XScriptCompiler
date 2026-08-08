// VFSHelper.cpp — VirtualFileSystem wrapper
// Include only what we actually need from libspk, not the full spk.h umbrella.
// spk.h pulls in Packages.h which references unrar — we don't need that.

#include "pch.h"
#include "File.h"
#include "File_IO.h"
#include "CatFile.h"
#include "VirtualFileSystem.h"
#include "VFSHelper.h"

static SPK::CVirtualFileSystem* s_vfs = nullptr;

void VFSHelper_SetDir(const wchar_t* dir, const wchar_t* mod)
{
	if (!s_vfs)
		s_vfs = new SPK::CVirtualFileSystem();

	if (s_vfs->LoadFilesystem(Utils::WString(dir)))
	{
		if (mod && mod[0])
			s_vfs->loadMod(Utils::WString(mod));
	}
}

void VFSHelper_SetAddon(const wchar_t* addon)
{
	if (s_vfs && addon && addon[0])
		s_vfs->setAddon(Utils::WString(addon));
}

bool VFSHelper_IsLoaded()
{
	return s_vfs != nullptr;
}

std::wstring VFSHelper_ExtractFile(const wchar_t* vfsPath, const wchar_t* tempPath)
{
	if (!s_vfs) return L"";
	Utils::WString result = s_vfs->extractGameFile(
		Utils::WString(vfsPath),
		Utils::WString(tempPath));
	if (result.empty()) return L"";
	return result.toStdWString();
}

std::wstring VFSHelper_FindText(int lang, int page, int id)
{
	if (!s_vfs) return L"";
	Utils::WString result = s_vfs->findText(lang, page, id);
	return result.empty() ? L"" : result.toStdWString();
}

bool VFSHelper_ReadFile(const wchar_t* path, std::vector<wchar_t>& outBuffer)
{
	Utils::WString wpath(path);
	CFileIO f(wpath);
	if (!f.exists()) return false;

	if (f.isFileExtension(L"pck"))
	{
		f.startRead();
		size_t fileSize;
		unsigned char* fileData = f.readAll(&fileSize);
		f.close();
		if (!fileData || !fileSize) { delete[] fileData; return false; }

		size_t unpackedSize;
		unsigned char* unpacked = UnPCKData(fileData, fileSize, &unpackedSize);
		delete[] fileData;
		if (!unpacked || !unpackedSize) { delete[] unpacked; return false; }

		// Convert to wchar_t buffer for rapidxml
		outBuffer.resize(unpackedSize + 1);
		for (size_t i = 0; i < unpackedSize; i++)
			outBuffer[i] = static_cast<wchar_t>(unpacked[i]);
		outBuffer[unpackedSize] = L'\0';
		delete[] unpacked;
	}
	else
	{
		// Plain XML — read as wide string via standard stream
		std::wifstream in(path);
		if (!in) return false;
		outBuffer.assign(std::istreambuf_iterator<wchar_t>(in), std::istreambuf_iterator<wchar_t>());
		outBuffer.push_back(L'\0');
	}
	return true;
}

void VFSHelper_CompressPck(const wchar_t* path)
{
	Utils::WString wpath(path);
	CFileIO f(wpath);
	if (!f.exists() || !f.isFileExtension(L"pck")) return;

	f.startRead();
	size_t size;
	unsigned char* readData = f.readAll(&size);
	if (!readData) return;

	size_t newSize;
	unsigned char* data = PCKData(readData, size, &newSize, false);
	delete[] readData;

	if (data)
	{
		f.close();
		CFileIO fWrite(wpath);
		fWrite.startWrite();
		fWrite.write((const char*)data, newSize);
		fWrite.close();
		delete[] data;
	}
}
