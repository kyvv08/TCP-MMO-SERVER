#pragma comment(lib, "winmm.lib")

/*
#include <cstddef>  // for std::size_t
#include <new>      // for placement new
#include <atomic>   // for atomic operations (optional for multi-threading)
#include "Timer.h"

template<typename t>
class memorypool {
private:
    struct Node {
        Node* next;
    };

    Node* freelist;   // 사용 가능한 메모리 블록을 관리하는 스택 (프리 리스트)
    char* pool;       // 메모리 풀 시작 주소
    std::size_t poolsize;  // 풀의 총 크기
    std::size_t blocksize; // 하나의 블록 크기
    std::size_t numblocks; // 블록 개수

public:
    memorypool(std::size_t numblocks)
        : freelist(nullptr), poolsize(sizeof(t)* numblocks), blocksize(sizeof(t)), numblocks(numblocks) {

        // 전체 메모리 풀 할당
        pool = static_cast<char*>(::operator new(poolsize));

        // 초기 메모리 블록들을 프리 리스트에 넣기
        for (std::size_t i = 0; i < numblocks; ++i) {
            deallocate(static_cast<t*>(static_cast<void*>(pool + i * blocksize)));
        }
    }

    ~memorypool() {
        ::operator delete(pool);
    }

    // 메모리 할당
    t* allocate() {
        if (freelist == nullptr) {
            return static_cast<t*>(::operator new(blocksize)); // 풀이 가득 찬 경우 예외적으로 new 호출
        }

        Node* node = freelist;
        freelist = freelist->next;

        return reinterpret_cast<t*>(node);
    }

    // 메모리 반환
    void deallocate(t* ptr) {
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = freelist;
        freelist = node;
    }

    // 메모리 풀이 소멸할 때까지 삭제하지 않고 유지
    void reset() {
        freelist = nullptr;
    }
};*/

//#include <iostream>
//#include <windows.h>
//#include "timer.h"
//template <typename T>
//class memorypool {
//private:
//    struct Node {
//        Node* next;
//        T data;  // 실제 데이터를 저장할 공간
//    };
//    Node* head;  // 스택처럼 동작할 free 리스트의 상단
//    DWORD poolsize; // 풀에서 관리하는 전체 블록의 수
//    DWORD allocnum; // 현재 할당된 블록의 수
//    void expandpool() {
//        
//    }
//public:
//    memorypool() : head(nullptr), poolsize(0), allocnum(0) {
//    }
//
//    ~memorypool() {
//        if (allocnum != 0) DebugBreak();
//        // 소멸 시 모든 노드를 삭제 (delete)
//        while (head) {
//            Node* temp = head;
//            head = head->next;
//            delete temp;
//        }
//    }
//
//    // 메모리 할당 (스택에서 pop)
//    T* allocate() {
//        if (head == nullptr) {
//            // 풀에 남은 블록이 없으면 새로운 노드를 할당
//            Node* newnode = new Node;
//            newnode->next = head;
//            head = newnode;
////            ++poolsize;
//        }
//        // 스택에서 메모리 블록을 가져옴
//        Node* allocnode = head;
//        head = head->next;  // 다음 사용 가능한 노드로 이동
////        ++allocnum;
//        allocnode->next = (Node*)this;
//        return &(allocnode->data);  // 데이터 공간을 리턴
//    }
//
//    // 메모리 반환 (스택에 push)
//    void deallocate(T* ptr) {
//        //node* node = (node*)((char*)ptr - offsetof(node,data));
//        Node* node = reinterpret_cast<Node*>((reinterpret_cast<char*>(ptr) - offsetof(Node, data)));
//        if (node->next != (Node*)this) return;
//        node->next = head;  // 반환된 블록을 스택 상단에 추가
//        head = node;
////        --allocnum;
//    }
//};

//-----------------------실제 사용할 버전

#include <iostream>
#include <new>
#include <windows.h>

// 메모리 풀 템플릿 클래스
template <typename T, bool ReuseObjects = false>
class MemoryPool {
private:
    struct Node {
        Node* next;
        alignas(alignof(T)) char data[sizeof(T)];  // 실제 데이터를 저장할 공간 (T 타입의 크기와 정렬)
    };

    Node* head;   // 사용 가능한 메모리 블록을 가리키는 포인터
    std::size_t poolSize;  // 풀에서 관리하는 전체 블록의 수
    std::size_t allocNum;  // 현재 할당된 블록의 수

    void ExpandPool() {
        // 메모리 풀을 확장하는 함수 (필요시 더 많은 노드를 생성)
        const std::size_t expandSize = 10;  // 한 번에 10개의 노드를 추가
        for (std::size_t i = 0; i < expandSize; ++i) {
            Node* newNode = new Node;
            newNode->next = head;
            head = newNode;
        }
        poolSize += expandSize;
    }

public:
    MemoryPool() : head(nullptr), poolSize(0), allocNum(0) {
    }

