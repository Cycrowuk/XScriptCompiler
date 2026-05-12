#include "pch.h"
#include "TextDB.h"

#include <sstream>
#include <fstream>
#include <vector>

using namespace XLib;

TextDB::TextDB() : _language(44)
{

}

TextDB::~TextDB()
{

}

void TextDB::setLanguage(unsigned int lang)
{
	_language = lang;
}
unsigned int TextDB::language() const
{
	return _language;
}

bool TextDB::loadTextFile(const std::wstring& file, unsigned int gameVersion)
{
	std::wifstream inFile(file);
	if (!inFile.good())
		return false;

	std::vector<wchar_t>* buffer = new std::vector<wchar_t>((std::istreambuf_iterator<wchar_t>(inFile)), std::istreambuf_iterator<wchar_t>());
	buffer->push_back('\0');

	rapidxml::xml_document<wchar_t>* doc = new rapidxml::xml_document<wchar_t>();
	doc->parse<0>(&(*buffer)[0]);

	rapidxml::xml_node<wchar_t>* root_node = doc->first_node(L"language");

	if (!root_node)
	{
		delete buffer;
		delete doc;
		return false;
	}

	unsigned int language = 44;
	{
		for (rapidxml::xml_attribute<wchar_t>* attr = root_node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::wstring name = attr->name();
			if (name == L"id")
				language = std::stoi(attr->value());
		}
	}

	PageList *pages = &_texts[language];

	for (rapidxml::xml_node<wchar_t>* page_node = root_node->first_node(L"page"); page_node; page_node = page_node->next_sibling())
	{
		unsigned int pageid = 0;
		std::wstring title, desc;
		bool voice = false;
		for (rapidxml::xml_attribute<wchar_t>* attr = page_node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::wstring name = attr->name();
			if (name == L"id")
				pageid = std::stoi(attr->value());
			else if (name == L"title")
				title = attr->value();
			else if (name == L"descr")
				desc = attr->value();
			else if (name == L"voice")
			{
				std::wstring v = attr->value();
				voice = (v == L"yes" || v == L"true");
			}
		}

		if (pageid)
		{
			unsigned int gameID = pageid / 10000;
			pageid %= 10000;

			if (gameVersion < gameID)
				continue;

			bool dontOverright = false;
			auto findItr = pages->find(pageid);
			if (findItr != pages->end())
			{
				if (findItr->second.game > gameID)
					dontOverright = true;
			}

			Page* page = &(*pages)[pageid];
			page->desc = desc;
			page->game = gameID;
			page->voice = voice;
			page->title = title;
			for (rapidxml::xml_node<wchar_t>* text_node = page_node->first_node(L"t"); text_node; text_node = text_node->next_sibling())
			{
				unsigned int id = 0;
				for (rapidxml::xml_attribute<wchar_t>* attr = text_node->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::wstring name = attr->name();
					if (name == L"id")
						id = std::stoi(attr->value());
				}

				if (dontOverright)
				{
					auto find = page->texts.find(id);
					if (find != page->texts.end())
						continue;
				}

				page->texts[id] = text_node->value();
			}
		}
	}

	delete buffer;
	delete doc;

	return true;
}

void TextDB::finalise()
{
	for (auto langItr = _texts.begin(); langItr != _texts.end(); langItr++)
	{
		for (auto pageItr = langItr->second.begin(); pageItr != langItr->second.end(); pageItr++)
		{
			for (auto textItr = pageItr->second.texts.begin(); textItr != pageItr->second.texts.end(); textItr++)
				pageItr->second.texts[textItr->first] = _parseText(langItr->first, pageItr->first, textItr->second);
		}
	}
}

std::wstring TextDB::get(unsigned int page, unsigned int text) const
{
	std::wstring str = _getText(_language, page, text);
	if (str.empty())
	{
		std::wstringstream strm;
		strm << "ReadText" << page << "-" << text;
		return strm.str();
	}

	return str;
}

std::wstring TextDB::exists(unsigned int pageid, unsigned int textid) const
{
	auto findLang = _texts.find(_language);
	if (findLang != _texts.end())
	{
		auto findPage = findLang->second.find(pageid);
		if (findPage != findLang->second.end())
		{
			auto findText = findPage->second.texts.find(textid);
			if (findText != findPage->second.texts.end())
				return findText->second;
		}
	}

	return L"";
}


std::wstring TextDB::_parseText(unsigned int language, unsigned int pageid, const std::wstring& text) const
{
	std::wstringstream strm;
	std::wstring::size_type startPos = 0;
	std::wstring::size_type pos = text.find_first_of(L"{", 0);
	while (pos != std::wstring::npos) 
	{
		strm << text.substr(startPos, pos - startPos);
		std::wstring::size_type endPos = text.find_first_of(L"}", pos);
		if (endPos != std::wstring::npos) 
		{
			std::wstring::size_type commaPos = text.find_first_of(L",", pos);
			if (commaPos != std::wstring::npos) 
			{
				std::wstring strPage = text.substr(pos + 1, commaPos - pos - 1);
				std::wstring strText = text.substr(commaPos + 1, endPos - commaPos - 1);
				try
				{
					unsigned int page = strPage.empty() ? pageid : std::stoi(strPage);
					unsigned int textid = std::stoi(strText);
					std::wstring replacement = _getText(language, page, textid);
					if (replacement.empty())
						strm << text.substr(pos, endPos - pos + 1);
					else
						strm << replacement;
					startPos = endPos + 1;
				}
				catch (std::exception)
				{
					strm << text.substr(pos, endPos - pos + 1);
					startPos = endPos + 1;
				}
			}
			else
			{
				strm << text.substr(pos, endPos - pos + 1);
				startPos = endPos + 1;
			}

			pos = text.find_first_of(L"{", startPos);
			if (pos != std::wstring::npos) 
			{			
				strm << text.substr(startPos, pos - startPos);
				startPos = pos;
			}
		}
		else
		{
			strm << text.substr(startPos, text.length() - startPos);
			startPos = text.length();
			break;
		}
	}

	strm << text.substr(startPos, text.length() - startPos);

	return strm.str();
}

std::wstring TextDB::_getText(unsigned int language, unsigned int pageid, unsigned int textid) const
{
	auto findLang = _texts.find(language);
	if (findLang != _texts.end())
	{
		auto findPage = findLang->second.find(pageid);
		if (findPage != findLang->second.end())
		{
			auto findText = findPage->second.texts.find(textid);
			if (findText != findPage->second.texts.end())
				return _parseText(language, pageid, findText->second);
		}
	}

	return L"";
}
