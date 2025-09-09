#include "Timer.h"

typedef struct PROFILE_DATA {
	LARGE_INTEGER RunningTime;

	unsigned __int64 TotalTime;
	unsigned __int64 TimeMin[2];
	unsigned __int64 TimeMax[2];
	unsigned __int64 CallTime;
	PROFILE_DATA(LARGE_INTEGER time):RunningTime(time), TotalTime(0), CallTime(1){
		TimeMin[0] = 0xffffffffffffffff;
		TimeMin[1] = 0xffffffffffffffff;
		TimeMax[0] = 0;
		TimeMax[1] = 0;
	}
}pData;

std::map<std::string, pData> pMap;

LARGE_INTEGER Start;
LARGE_INTEGER End;
LARGE_INTEGER Freq;

Timer::Timer(const char* name) {
	PRO_BEGIN(name);
	strcpy_s(funcName, 64, name);
}
Timer::	~Timer() {
	PRO_END(funcName);
}

void TimerBegin(const char* name) {
	QueryPerformanceCounter(&Start);
	std::map<std::string, pData>::iterator it;
	it = pMap.find(name);
	if (it == pMap.end()) {
		pMap.insert({ name, pData(Start) });
	}
	else {
		it->second.RunningTime = Start;
		it->second.CallTime++;
	}
}

void TimerEnd(const char* name) {
	QueryPerformanceCounter(&End);
	std::map<std::string, pData>::iterator it;
	it = pMap.find(name);
	__int64 time = End.QuadPart - it->second.RunningTime.QuadPart;
	it->second.TotalTime += time;
	if (time > it->second.TimeMax[1]) {
		if (time > it->second.TimeMax[0]) {
			it->second.TimeMax[1] = it->second.TimeMax[0];
			it->second.TimeMax[0] = time;
		}
		else {
			it->second.TimeMax[1] = time;
		}
	}
	if (time < it->second.TimeMin[1]) {
		if (time < it->second.TimeMin[0]) {
			it->second.TimeMin[1] = it->second.TimeMin[0];
			it->second.TimeMin[0] = time;
		}
		else {
			it->second.TimeMin[1] = time;
		}
	}
}

void PrintProfileData() {
	FILE* fp;
	QueryPerformanceFrequency(&Freq);
	fopen_s(&fp, "PROFILE_DATA.txt", "w");
	fprintf(fp, "%20s  |%15s  |%15s  |%15s  |%10s  |\n", "Name","Average","Min","Max","Call");
	fprintf(fp, "--------------------------------------------------------------------------------------\n");
	for (auto it : pMap) {
		__int64 totalTime = it.second.TotalTime - it.second.TimeMin[0] - it.second.TimeMin[1] - it.second.TimeMax[0] - it.second.TimeMax[1];
		float average = totalTime / (float)(it.second.CallTime - 4);
		//average /= (it.second.CallTime - 2);
		fprintf(fp, "%-20s  |%15.6lf  |%15.6lf  |%15.6lf  |%10lld  |\n", it.first.c_str(), average / ((double)Freq.QuadPart / 1000000), it.second.TimeMin[0] / ((double)Freq.QuadPart / 1000000), it.second.TimeMax[0] / ((double)Freq.QuadPart / 1000000), it.second.CallTime);
	}
	fclose(fp);
}