#pragma once

#include <iostream>
#include <map>
#include <windows.h>

#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName)	TimerBegin(TagName)
#define PRO_END(TagName)	TimerEnd(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

class Timer
{
private:
	char funcName[64];
public:
	Timer(const char*);
	~Timer();

};
void PrintProfileData();
void TimerBegin(const char*);
void TimerEnd(const char*);