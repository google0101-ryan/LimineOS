#pragma once

template<typename K, typename V>
class HashNode
{
public:
    HashNode(const K& key, const V& value)
    : key(key), value(value), next(nullptr) {}

    K getKey()
    {
        return key;
    }

    V getValue()
    {
        return value;
    }

    void setValue(V value)
    {
        this->value = value;
    }

    HashNode* getNext() const
    {
        return next;
    }

    void setNext(HashNode* next)
    {
        this->next = next;
    }

private:
    K key;
    V value;

    HashNode* next;
};

template<typename K, unsigned long tableSize>
struct KeyHash
{
    unsigned long operator()(const K& key) const
    {
        return static_cast<unsigned long>(key) % tableSize;
    }
};

namespace std
{

template<typename K, typename V, unsigned long tableSize = 10, typename F = KeyHash<K, tableSize>>
class unordered_map
{
public:
    unordered_map()
    : table(), hashFunc() {}

    void put(const K& key, const V& value)
    {
        unsigned long hashValue = hashFunc(key);
        HashNode<K, V>* prev = nullptr;
        HashNode<K, V>* entry = table[hashValue];

        while (entry != nullptr && entry->getKey() != key)
        {
            prev = entry;
            entry = entry->getNext();
        }

        if (entry == nullptr)
        {
            entry = new HashNode<K, V>(key, value);

            if (prev == nullptr)
                table[hashValue] = entry;
            else
                prev->setNext(entry);
        }
        else
            entry->setValue(value);
    }

    bool get(const K& key, V& value)
    {
        unsigned long hashValue = hashFunc(key);
        HashNode<K, V>* entry = table[hashValue];

        while (entry != nullptr)
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

    void remove(const K& key)
    {
        unsigned long hashValue = hashFunc(key);
        HashNode<K, V> *prev = NULL;
        HashNode<K, V> *entry = table[hashValue];

        while (entry != NULL && entry->getKey() != key) {
            prev = entry;
            entry = entry->getNext();
        }

        if (entry == NULL) {
            // key not found
            return;

        } else {
            if (prev == NULL) {
                // remove first bucket of the list
                table[hashValue] = entry->getNext();

            } else {
                prev->setNext(entry->getNext());
            }

            delete entry;
        }
    }
private:
    unordered_map(const unordered_map& other) {}
    const unordered_map& operator=(const unordered_map& other) {}
    HashNode<K, V>* table[tableSize];
    F hashFunc;
};

}