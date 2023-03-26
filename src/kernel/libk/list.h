#pragma once

template<class T>
class List
{
public:
	typedef T* iterator;

	List();
	List(unsigned int size);
	List(unsigned int size, const T & initial);
    List(const List<T> & v);      
    ~List();

	unsigned int capacity() const;
	unsigned int size() const;
	iterator begin();
	iterator end();
	T& front();
	T& back();
	void push_back(const T& value);
	void pop_back();

	void remove(unsigned int index);

	void reserve(unsigned int capacity);
	void resize(unsigned int size);

	T& operator[](unsigned int index);
	List<T>& operator=(const List<T>&);
	void clear();
private:
	unsigned int mSize;
	unsigned int mCapacity;
	T* buffer;
};

template <class T>
inline List<T>::List()
{
	mCapacity = 0;
	mSize = 0;
	buffer = 0;
}

template <class T>
inline List<T>::List(unsigned int size)
{
	mCapacity = size;
	mSize = size;
	buffer = new T[size];
}

template <class T>
inline List<T>::List(unsigned int size, const T &initial)
{
	mSize = size;
	mCapacity = size;
	buffer = new T[size];
	for (unsigned int i = 0; i < size; i++)
		buffer[i] = initial;
}

template <class T>
inline List<T>::List(const List<T> &v)
{
	mSize = v.mSize;
	mCapacity = v.mCapacity;
	buffer = new T[mSize];
	for (unsigned int i = 0; i < mSize; i++)
		buffer[i] = v.buffer[i];
}

template <class T>
inline List<T>::~List()
{
	if (buffer)
		delete[] buffer;
}

template <class T>
inline unsigned int List<T>::capacity() const
{
	return mCapacity;
}

template <class T>
inline unsigned int List<T>::size() const
{
	return mSize;
}

template <class T>
inline List<T>::iterator List<T>::begin()
{
	return buffer;
}

template <class T>
inline List<T>::iterator List<T>::end()
{
	return buffer + size();
}

template <class T>
inline T &List<T>::front()
{
	return buffer[0];
}

template <class T>
inline T &List<T>::back()
{
	return buffer[size() - 1];
}

template <class T>
inline void List<T>::push_back(const T &value)
{
	if (mSize >= mCapacity)
		resize(mCapacity+5);
	buffer[mSize++] = value;
}

template <class T>
inline void List<T>::pop_back()
{
	mSize--;
}

template <class T>
inline void List<T>::reserve(unsigned int capacity)
{
	if (!buffer)
	{
		mSize = 0;
		mCapacity = 0;
	}
	T* newBuffer = new T[capacity];
	unsigned int l_Size = capacity < mSize ? capacity : mSize;

	for (unsigned int i = 0; i < l_Size; i++)
		newBuffer[i] = buffer[i];
	
	mCapacity = capacity;
	delete[] buffer;
	buffer = newBuffer;
}

template <class T>
inline void List<T>::resize(unsigned int size)
{
	reserve(size);
	mSize = size;
}

template <class T>
inline T &List<T>::operator[](unsigned int index)
{
	return buffer[index];
}

template <class T>
inline List<T> &List<T>::operator=(const List<T> &v)
{
	delete[] buffer;
	mSize = v.mSize;
	mCapacity = v.mCapacity;
	buffer = new T[mSize];
	for (unsigned int i = 0; i < mSize; i++)
		buffer[i] = v.buffer[i];
	return *this;
}

template <class T>
inline void List<T>::clear()
{
	mCapacity = 0;
	mSize = 0;
	buffer = 0;
}
