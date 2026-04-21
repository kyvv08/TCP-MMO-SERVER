#pragma once
#include <iostream>
#include <new>
#include <windows.h>

#define EXPAND_SIZE 10

/**
 * @brief Template Memory Pool Class
 * @tparam T Type of object to pool
 * @tparam ReuseObjects If true, calls constructor/destructor on Allocate/Free
 */
template <typename T, bool ReuseObjects = false>
class MemoryPool {
private:
    struct Node {
        Node* pNext;
        alignas(alignof(T)) char Data[sizeof(T)];
    };

    Node* m_pHead;
    std::size_t m_PoolSize;
    std::size_t m_AllocNum;

    void ExpandPool() {
        for (std::size_t i = 0; i < EXPAND_SIZE; ++i) {
            Node* pNewNode = new Node;
            pNewNode->pNext = m_pHead;
            m_pHead = pNewNode;
        }
        m_PoolSize +=  EXPAND_SIZE;
    }

public:
    MemoryPool() : m_pHead(nullptr), m_PoolSize(0), m_AllocNum(0) {}

    ~MemoryPool() {
        while (m_pHead) {
            Node* pTemp = m_pHead;
            m_pHead = m_pHead->pNext;
            delete pTemp;
        }
    }

    /**
     * @brief Allocate an object from the pool
     * @return Pointer to the allocated object
     */
    T* Allocate() {
        if (!m_pHead) {
            ExpandPool();
        }

        Node* pAllocNode = m_pHead;
        m_pHead = m_pHead->pNext;

        T* pObj = reinterpret_cast<T*>(pAllocNode->Data);

        if constexpr (ReuseObjects) {
            new (pObj) T(); 
        }

        ++m_AllocNum;
        return pObj;
    }

    /**
     * @brief Return an object to the pool
     * @param pPtr Pointer to the object to return
     */
    void Free(T* pPtr) {
        if (!pPtr) return;

        if constexpr (ReuseObjects) {
            pPtr->~T();
        }

        Node* pNode = reinterpret_cast<Node*>((reinterpret_cast<char*>(pPtr) - offsetof(Node, Data)));
        pNode->pNext = m_pHead;
        m_pHead = pNode;

        --m_AllocNum;
    }

    std::size_t GetPoolSize() const { return m_PoolSize; }
    std::size_t GetAllocCount() const { return m_AllocNum; }
};
