#include <iostream>
#include <windows.h>
#include <time.h>
#include "RingBuffer.h"

RingBuffer::RingBuffer(void):front(0),rear(0),size(BUF_SIZE) {
    ringBuf = new char[BUF_SIZE];
}

RingBuffer::RingBuffer(int s) :front(0), rear(0),size(s) {
    ringBuf = new char[s];
}
RingBuffer::~RingBuffer(void) {
    delete ringBuf;
}

int RingBuffer::GetUseSize() {
    if (front <= rear) {
        return rear - front;
    }
    else {
        return size - front + rear;
    }
}

int RingBuffer::GetFreeSize() {
    return size - GetUseSize() - 1;
}

int RingBuffer::Enqueue(char* msg, int len) {
    if (GetFreeSize() < len || len > size-1) return -1;
    int fLen = size - rear;
    int sLen;
    if (fLen > len) {
        fLen = len;
    }
    sLen = len - fLen;
    memcpy(ringBuf + rear, msg, fLen);
    if(sLen > 0){
        memcpy(ringBuf,msg+fLen,sLen);
    }
    rear = (rear + len) % size;
    return len;
}

int RingBuffer::Dequeue(char* msg, int len) {
    if (GetUseSize() < len) {
        len = GetUseSize();
    }
    int fLen = size - front;
    int sLen;
    if (fLen > len) {
        fLen = len;
    }
    sLen = len - fLen;
    memcpy(msg, ringBuf + front,fLen);
    if (sLen > 0) {
        memcpy(msg + fLen, ringBuf, sLen);
    }
    front = (front + len) % size;
    return len;
}
void RingBuffer::MoveFront(int len) {
    front = (front + len) % size;
}
void RingBuffer::MoveRear(int len) {
    rear = (rear + len) % size;
}

void RingBuffer::ClearBuffer() {
    front = rear = 0;
    memset(ringBuf, ' ', size);
}

int RingBuffer::Peek(char* msg, int len) {
    if (GetUseSize() < len) {
        len = GetUseSize();
    }
    int fLen = size - front;
    int sLen;
    if (fLen > len) {
        fLen = len;
    }
    sLen = len - fLen;
    memcpy(msg, ringBuf + front, fLen);
    if (sLen > 0) {
        memcpy(msg + fLen, ringBuf, sLen);
    }
    return len;
}

int RingBuffer::DirectEnqueue() {
    if (GetFreeSize() == 0) {
        return 0;
    }
    else if (front <= rear) {
        return size - rear;
    }
    else {
        return front - rear - 1;
    }
}

int RingBuffer::DirectDequeue() {
    if (front <= rear) {
        return rear - front;
    }
    else {
        return size - front;
    }
}

char* RingBuffer::GetBufferFront() {
    return (ringBuf+front);
}
char* RingBuffer::GetBufferRear() {
    return (ringBuf + rear);
}