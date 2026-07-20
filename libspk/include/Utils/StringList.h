#pragma once

#include "String.h"
#include "../lists.h"
#include "List.h"

namespace Utils {

	typedef struct {
		SString str;
		SString data;
	} SStringList;

	typedef CListNode<SStringList> CStringListNode;
	typedef CList<SStringList>::iterator CStringListIterator;

	class SPKEXPORT CStringList
	{
	private:
		CList<SStringList> *_lList;

	public:
		CStringList();
		~CStringList();

		void pushBack(const SString &str, const SString &data);
		void pushBack(const SString &str);
		void pushFront(const SString &str, const SString &data);
		void pushFront(const SString &str);
		void popBack();
		void popFront();

		void insertAt(int at, const SString& str, const SString& data);
		void insertAt(int at, const SString& str);

		void clear();
		void tokenise(const SString &str, const SString &token);

		Utils::SString firstString();
		Utils::SString nextString();
		Utils::SStringList *first();
		Utils::SStringList *next();
		CStringListIterator begin() const;
		CStringListIterator end() const;

		Utils::SString front() const;
		Utils::SString back() const;

		Utils::SStringList *get(int i) const;

		bool changeData(const Utils::SString &str, const Utils::SString &data, bool bIgnoreCase = false);
		bool contains(const Utils::SString &data, bool bIgnoreCase = false) const;
		bool containsData(const Utils::SString& str, bool bIgnoreCase = false) const;
		bool containsStringAndData(const Utils::SString& str, const Utils::SString &data, bool bIgnoreCase = false) const;
		Utils::SString findData(const Utils::SString &data, bool bIgnoreCase = false) const;
		Utils::SString findString(const Utils::SString &str, bool bIgnoreCase = false) const;
		int findStringAndData(const Utils::SString& str, const Utils::SString& data, bool bIgnoreCase = false) const;
		int findPos(const Utils::SString& str, bool bIgnoreCase = false) const;

		CStringListIterator remove(CStringListIterator itr);
		bool remove(const Utils::SString &str, bool single = true);
		void removeAt(int at);

		size_t size() const;
		bool empty() const;

		const Utils::SStringList* operator[](int num) const;
	};

} //NAMESPACE

