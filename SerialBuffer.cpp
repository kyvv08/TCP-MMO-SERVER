#include "SerialBuffer.h"
#include <cstring>

SerialBuffer::SerialBuffer() 
    : m_WritePos(0), m_ReadPos(0), m_Capacity(DEFAULT_SIZE), m_Length(0) {
    m_pBuf = new char[DEFAULT_SIZE];
}

SerialBuffer::SerialBuffer(int size) 
    : m_WritePos(0), m_ReadPos(0), m_Capacity(size), m_Length(0) {
    m_pBuf = new char[size];
}

SerialBuffer::~SerialBuffer() {
    delete[] m_pBuf;
}

void SerialBuffer::Initialize() {
    m_WritePos = m_ReadPos = m_Length = 0;
}

void SerialBuffer::Clear() {
    m_WritePos = m_ReadPos = m_Length = 0;
}

int SerialBuffer::GetCapacity() const {
    return m_Capacity;
}

int SerialBuffer::GetLength() const {
    return m_Length;
}

void SerialBuffer::SetDataLength(int len) {
    m_Length = len;
}

char* SerialBuffer::GetBufferPtr() {
    return m_pBuf;
}

char* SerialBuffer::GetBufferWritePtr() {
    return m_pBuf + m_WritePos;
}

void SerialBuffer::MoveWritePos(int len) {
    m_WritePos += len;
}

void SerialBuffer::MoveReadPos(int len) {
    m_ReadPos += len;
}

int SerialBuffer::Read(char* pDest, int len) {
    if (m_Length < len) return -1;
    memcpy(pDest, m_pBuf + m_ReadPos, len);
    m_Length -= len;
    m_ReadPos += len;
    return len;
}

int SerialBuffer::Write(const char* pSrc, int len) {
    if (m_Capacity - m_Length < len) return -1;
    memcpy(m_pBuf + m_WritePos, pSrc, len);
    m_Length += len;
    m_WritePos += len;
    return len;
}

SerialBuffer& SerialBuffer::operator << (const char* pStr) {
    if (pStr == nullptr) return *this;
    int len = static_cast<int>(strlen(pStr));
    if (m_Capacity - m_Length < len) {
        throw(SERIAL_BUFFER_FULL);
    }
    Write(pStr, len);
    return *this;
}

SerialBuffer& SerialBuffer::operator << (const WCHAR* pWStr) {
    if (pWStr == nullptr) return *this;
    int len = static_cast<int>(wcslen(pWStr) * sizeof(WCHAR));
    if (m_Capacity - m_Length < len) {
        throw(SERIAL_BUFFER_FULL);
    }
    Write(reinterpret_cast<const char*>(pWStr), len);
    return *this;
}

SerialBuffer& SerialBuffer::operator >> (char* pDest) {
    Read(pDest, m_Length);
    return *this;
}

SerialBuffer& SerialBuffer::operator >> (WCHAR* pDest) {
    Read(reinterpret_cast<char*>(pDest), m_Length);
    return *this;
}
