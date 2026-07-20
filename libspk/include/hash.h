#ifndef __HASH_H__
#define __HASH_H__

#include "CyString.h"

#define HASH_DEFAULT_PRIME	383
#define HASH_DEFAULT_MULTI	1024

#define CStringHashTable CHashTableNonPtr<CyString>

template <class DATA>
class CHashNode
{
public:
	CHashNode<DATA> ( CyString &name, DATA index ) { m_name = name; m_index = index; m_next = NULL; m_prev = NULL; }

	CHashNode<DATA> *Next	 () { return m_next;  }
	CHashNode<DATA> *Prev	 () { return m_prev;  }
	CyString		     &Name	 () { return m_name;  }
	DATA             GetData () { return m_index; }

	void SetData ( DATA index )				{ m_index = index; }
	void SetNext ( CHashNode<DATA> *next )  { m_next = next; }
	void SetPrev ( CHashNode<DATA> *prev )  { m_prev = prev; }

private:
	CyString		 m_name;
	DATA			 m_index;
	CHashNode<DATA> *m_next;
	CHashNode<DATA> *m_prev;
};

template <class DATA>
class CHashTable 
{
public:
	CHashTable<DATA> ( unsigned int prime, unsigned int multi ) 
	{ 
		m_prime = prime;
		m_multi = multi;

		#ifdef TESTHASH
			m_count = (int *)malloc(sizeof(int) * m_prime);
		#else
			m_head = (CHashNode<DATA> **)malloc(sizeof(CHashNode<DATA> *) * m_prime);
			m_tail = (CHashNode<DATA> **)malloc(sizeof(CHashNode<DATA> *) * m_prime);
		#endif

		for ( unsigned int i = 0; i < m_prime; i++ )
		{
			#ifdef TESTHASH
				m_count[i] = 0;
			#else
				m_head[i] = NULL;
				m_tail[i] = NULL;
			#endif
		}
		m_currentItr = NULL;
		m_currentItrTable = 0;
	}
	~CHashTable<DATA> ()
	{
		#ifdef TESTHASH
			free ( m_count );
		#else
			for ( unsigned int i = 0; i < m_prime; i++ )
			{
				CHashNode<DATA> *nextnode, *node = m_head[i];
				while ( node )
				{
					nextnode = node->Next();
					delete node;
					node = nextnode;
				}
			}
			free ( m_head );
			free ( m_tail );
		#endif
	}

	void ClearIterate () { m_currentItrTable = 0; m_currentItr = NULL; }

	CHashNode<DATA> *Iterate () 
	{
		if ( !m_currentItr )
		{
			m_currentItrTable = 0;
			m_currentItr = m_head[0];

			while ( !m_currentItr )
			{
				++m_currentItrTable;
				if ( m_currentItrTable >= m_prime )
					break;
				m_currentItr = m_head[m_currentItrTable];
			}
		}
		else
		{
			m_currentItr = m_currentItr->Next();			
			while ( !m_currentItr )
			{
				++m_currentItrTable;
				if ( m_currentItrTable >= m_prime )
					break;
				m_currentItr = m_head[m_currentItrTable];
			}
		}
		return m_currentItr;
	}

	DATA IterateData ()
	{
		if ( m_currentItr )
			return m_currentItr->GetData();
		return NULL;
	}

