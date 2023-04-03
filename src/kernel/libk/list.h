#pragma once

#include <stddef.h>

template<class T>
class vector
{
public:
	vector();

	void push_back(const T& d);

	void reserve(int newalloc);

	size_t size() const;

	T& operator[](int i);
private:
	size_t _size;
	T* _elements;
	size_t _space;
};

template<class T>
vector<T>::vector()
: _size(0), _elements(0), _space(0)
{
}

template <class T>
inline void vector<T>::push_back(const T &d)
{
	if (_space == 0)
		reserve(8);
	else if (_size == _space)
		reserve(2 * _space);
	
	_elements[_size] = d;

	++_size;
}

template <class T>
inline void vector<T>::reserve(int newalloc)
{
	if (newalloc <= _space) return;

	T* p = new T[newalloc];

	for (int i = 0; i < _size; ++i)
		p[i] = _elements[i];
	
	delete[] _elements;

	_elements = p;

	_space = newalloc;
}

template <class T>
inline size_t vector<T>::size() const
{
	return _size;
}

template <class T>
inline T &vector<T>::operator[](int i)
{
	return _elements[i];
}
