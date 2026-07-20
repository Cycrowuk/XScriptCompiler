#ifndef __BASE_ENGINE_STRINGLIST_H__
#define __BASE_ENGINE_STRINGLIST_H__

#include "Utils/String.h"
#include "CyString.h"
#include "spkdll.h"
#include "spkdef.h"
#include "lists.h"

typedef struct STRUCT_STRINGLIST {
	struct STRUCT_STRINGLIST *next;
	struct STRUCT_STRINGLIST *prev;
	CyString str;
	CyString data;
	bool remove;
} SStringList;

class SPKEXPORT CyStringList
{
public:
	CyStringList () { m_head = m_tail = NULL; m_iCount = 0; }
	~CyStringList ()
	{
		SStringList *node = m_head;
		while ( node )
		{
			SStringList *nextnode = node->next;
			delete node;
			node = nextnode;
		}
	}

	void SplitToken ( char token, CyString str )
	{
		int num = 0;
		CyString *s = str.SplitToken ( token, &num );

		for ( int i = 0; i < num; i++ )
			PushBack ( s[i] );

		CLEANSPLIT(s, num);
	}

	CyStringList &operator= ( CyStringList &list )
	{
		for ( SStringList *node = list.Head(); node; node = node->next )
		{
			if ( node->remove )
				continue;
                        PushBack ( node->str, node->data );
		}
		return (*this);
	}

	SStringList *Head () const { return m_head; }
	SStringList *Tail () const { return m_tail; }

	bool Empty() { if ( m_head ) return false; return true; }

	void Clear()
	{
		SStringList *node = m_head;
		while ( node )
		{
			SStringList *nextnode = node->next;
			delete node;
			node = nextnode;
		}
		m_head = m_tail = NULL;
		m_iCount = 0;
	}

	int Count () { return m_iCount; }

	SStringList *GetAt ( int at )
	{
		if ( at >= m_iCount ) return NULL;

		SStringList *node = m_head;
		for ( int i = 0; i < at; i++ )
		{
			node = node->next;
		}
		return node;
	}

	CyString StringAt(int at)
	{
		SStringList *strList = this->GetAt(at);
		if ( !strList )
			return NullString;
		return strList->str;
	}

	bool Insert(SStringList *node, CyString str, CyString data = NullString)
	{
		// if no node, then add at the end
		if ( !node )
		{
			SStringList *newNode = NewNode(str, data);
			m_tail->next = newNode;
			newNode->prev = m_tail;
			m_tail = newNode;
			++m_iCount;
		}
		// first node (ie pushfront
		else if ( node == m_head )
			PushFront(str, data);
		else // somewhere in the middle
		{
			SStringList *newNode = NewNode(str, data);
			newNode->next = node;
			newNode->prev = node->prev;
			node->prev->next = newNode;
			node->prev = newNode;
			++m_iCount;
		}

		return true;
	}
	bool Insert(int pos, CyString str, CyString data = NullString)
	{
		if ( pos < 0 )
			return false;

		SStringList *tmpNode = m_head;
		for ( int i = 0; i < pos; i++ )
		{
			tmpNode = tmpNode->next;
			if ( !tmpNode )
				break;
		}

		return Insert(tmpNode, str, data);
	}

	void DontRemove ( CyString str )
	{
		SStringList *node = m_head;
		while ( node )
		{
			if ( node->str == str )
			{
				node->remove = false;
				break;
			}
			node = node->next;
		}
	}

	int FindStringPos ( CyString str )
	{
		int pos = 0;
		for ( SStringList *node = m_head; node; node = node->next )
		{
			if ( node->str == str )
				return pos;
			++pos;
		}
		return -1;
	}
	SStringList *FindString ( CyString str )
	{
		for ( SStringList *node = m_head; node; node = node->next )
		{
			if ( node->str == str )
				return node;
		}
		return NULL;
	}
	SStringList *FindData ( CyString str )
	{
		for ( SStringList *node = m_head; node; node = node->next )
		{
			if ( node->data == str )
				return node;
		}
		return NULL;
	}

	void RemoveMarked ()
	{
		SStringList *node = m_head;
		while ( node )
		{
			if ( node->remove )
			{
				if ( node->prev )
					node->prev->next = node->next;
				else
					m_head = node->next;

				if ( node->next )
					node->next->prev = node->prev;
				else
					m_tail = node->prev;

				SStringList *nextnode = node->next;
				delete node;
				node = nextnode;
				--m_iCount;
			}
			else
				node = node->next;
		}
	}