	void RemoveIterate ()
	{
		if ( !m_currentItr )
			return;

		CHashNode<DATA> *nextnode = m_currentItr->Prev();
		int table = m_currentItrTable;

		while ( (!nextnode) && (table >= 0) )
		{
			--table;
			nextnode = m_tail[table];
		}

		if ( m_currentItr->Next() )
			m_currentItr->Next()->SetPrev ( m_currentItr->Prev() );
		else
			m_tail[m_currentItrTable] = m_currentItr->Prev();

		if ( m_currentItr->Prev() )
			m_currentItr->Prev()->SetNext ( m_currentItr->Next() );
		else
			m_head[m_currentItrTable] = m_currentItr->Next();

		delete m_currentItr;
		m_currentItrTable = table;
		m_currentItr = nextnode;
	}

/*#ifdef TESTHASH
	void Add ( CyString &name )
	{
		++m_count[Hash ( name )];
	}
#else*/
	void Remove ( CyString &name, bool deletenode = true )
	{
		unsigned int hash = Hash ( name );

		CHashNode<DATA> *node = FindEntity ( hash, name );
		if ( node )
		{
			// only one in list?
			if ( (m_head[hash] == node) && (m_tail[hash] == node) )
			{
				m_head[hash] = NULL;
				m_tail[hash] = NULL;
			}
			// remove from front of list
			else if ( m_head[hash] == node )
			{
				node->Next()->SetPrev ( NULL );
				m_head[hash] = node->Next();
			}
			// remove from end of list
			else if ( m_tail[hash] == node )
			{
				node->Prev()->SetNext ( NULL );
				m_tail[hash] = node->Prev();
			}
			// some where in the middle
			else
			{
				node->Prev()->SetNext ( node->Next() );
				node->Next()->SetPrev ( node->Prev() );
			}
			if ( deletenode )
				delete node;
		}
	}

	void Add ( CyString &name, DATA index, bool search = true )
	{
		unsigned int hash = Hash ( name );

		if ( search )
		{
			CHashNode<DATA> *node = FindEntity ( hash, name );
			if ( node )
				node->SetData ( index );
			else
				Put ( hash, name, index );
		}
		else
			Put ( hash, name, index );

	}

	DATA FindData ( CyString &name )
	{
		unsigned int hash = Hash ( name );

		CHashNode<DATA> *node = FindEntity ( hash, name );

		if ( node )
			return node->GetData ();

		return 0;
	}
	CHashNode<DATA> *FindEntity ( CyString &name )
	{
		return FindEntity ( Hash ( name ), name );
	}

	void Empty ()
	{
		for ( int i = 0; i < m_prime; i++ )
		{
			CHashNode<DATA> *node = m_head[i], *curnode = NULL;
			while ( node )
			{
				curnode = node;
				node = node->Next();
				delete curnode;
			}
			m_head[i] = m_tail[i] = NULL;
		}
	}
//#endif


private:
#ifndef TESTHASH
	CHashNode<DATA> *FindEntity ( unsigned int hash, CyString &name )
	{
		CHashNode<DATA> *node = m_head[hash];

		while ( node )
		{
			if ( node->Name() == name )
				return node;

			node = node->Next();
		}
		return NULL;
	}
	void Put ( unsigned int pos, CyString &name, DATA index )
	{
		if ( pos >= m_prime )
			pos %= m_prime;

		CHashNode<DATA> *node = new CHashNode<DATA> ( name, index );

		if ( !m_tail[pos] )
			m_head[pos] = m_tail[pos] = node;
		else
		{
			node->SetPrev ( m_tail[pos] );
			m_tail[pos]->SetNext ( node );
			m_tail[pos] = node;
		}

	}
#endif

	unsigned int Hash ( CyString &name )
	{
		unsigned int hash = 0;
 
		/*  transform the string into an integer  */
		for ( int i = 0; i < name.Length(); i++ )
			hash = (hash * m_multi) + name[i];
  
		/*  reduce the integer to the correct range  */
		return hash % m_prime;
	}

	unsigned int		m_prime;
	unsigned int		m_multi;
#ifndef TESTHASH
	CHashNode<DATA>   **m_head;
	CHashNode<DATA>   **m_tail;
	CHashNode<DATA>   *m_currentItr;
	unsigned int	   m_currentItrTable;
#else
	int *m_count;
#endif
};


template <class DATA>
class CHashNodeNonPtr
{
public:
	CHashNodeNonPtr<DATA> ( CyString &name, DATA index ) { m_name = name; m_index = index; m_next = NULL; m_prev = NULL; }

