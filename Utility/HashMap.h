#pragma once
//=============================================================================
//
// キーと値をすばやく整理する図書館司書ちゃん [HashMap.h]
// Author : 
// ハッシュ関数と等価演算子を使って、データをすばやく検索・追加・削除できる自作マップクラスちゃんっ！
// 連想配列としての使いやすさとカスタマイズ性を両立した、かしこかわいい構造なの!
//
//=============================================================================
#include "main.h"

struct CharPtrHash
{
    unsigned int operator()(const char* str) const
    {
        unsigned long hash = 5381; // djb2アルゴリズムの初期値
        int c;
        if (str == nullptr)
            return -1;
        // 文字列を一文字ずつ処理
        while ((c = *str++))
        {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }

        return hash;
    }
};

struct CharPtrEquals
{
    bool operator()(const char* a, const char* b) const
    {
        return strcmp(a, b) == 0;
    }
};

struct HashUInt64
{
    size_t operator()(uint64_t key) const
    {
        return key ^ (key >> 33);
    }
};


struct EqualUInt64
{
    bool operator()(uint64_t a, uint64_t b) const
    {
        return a == b;
    }
};

template<typename Key, typename Value, typename HashFunc, typename EqualsFunc>
class HashMap
{
private:

    struct Data
    {
        Key key;
        Value value;

        Data(const Key& key, const Value& value) : key(key), value(value) {};
    };

    struct Node 
    {
        Data data;
        Node* next;
        Node(const Key& key, const Value& value) : data(key, value), next(nullptr) {}
    };

    Node** buckets;
    int size;

    HashFunc hashFunc;
    EqualsFunc equalsFunc;

public:

    HashMap()
        : size(32), hashFunc(HashFunc{}), equalsFunc(EqualsFunc{})
    {
        buckets = new Node * [size];
        for (int i = 0; i < size; ++i)
        {
            buckets[i] = nullptr;
        }
    }

    HashMap(int size, HashFunc hashFunc, EqualsFunc equalsFunc)
        : size(size), hashFunc(hashFunc), equalsFunc(equalsFunc)
    {
        buckets = new Node * [size];
        for (int i = 0; i < size; ++i) 
        {
            buckets[i] = nullptr;
        }
    }


    ~HashMap()
    {
        for (int i = 0; i < size; ++i) 
        {
            Node* current = buckets[i];
            while (current != nullptr) 
            {
                Node* toDelete = current;
                current = current->next;
                delete toDelete;
            }
        }
        delete[] buckets;
    }

    HashMap& operator=(const HashMap& other)
    {
        if (this == &other) return *this;

        this->clear();
        delete[] this->buckets;

        this->size = other.size;
        this->buckets = new Node * [this->size] {};
        for (int i = 0; i < this->size; ++i)
        {
            Node* current = other.buckets[i];
            Node** ptr = &this->buckets[i];
            while (current != nullptr)
            {
                Node* newNode = new Node(current->data.key, current->data.value);
                *ptr = newNode;
                ptr = &newNode->next;
                current = current->next;
            }
        }
        this->hashFunc = other.hashFunc;
        this->equalsFunc = other.equalsFunc;

        return *this;
    }


    void insert(const Key& key, const Value& value)
    {
        unsigned int index = hashFunc(key) % size;
        Node* newNode = new Node(key, value);
        newNode->next = buckets[index];
        buckets[index] = newNode;
    }

    Value* search(const Key& key) const
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];
        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                return &current->data.value;
            }
            current = current->next;
        }
        return nullptr;
    }

    Value& operator[](const Key& key)
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];

        // Search for the key in the linked list
        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                return current->data.value;
            }
            current = current->next;
        }

        // Key not found, create new node with default value
        insert(key, Value{});
        return buckets[index]->data.value; // Return the newly inserted node's value
    }

    int count(const Key& key) const
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];
        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                return 1;
            }
            current = current->next;
        }
        return 0;
    }

    bool contains(const Key& key) const 
    { 
        return count(key) != 0; 
    }

    bool remove(const Key& key)
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];
        Node* previous = nullptr;

        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                if (previous == nullptr) 
                {
                    buckets[index] = current->next;
                }
                else {
                    previous->next = current->next;
                }
                delete current;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    void clear()
    {
        for (int i = 0; i < size; ++i)
        {
            Node* current = buckets[i];
            while (current != nullptr)
            {
                Node* toDelete = current;
                current = current->next;
                delete toDelete;
            }
            buckets[i] = nullptr; // Reset the bucket pointer after clearing
        }
    }

    Value& at(const Key& key)
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];
        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                return current->data.value;
            }
            current = current->next;
        }
        throw std::out_of_range("Key not found in HashMap");
    }

    const Value& at(const Key& key) const
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];
        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                return current->data.value;
            }
            current = current->next;
        }
        throw std::out_of_range("Key not found in HashMap");
    }

    class Iterator
    {
    private:
        Node** buckets;
        Node* current;
        int index;
        int size;

    public:
        Iterator(Node** buckets, int size, int index = 0, Node* node = nullptr)
            : buckets(buckets), size(size), index(index), current(node)
        {
            if (current == nullptr)
            {
                // Advance to the first valid node
                for (; this->index < size; ++this->index)
                {
                    if (buckets[this->index])
                    {
                        current = buckets[this->index];
                        break;
                    }
                }
            }
        }

        bool operator!=(const Iterator& other) const
        {
            return current != other.current;
        }

        bool operator==(const Iterator& other) const
        {
            return current == other.current;
        }

        Iterator& operator++()
        {
            if (current && current->next)
            {
                current = current->next;
            }
            else
            {
                do {
                    ++index;
                    if (index >= size) {
                        current = nullptr;
                        break;
                    }
                    current = buckets[index];
                } while (current == nullptr);
            }
            return *this;
        }

        Data& operator*() const
        {
            return current->data;
        }

        Data* operator->() const
        {
            return &current->data;
        }
    };

    Iterator begin() const
    {
        return Iterator(buckets, size);
    }

    Iterator end() const
    {
        return Iterator(buckets, size, size);
    }

    Iterator find(const Key& key) const
    {
        unsigned int index = hashFunc(key) % size;
        Node* current = buckets[index];
        while (current != nullptr)
        {
            if (equalsFunc(current->data.key, key))
            {
                return Iterator(buckets, size, index, current);
            }
            current = current->next;
        }
        return end();
    }
};