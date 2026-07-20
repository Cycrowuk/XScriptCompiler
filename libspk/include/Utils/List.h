#ifndef __LIST_H__
#define __LIST_H__

#include "../spkdll.h"
#include <vector>

//namespace SPK {
namespace Utils {


	template <class T>
	class CList
	{
	public:
		typedef typename std::vector<T *>::iterator iterator ;

	private:
		std::vector<T *> *_lItems;
		bool			 _bDontClear;
		typename std::vector<T *>::iterator _pItr;

	public:
		CList(bool bDontClear = false);
		~CList();

		void clear();
		void deleteItems();

		T *push_back(T *pItem);
		T *push_front(T *pItem);
		void pop_back();

		iterator insert(iterator itr, T *pItem);
		iterator insertAt(size_t idx, T* pItem);

		iterator begin() const;
		iterator end() const;
		T *first();
		T *next();

		T* front() const;
		T* back() const;

		bool empty() const;
		size_t size() const;

		T *get(size_t i) const;

		iterator remove(iterator itr);
		iterator removeAt(size_t idx);

		T* operator[](size_t idx);
	};


	///////////////////////////////////////////////////////////////////////////

	template<class T>
	CList<T>::CList(bool bDontClear) : _bDontClear(bDontClear)
	{
		_lItems = new std::vector<T *>();
		_pItr = _lItems->end();
	}

	template <class T>
	CList<T>::~CList()
	{
		clear();
		delete _lItems;
	}

	template <class T>
	void CList<T>::clear()
	{
		if ( !_bDontClear ) {
			for(std::vector<T *>::iterator itr = _lItems->begin(); itr != _lItems->end(); itr++) {
				T *data = (*itr);
				delete (*itr);
			}
		}

		_lItems->clear();
	}

	template <class T>
	void CList<T>::deleteItems()
	{
		for (std::vector<T*>::iterator itr = _lItems->begin(); itr != _lItems->end(); itr++) {
			T* data = (*itr);
			delete (*itr);
		}
		_lItems->clear();
	}

	template <class T>
	T* CList<T>::push_back(T* pItem)
	{
		_lItems->push_back(pItem);
		return pItem;
	}

	template <class T>
	void CList<T>::pop_back()
	{
		_lItems->pop_back();
	}

	template <class T>
	typename CList<T>::iterator CList<T>::insert(CList<T>::iterator itr, T *pItem)
	{
		return _lItems->insert(itr, pItem);
	}

	template <class T>
	typename CList<T>::iterator CList<T>::insertAt(size_t idx, T *pItem)
	{
		return _lItems->insert(_lItems->begin() + idx, pItem);
	}

	template <class T>
	T *CList<T>::push_front(T *pItem)
	{
		_lItems->insert(_lItems->begin(), pItem);
		return pItem;
	}

	template <class T>
	typename CList<T>::iterator CList<T>::begin() const
	{
		return _lItems->begin();
	}

	template <class T>
	typename CList<T>::iterator CList<T>::end() const
	{
		return _lItems->end();
	}

	template <class T>
	T* CList<T>::front() const
	{
		if (_lItems->empty()) return NULL;
		return _lItems->front();
	}

	template <class T>
	T* CList<T>::back() const
	{
		if (_lItems->empty()) return NULL;
		return _lItems->back();
	}

	template <class T>
	T* CList<T>::first()
	{
		if (_lItems->empty()) return NULL;
		_pItr = _lItems->begin();
		return *_pItr;
	}

	template <class T>
	T *CList<T>::next()
	{
		if ( _lItems->empty() ) return NULL;
		if ( _pItr == _lItems->end() ) return NULL;
		++_pItr;
		if ( _pItr == _lItems->end() ) return NULL;
		return *_pItr;
	}

	template <class T>
	bool CList<T>::empty() const
	{
		return _lItems->empty();
	}

	template <class T>
	size_t CList<T>::size() const
	{
		return _lItems->size();
	}

	template <class T>
	T *CList<T>::get(size_t i) const
	{
		return (*_lItems)[i];
	}

	template <class T>
	typename CList<T>::iterator CList<T>::remove(CList<T>::iterator itr)
	{
		return _lItems->erase(itr);
	}

	template <class T>
	typename CList<T>::iterator CList<T>::removeAt(size_t idx)
	{
		return _lItems->erase(_lItems->begin() + idx);
	}

	template <class T>
	T* CList<T>::operator[](size_t idx)
	{
		if(idx < _lItems->size())	
			return _lItems[idx];
		return nullptr;
	}
}
//}

#endif //__LIST_H__