	bool Remove ( CyString str, bool single = true )
	{
		bool removed = false;

		SStringList *node = m_head;
		while ( node )
		{
			if ( node->str == str )
			{
				if ( node->prev )
					node->prev->next = node->next;
				else
					m_head = node->next;

				if ( node->next )
					node->next->prev = node->prev;
				else
					m_tail = node->prev;

				SStringList *nextnode = node->next;
				delete node;
				removed = true;
				--m_iCount;
				node = nextnode;
				if ( single )
					break;
			}
			else
				node = node->next;
		}
		return removed;
	}

	SStringList *Change ( CyString str, CyString to )
	{
		SStringList *node = m_head;
		while ( node )
		{
			if ( node->str == str )
			{
				node->str = to;
				return node;
			}
			node = node->next;
		}
		return NULL;
	}

	void PopBack ()
	{
		if ( !m_head )
			return;

		if ( m_tail->prev )
			m_tail->prev->next = NULL;
		else
			m_head = NULL;
		SStringList *node = m_tail->prev;
		delete m_tail;
		m_tail = node;
		--m_iCount;
	}

	void PopFront ()
	{
		if ( !m_head )
			return;

		if ( m_head->next )
			m_head->next->prev = NULL;
		else
			m_tail = NULL;
		SStringList *node = m_head->next;
		delete m_head;
		m_head = node;
		--m_iCount;
	}

	void DeleteFrom ( SStringList *node )
	{
		if ( !node ) return;

		m_tail = node->prev;
		if ( !m_tail )
			m_head = NULL;
		else
			m_tail->next = NULL;

		while ( node )
		{
			SStringList *nextnode = node->next;
			delete node;
			--m_iCount;
			node = nextnode;
		}
	}

	void DeleteFrom(int pos)
	{
		DeleteFrom(GetAt(pos));
	}

	void PushFront ( const char *str, const char *data = 0) { return this->PushFront(CyString(str), (data) ? CyString(data) : NullString); }
    void PushFront ( const char *str, CyString data ) { return this->PushFront(CyString(str), data); }
	void PushFront ( CyString str, const char *data = 0) { return this->PushFront(str, (data) ? CyString(data) : NullString); }
    void PushFront ( CyString str, CyString data )
	{
		SStringList *s = NewNode(str, data);
		if ( !m_tail )
			m_tail = s;
		else
		{
			m_head->prev = s;
			s->next = m_head;
		}
		m_head = s;
		++m_iCount;
	}

	SStringList *PushBack ( const char *str, const char *data, bool search = false ) { return PushBack(CyString(str), CyString(data), search); }
    SStringList *PushBack ( const char *str, CyString data, bool search = false ) { return PushBack(CyString(str), data, search); }
    SStringList *PushBack ( CyString str, const char *data, bool search = false ) { return PushBack(str, CyString(data), search); }
    SStringList *PushBack ( CyString str, CyString data, bool search = false )
	{
		if ( search )
		{
			SStringList *n = FindString(str);
			if ( n )
			{
				n->data = data;
				n->remove = false;
				return n;
			}
		}
		SStringList *s = NewNode(str, data);
		if ( !m_head )
			m_head = s;
		else
		{
			m_tail->next = s;
			s->prev = m_tail;
		}
		m_tail = s;
		++m_iCount;
		return m_tail;
	}

	SStringList *PushBack ( const char *str, bool search = false ) { return PushBack(CyString(str), search); }
	SStringList *PushBack ( CyString str, bool search = false )
	{
		if ( search )
		{
			SStringList *n = FindString(str);
			if ( n )
			{
				n->remove = false;
				return n;
			}
		}
		SStringList *s = NewNode(str);
		if ( !m_head )
			m_head = s;
		else
		{
			m_tail->next = s;
			s->prev = m_tail;
		}
		m_tail = s;
		++m_iCount;
		return m_tail;
	}

	CyString First()
	{
		if ( m_head )
			return m_head->str;
		return NullString;
	}

private:
	SStringList *NewNode(CyString str, CyString data = NullString)
	{
		SStringList *s = new SStringList;
		s->next = s->prev = NULL;
		s->str = str;
		s->data = data;
		s->remove = false;

		return s;
	}

	SStringList *m_head;
	SStringList *m_tail;

	int m_iCount;
};

#endif //__BASE_ENGINE_STRINGLIST_H__