    ~MemoryPool() {
        // 소멸 시 모든 노드를 삭제
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    // 메모리 할당 (스택에서 pop)
    T* allocate() {
        //Timer("allocate");

        // 메모리 풀이 비었으면 확장
        if (!head) {
            ExpandPool();
        }

        // 스택에서 메모리 블록을 가져옴
        Node* allocNode = head;
        head = head->next;

        T* objPtr = nullptr;

        if constexpr (ReuseObjects) {
            // 재사용 모드인 경우 메모리 블록을 새로운 객체로 초기화
            objPtr = reinterpret_cast<T*>(allocNode->data);
            new (objPtr) T();  // placement new: 메모리 풀의 공간에 새 객체 생성
        }
        else {
            // 일반 모드에서는 객체가 생성되지 않고 순수 메모리 블록만 반환
            objPtr = reinterpret_cast<T*>(allocNode->data);
        }

        ++allocNum;
        return objPtr;
    }

    // 메모리 반환 (스택에 push)
    void deallocate(T* ptr) {
        //Timer("deallocate");

        if constexpr (ReuseObjects) {
            // 재사용 모드인 경우 소멸자 호출 후 메모리 반환
            ptr->~T();  // 명시적으로 소멸자 호출
        }

        // Node*로 변환하고 스택에 다시 push
        Node* node = reinterpret_cast<Node*>((reinterpret_cast<char*>(ptr) - offsetof(Node, data)));
        node->next = head;
        head = node;

        --allocNum;
    }
};
//
//class A {
//private:
//    int* a;
//public:
//    A(){
//        a = new int[10];
//    }
//};
////memorypool<A> pool;
//MemoryPool<A> pool;
//void TestNew(A*& a) {
//    Timer("new");
//    a = new A;
//}
//
//void TestDelete(A*& a) {
//    Timer("delete");
//    delete a;
//}
//
////void TestPoolNew(A*& a,memorypool<A>& pool) {
////    Timer("poolNew");
////    a = pool.allocate();
////}
////
////void TestPoolDelete(A*& a, memorypool<A>& pool) {
////    Timer("poolDelete");
////    pool.deallocate(a);
////}
//
//void TestPoolNew(A*& a) {
//    Timer("poolNew");
//    a = pool.allocate();
//}
//
//void TestPoolDelete(A*& a) {
//    Timer("poolDelete");
//    pool.deallocate(a);
//}
//
////void TestNewDelete(A*& a) {
////        Timer("Total new/delete");
////        for (int i = 0; i < 10000; i++) {
////            TestNew(a);
////            TestDelete(a);
////        }
////}
////
////void TestPoolNewDelete(A*& a) {
////    Timer("Total pool allocate/deallocate");
////    for (int i = 0; i < 10000; i++) {
////        TestPoolNew(a);
////        TestPoolDelete(a);
////    }
////}
//
//
//
//int main()
//{
//    timeBeginPeriod(1);
//    LARGE_INTEGER Start;
//    LARGE_INTEGER End;
//    LARGE_INTEGER Freq;
//    //memorypool<A> pool(1);
//    A* a;
//    a = NULL;
//    for (int i = 0; i < 100000; i++) {
//        a = new A;
//        delete a;
//    }
//
//    for (int i = 0; i < 100000; i++) {
//        a = pool.allocate();
//        pool.deallocate(a);
//    }
//
//    QueryPerformanceCounter(&Start);
//    for (int i = 0; i < 1000000; i++) {
//        a = new A;
//        delete a;
//    }
//    QueryPerformanceCounter(&End);
//    std::cout << "new/delete : " << (End.QuadPart - Start.QuadPart) << std::endl;
//    QueryPerformanceCounter(&Start);
//    for (int i = 0; i < 1000000; i++) {
//        a = pool.allocate();
//        pool.deallocate(a);
//    }
//    QueryPerformanceCounter(&End);
//    std::cout << "Pool new/delete : " << (End.QuadPart - Start.QuadPart) << std::endl;
//    for (int i = 0; i < 1000000; i++) {
//        TestNew(a);
//        TestDelete(a);
//    }
//
//    for (int i = 0; i < 1000000; i++) {
//        TestPoolNew(a);
//        TestPoolDelete(a);
//    }
//
//    //for (int i = 0; i < 100000; i++) {
//    //    TestPoolNew(a,pool);
//    //    TestPoolDelete(a,pool);
//    //}
//
//    PrintProfileData();
//
//    return 0;
//}