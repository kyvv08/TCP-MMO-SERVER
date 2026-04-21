#pragma once
#include <Windows.h>
#include <type_traits>

#define SERIAL_BUFFER_FULL -1
#define NO_DATA_IN_BUF -2

class SerialBuffer {
public:
    enum BufferDefaults {
        DEFAULT_SIZE = 32
    };

    SerialBuffer();
    SerialBuffer(int size);
    virtual ~SerialBuffer();

    void Initialize();
    void Clear();

    int GetCapacity() const;
    int GetLength() const;
    char* GetBufferPtr();
    char* GetBufferWritePtr();

    void MoveWritePos(int len);
    void MoveReadPos(int len);
    void SetDataLength(int len);

    int Read(char* pDest, int len);
    int Write(const char* pSrc, int len);

    // Template operators (must remain in header)
    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    SerialBuffer& operator << (const T& tValue) {
        if (m_Capacity - m_Length < (int)sizeof(T)) {
            throw(SERIAL_BUFFER_FULL);
        }
        Write(reinterpret_cast<const char*>(&tValue), sizeof(T));
        return *this;
    }

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    SerialBuffer& operator << (T& tValue) {
        return *this << static_cast<const T&>(tValue);
    }

    SerialBuffer& operator << (const char* pStr);
    SerialBuffer& operator << (const WCHAR* pWStr);

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    SerialBuffer& operator >> (T& tValue) {
        if (m_Length < (int)sizeof(T)) {
            throw(NO_DATA_IN_BUF);
        }
        Read(reinterpret_cast<char*>(&tValue), sizeof(T));
        return *this;
    }

    // Explicit char/WCHAR reading support could be added if needed
    SerialBuffer& operator >> (char* pDest);
    SerialBuffer& operator >> (WCHAR* pDest);

protected:
    int m_Capacity;
    int m_Length;

private:
    char* m_pBuf;
    int m_WritePos;
    int m_ReadPos;
};