	CHashNodeNonPtr<DATA> *Next	 () { return m_next;  }
	CHashNodeNonPtr<DATA> *Prev	 () { return m_prev;  }
	CyString		     &Name	 () { return m_name;  }
	DATA             GetData () { return m_index; }

	void SetData ( DATA index )				{ m_index = index; }
	void SetNext ( CHashNodeNonPtr<DATA> *next )  { m_next = next; }
	void SetPrev ( CHashNodeNonPtr<DATA> *prev )  { m_prev = prev; }

private:
	CyString		 m_name;
	DATA			 m_index;
	CHashNodeNonPtr<DATA> *m_next;
	CHashNodeNonPtr<DATA> *m_prev;
};

template <class DATA>
class CHashTableNonPtr
{
public:
	CHashTableNonPtr<DATA> ( unsigned int prime, unsigned int multi, DATA null ) 
	{ 
		m_prime = prime;
		m_multi = multi;

		m_null = null;

		#ifdef TESTHASH
			m_count = (int *)malloc(sizeof(int) * m_prime);
		#else
			m_head = (CHashNodeNonPtr<DATA> **)malloc(sizeof(CHashNodeNonPtr<DATA> *) * m_prime);
			m_tail = (CHashNodeNonPtr<DATA> **)malloc(sizeof(CHashNodeNonPtr<DATA> *) * m_prime);
		#endif

		for ( unsigned int i = 0; i < m_prime; i++ )
		{
			#ifdef TESTHASH
				m_count[i] = 0;
			#else
				m_head[i] = NULL;
				m_tail[i] = NULL;
			#endif
		}
		m_bIgnoreCase = false;
		m_currentItr = NULL;
		m_currentItrTable = 0;
	}
	~CHashTableNonPtr<DATA> ()
	{
		#ifdef TESTHASH
			free ( m_count );
		#else
			for ( unsigned int i = 0; i < m_prime; i++ )
			{
				CHashNodeNonPtr<DATA> *nextnode, *node = m_head[i];
				while ( node )
				{
					nextnode = node->Next();
					delete node;
					node = nextnode;
				}
			}
			free ( m_head );
			free ( m_tail );
		#endif
	}

	void IgnoreCase(bool b) { m_bIgnoreCase = b; }
	void ClearIterate () { m_currentItrTable = 0; m_currentItr = NULL; }

	CHashNodeNonPtr<DATA> *Iterate () 
	{
		if ( !m_currentItr )
		{
			m_currentItrTable = 0;
			m_currentItr = m_head[0];

			while ( !m_currentItr )
			{
				++m_currentItrTable;
				if ( m_currentItrTable >= m_prime )
					break;
				m_currentItr = m_head[m_currentItrTable];
			}
		}
		else
		{
			m_currentItr = m_currentItr->Next();			
			while ( !m_currentItr )
			{
				++m_currentItrTable;
				if ( m_currentItrTable >= m_prime )
					break;
				m_currentItr = m_head[m_currentItrTable];
			}
		}
		return m_currentItr;
	}

	DATA IterateData ()
	{
		if ( m_currentItr )
			return m_currentItr->GetData();
		return m_null;
	}

