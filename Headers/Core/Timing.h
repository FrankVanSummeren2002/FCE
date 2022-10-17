#pragma once
namespace FCE
{
	struct Timer
	{
		int mIndex{ 0 };
		float mTimer{ 0 };
		float mTime{ 0 };
		std::function<void()> callback;
		Timer() {};
		Timer(int index, float Time);
		~Timer();
		bool IsOver();
		void Reset();
	};

	class TimeHandler
	{

	public:
		static Timer* CreateTimer(float Time);
		static void Update(float DT);
		static void DeleteTimer(Timer* timer);
	};
}

