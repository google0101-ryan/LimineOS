#pragma once

#include <kernel/libk/string.h>

#define TABLE_SIZE 512

template<typename K, typename V>
class HashNode
{
public:
	HashNode(const K& key, const V& value) :
	key(key), value(value), next(nullptr) {}

	K getKey() const {return key;}

	V getValue() const {return value;}

	void setValue(V value) {this->value = value;}

	HashNode* getNext() const {return next;}

	void setNext(HashNode* next) {this->next = next;}
private:
	K key;
	V value;
	HashNode* next;
};

template<typename K>
struct KeyHash
{
	unsigned long operator()(K key) const
	{
		return reinterpret_cast<unsigned long>(key) % TABLE_SIZE;
	}
};

template<>
inline unsigned long KeyHash<const char*>::operator()(const char* key) const
{
	unsigned long hash = 0;
	while (*key)
		hash += *key++;
	return hash % TABLE_SIZE;
}

template<>
inline unsigned long KeyHash<int>::operator()(int key) const
{
	return key % TABLE_SIZE;
}

template<typename K, typename V, typename F = KeyHash<K>>
class HashMap
{
public:
	HashMap()
	{
		table = new HashNode<K, V>*[TABLE_SIZE]();
		for (int i = 0; i < TABLE_SIZE; i++)
			table[i] = nullptr;
	}

	~HashMap()
	{
		for (int i = 0; i < TABLE_SIZE; ++i) 
		{
            HashNode<K, V> *entry = table[i];
            while (entry != NULL) 
			{
                HashNode<K, V> *prev = entry;
                entry = entry->getNext();
                delete prev;
            }
            table[i] = NULL;
        }
        // destroy the hash table
        delete [] table;
	}

	bool get(const K& key, V& value)
	{
		unsigned long hashValue = hashFunc(key);
		HashNode<K, V>* entry = table[hashValue];

		while (entry)
		{
			if (entry->getKey() == key)
			{
				value = entry->getValue();
				return true;
			}
			entry = entry->getNext();
		}
		return false;
	}

	void put(const K& key, const V& value)
	{
		unsigned long hashValue = hashFunc(key);
		HashNode<K, V>* prev = nullptr;
		HashNode<K, V>* entry = table[hashValue];

		while (entry && entry->getKey() != key)
		{
			prev = entry;
			entry = entry->getNext();
		}

		if (!entry)
		{
			entry = new HashNode<K, V>(key, value);
			if (!prev)
				table[hashValue] = entry;
			else
				prev->setNext(entry);
		}
		else
			entry->setValue(value);
	}

	void remove(const K& key)
	{
		unsigned long hashValue = hashFunc(key);
		HashNode<K, V>* prev = nullptr;
		HashNode<K, V>* entry = table[hashValue];

		while (entry && entry->getKey() != key)
		{
			prev = entry;
			entry = entry->getNext();
		}

		if (!entry)
			return;
		else
		{
			if (!prev)
				table[hashValue] = entry->getNext();
			else
				prev->setNext(entry->getNext());
		}
		delete entry;
	}
private:
	HashNode<K, V>** table;
	F hashFunc;
};