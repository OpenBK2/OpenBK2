#pragma once


class CAITimer
{
	static struct IGameTimer *pTimer;
public:
	CAITimer();
	static NTimer::STime GetSegmentTime();
	static NTimer::STime GetGameTime(); // Р А С Х О Д И Т С Я ! ! ! ! ! ТОЛЬКО ДЛЯ Updater'а
	static void ToClientTime( NTimer::STime *pTime );
	static void SetSpeed( int nSpeed );
};

