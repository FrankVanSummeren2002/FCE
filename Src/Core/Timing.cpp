#include "Core/FCEpch.h"
#include "Core/Timing.h"
using namespace FCE;
int gTimerIndex = 0;
std::vector<Timer*> gTimers;

Timer::Timer(int index, float Time): mIndex{index},mTime{Time},mTimer{Time}
{
}

Timer::~Timer()
{
}

bool Timer::IsOver()
{
	return mTimer < 0;
}

void Timer::Reset()
{
	mTimer = mTime;
}

Timer* TimeHandler::CreateTimer(float Time)
{
	gTimerIndex++;
	Timer* time = new Timer(gTimerIndex, Time);
	gTimers.push_back(time);
	return time;
}

void TimeHandler::Update(float DT)
{
	for (auto it : gTimers)
	{
		it->mTimer -= DT;
		if (it->mTimer < 0)
		{
			if (it->callback)
				it->callback();
		}
	}
}

void TimeHandler::DeleteTimer(Timer* timer)
{
	for (int i = 0; i < gTimers.size(); i++)
	{
		if (timer->mIndex == gTimers[i]->mIndex)
		{
			gTimers.erase(gTimers.begin() + i);
			delete timer;
			return;
		}
	}
}