	void RemoveIterate ()
	{
		if ( !m_currentItr )
			return;

		CHashNodeNonPtr<DATA> *nextnode = m_currentItr->Prev();
		int table = m_currentItrTable;

		while ( (!nextnode) && (table >= 0) )
		{
			--table;
			nextnode = m_tail[table];
		}

		if ( m_currentItr->Next() )
			m_currentItr->Next()->SetPrev ( m_currentItr->Prev() );
		else
			m_tail[m_currentItrTable] = m_currentItr->Prev();

		if ( m_currentItr->Prev() )
			m_currentItr->Prev()->SetNext ( m_currentItr->Next() );
		else
			m_head[m_currentItrTable] = m_currentItr->Next();

		delete m_currentItr;
		m_currentItrTable = table;
		m_currentItr = nextnode;
	}

/*#ifdef TESTHASH
	void Add ( CyString &name )
	{
		++m_count[Hash ( name )];
	}
#else*/
	void Remove ( CyString &name, bool deletenode = true )
	{
		unsigned int hash = Hash ( name );

		CHashNodeNonPtr<DATA> *node = FindEntity ( hash, name );
		if ( node )
		{
			// only one in list?
			if ( (m_head[hash] == node) && (m_tail[hash] == node) )
			{
				m_head[hash] = NULL;
				m_tail[hash] = NULL;
			}
			// remove from front of list
			else if ( m_head[hash] == node )
			{
				node->Next()->SetPrev ( NULL );
				m_head[hash] = node->Next();
			}
			// remove from end of list
			else if ( m_tail[hash] == node )
			{
				node->Prev()->SetNext ( NULL );
				m_tail[hash] = node->Prev();
			}
			// some where in the middle
			else
			{
				node->Prev()->SetNext ( node->Next() );
				node->Next()->SetPrev ( node->Prev() );
			}
			if ( deletenode )
				delete node;
		}
	}

	void Add ( CyString &name, DATA index, bool search = true )
	{
		unsigned int hash = Hash ( name );

		if ( search )
		{
			CHashNodeNonPtr<DATA> *node = FindEntity ( hash, name );
			if ( node )
				node->SetData ( index );
			else
				Put ( hash, name, index );
		}
		else
			Put ( hash, name, index );

	}

	DATA FindData ( CyString &name )
	{
		unsigned int hash = Hash ( name );

		CHashNodeNonPtr<DATA> *node = FindEntity ( hash, name );

		if ( node )
			return node->GetData ();

		return m_null;
	}
	CHashNodeNonPtr<DATA> *FindEntity ( CyString &name )
	{
		return FindEntity ( Hash ( name ), name );
	}

	void Empty ()
	{
		for ( int i = 0; i < m_prime; i++ )
		{
			CHashNodeNonPtr<DATA> *node = m_head[i], *curnode = NULL;
			while ( node )
			{
				curnode = node;
				node = node->Next();
				delete curnode;
			}
			m_head[i] = m_tail[i] = NULL;
		}
	}
//#endif


protected:
#ifndef TESTHASH
	CHashNodeNonPtr<DATA> *FindEntity ( unsigned int hash, CyString &name )
	{
		CHashNodeNonPtr<DATA> *node = m_head[hash];

		while ( node )
		{
			if ( (!m_bIgnoreCase) && (node->Name() == name) )
				return node;
			else if ( (m_bIgnoreCase) && (node->Name().Compare(name)) )
				return node;

			node = node->Next();
		}
		return NULL;
	}
	void Put ( unsigned int pos, CyString &name, DATA index )
	{
		if ( pos >= m_prime )
			pos %= m_prime;

		CHashNodeNonPtr<DATA> *node = new CHashNodeNonPtr<DATA> ( name, index );

		if ( !m_tail[pos] )
			m_head[pos] = m_tail[pos] = node;
		else
		{
			node->SetPrev ( m_tail[pos] );
			m_tail[pos]->SetNext ( node );
			m_tail[pos] = node;
		}

	}
#endif

	unsigned int Hash ( CyString &name )
	{
		unsigned int hash = 0;
 
		/*  transform the string into an integer  */
		for ( unsigned int i = 0; i < name.Length(); i++ )
			hash = (hash * m_multi) + name[i];
  
		/*  reduce the integer to the correct range  */
		return hash % m_prime;
	}

	unsigned int		m_prime;
	unsigned int		m_multi;
#ifndef TESTHASH
	CHashNodeNonPtr<DATA>   **m_head;
	CHashNodeNonPtr<DATA>   **m_tail;
	CHashNodeNonPtr<DATA>   *m_currentItr;
	unsigned int	   m_currentItrTable;
	DATA				m_null;
	bool			m_bIgnoreCase;
#else
	int *m_count;
#endif
};

#endif //__HASH_H__