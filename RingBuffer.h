#pragma once

#define BUF_SIZE 129
//size는 실제 사용 가능한 크기, 버퍼의 메모리 크기는 size + 1
class RingBuffer {
private:
	char* ringBuf;
	int front,rear,size;
public:
	RingBuffer(void);
	RingBuffer(int);
	~RingBuffer(void);

	int GetBufferSize() { return size; }
	int GetUseSize();
	int GetFreeSize();

	void ClearBuffer();

	void MoveRear(int len);
	void MoveFront(int len);

	int Enqueue(char*, int);
	int Dequeue(char*, int);
	int Peek(char*,int);

	int DirectEnqueue(void);
	int DirectDequeue(void);

	char* GetBufferFront(void);
	char* GetBufferRear(void);
};