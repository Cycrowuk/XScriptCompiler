#pragma once

#include "WString.h"
#include "../lists.h"
#include "List.h"

namespace Utils {

	typedef struct {
		WString str;
		WString data;
	} WStringNode;

	typedef CListNode<WStringNode> WStringListNode;
	typedef CList<WStringNode>::iterator WStringListIterator;

	class SPKEXPORT WStringList
	{
	private:
		CList<WStringNode> *_lList;

	public:
		WStringList();
		~WStringList();

		void pushBack(const WString& str, const WString& data);
		void pushBack(const WString& str);
		void pushBackUnique(const WString& str, const WString& data);
		void pushBackUnique(const WString& str);
		void pushFront(const WString &str, const WString &data);
		void pushFront(const WString &str);
		void popBack();
		void popFront();

		void insertAt(size_t at, const WString& str, const WString& data);
		void insertAt(size_t at, const WString& str);

		void clear();
		void tokenise(const WString &str, const WString &token);

		Utils::WString firstString();
		Utils::WString nextString();
		Utils::WStringNode *first();
		Utils::WStringNode *next();
		WStringListIterator begin() const;
		WStringListIterator end() const;

		Utils::WString front() const;
		Utils::WString back() const;

		Utils::WStringNode *get(size_t i) const;

		bool changeData(const Utils::WString &str, const Utils::WString &data, bool bIgnoreCase = false);
		bool contains(const Utils::WString &data, bool bIgnoreCase = false) const;
		bool containsData(const Utils::WString& str, bool bIgnoreCase = false) const;
		bool containsStringAndData(const Utils::WString& str, const Utils::WString &data, bool bIgnoreCase = false) const;
		Utils::WString findData(const Utils::WString &data, bool bIgnoreCase = false) const;
		Utils::WString findString(const Utils::WString &str, bool bIgnoreCase = false) const;
		long findStringAndData(const Utils::WString& str, const Utils::WString& data, bool bIgnoreCase = false) const;
		long findPos(const Utils::WString& str, bool bIgnoreCase = false) const;

		WStringListIterator remove(WStringListIterator itr);
		bool remove(const Utils::WString &str, bool single = true);
		void removeAt(size_t at);

		size_t size() const;
		bool empty() const;

		const Utils::WStringNode* operator[](size_t num) const;
		Utils::WStringNode* operator[](size_t num);
		const Utils::WStringNode* operator[](const WString& str) const;
		Utils::WStringNode* operator[](const WString& str);
	};
} //NAMESPACE

