#pragma once

#include "rapidxml\rapidxml.hpp"
#include <map>

namespace XLib
{

	class TextDB
	{
	public:
		typedef std::map<unsigned int, std::wstring> TextList;
		struct Page
		{
			TextList texts;
			std::wstring title;
			std::wstring desc;
			unsigned int game;
			bool voice;
		};
		typedef std::map<unsigned int, Page> PageList;
		typedef std::map<unsigned int, PageList> LanguageList;

	private:
		LanguageList _texts;
		unsigned int _language;

	public:
		TextDB();
		~TextDB();

		void setLanguage(unsigned int lang);
		unsigned int language() const;

		bool loadTextFile(const std::wstring& file, unsigned int gameVersion);
		void finalise();
		
		std::wstring get(unsigned int page, unsigned int text) const;
		std::wstring exists(unsigned int page, unsigned int text) const;

	private:
		std::wstring _parseText(unsigned int language, unsigned int pageid, const std::wstring& text) const;
		std::wstring _getText(unsigned int language, unsigned int pageid, unsigned int textid) const;
	};

}

