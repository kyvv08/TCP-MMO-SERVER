#pragma once
#include <Windows.h>
#include <type_traits>

#define SERIAL_BUFFER_FULL -1
#define NO_DATA_IN_BUF -2


class SerialBuffer
{
public:
	enum BUFFER_SIZE {
		SIZE_DEFAULT = 32//8192 -- 이 서버 전용 직렬화 버퍼
	};
	inline SerialBuffer();
	inline SerialBuffer(int);

	inline void Init();

	inline virtual ~SerialBuffer();

	inline void Clear();
	inline int GetBufferSize();
	inline int GetDataSize();
	inline char* GetBufferPtr();
	inline char* GetBufferWritePtr();

	inline int MoveWritePos(int);
	inline int MoveReadPos(int);

	inline void SetDataSize(int);

	inline int GetData(char*, int);
	inline int PutData(char*, int);

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	inline SerialBuffer& operator << (T& tValue);

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	inline SerialBuffer& operator << (const T& tValue);

	inline SerialBuffer& operator << (const char* tValue);

	inline SerialBuffer& operator << (const WCHAR* tValue);


	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	inline SerialBuffer& operator >> (T& tValue);

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	inline SerialBuffer& operator>> (const T& tValue);

	inline SerialBuffer& operator>> (const char* tValue);

	inline SerialBuffer& operator>> (const WCHAR* tValue);

protected:
	int bufSize;
	int dataSize;
private:
	char* buf;
	int write;
	int read;
};

SerialBuffer::SerialBuffer() :write(0), read(0), bufSize(SIZE_DEFAULT), dataSize(0) {
	buf = new char[SIZE_DEFAULT];
}

SerialBuffer::SerialBuffer(int size) :write(0), read(0), bufSize(size), dataSize(0) {
	buf = new char[size];
}

SerialBuffer::~SerialBuffer() {
	delete buf;
}

void SerialBuffer::Init() {
	//memset(buf, 0, dataSize);
	write = read = dataSize = 0;
}

void SerialBuffer::Clear() {
	//memset(buf, 0, dataSize);
	write = read = dataSize = 0;
}

int SerialBuffer::GetBufferSize() {
	return bufSize;
}

int SerialBuffer::GetDataSize() {
	return dataSize;
}

void SerialBuffer::SetDataSize(int size) {
	dataSize = size;
}

char* SerialBuffer::GetBufferPtr() {
	return buf;
}

char* SerialBuffer::GetBufferWritePtr() {
	return buf + write;
}

int SerialBuffer::MoveWritePos(int len) {
	write += len;
	return len;
}

int SerialBuffer::MoveReadPos(int len) {
	read += len;
	return len;
}


int SerialBuffer::GetData(char* data, int len) {
	if (dataSize < len)return -1;
	memcpy(data, buf + read, len);
	dataSize -= len;
	read += len;
	return len;
}

int SerialBuffer::PutData(char* data, int len) {
	if (bufSize - dataSize < len)return -1;
	memcpy(buf + write, data, len);
	dataSize += len;
	write += len;
	return len;
}

//------------------------------------
//----------------넣기----------------
//------------------------------------

template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
SerialBuffer& SerialBuffer::operator << (T& tValue) {
	if (bufSize - dataSize < sizeof(T)) {
		throw(SERIAL_BUFFER_FULL);
	}
	PutData((char*)&tValue, sizeof(T));
	return *this;
}

template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
SerialBuffer& SerialBuffer::operator << (const T& tValue) {
	if (bufSize - dataSize < sizeof(T)) {
		throw(SERIAL_BUFFER_FULL);
	}
	PutData((char*)&tValue, sizeof(T));
	return *this;
}

SerialBuffer& SerialBuffer::operator << (const char* tValue) {
	if (tValue == NULL) return *this;
	int len = strlen(tValue);
	if (bufSize - dataSize < len) {
		throw(SERIAL_BUFFER_FULL);
	}
	PutData((char*)tValue, len);
	return *this;
}

SerialBuffer& SerialBuffer::operator << (const WCHAR* tValue) {
	if (tValue == NULL) return *this;
	int len = wcslen(tValue) * 2;
	if (bufSize - dataSize < len) {
		throw(SERIAL_BUFFER_FULL);
	}
	PutData((char*)tValue, len);
	return *this;
}

//------------------------------------
//----------------빼기----------------
//------------------------------------

template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
SerialBuffer& SerialBuffer::operator >> (T& tValue) {
	if (dataSize < sizeof(T)) {
		throw(NO_DATA_IN_BUF);
	}
	GetData((char*)&tValue, sizeof(T));
	return *this;
}

template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int>>
SerialBuffer& SerialBuffer::operator >> (const T& tValue) {
	if (dataSize < sizeof(T)) {
		throw(NO_DATA_IN_BUF);
	}
	GetData((char*)&tValue, sizeof(T));
	return *this;
}

SerialBuffer& SerialBuffer::operator >> (const char* tValue) {
	GetData((char*)tValue, dataSize);
	return *this;
}

SerialBuffer& SerialBuffer::operator >> (const WCHAR* tValue) {
	GetData((char*)tValue, dataSize);
	return *this;
}