//------------------------------------------------------------------------------------------------------------------------------
// Time.cpp
//------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------
#include "Time.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <time.h>

//------------------------------------------------------------------------------------------------------------------------------
double InitializeTime( LARGE_INTEGER& out_initialTime )
{
	LARGE_INTEGER countsPerSecond;
	QueryPerformanceFrequency( &countsPerSecond );
	QueryPerformanceCounter( &out_initialTime );
	return( 1.0 / static_cast< double >( countsPerSecond.QuadPart ) );
}

//------------------------------------------------------------------------------------------------------------------------------
double GetCurrentTimeSeconds()
{
	static LARGE_INTEGER initialTime;
	static double secondsPerCount = InitializeTime( initialTime );
	LARGE_INTEGER currentCount;
	QueryPerformanceCounter( &currentCount );
	LONGLONG elapsedCountsSinceInitialTime = currentCount.QuadPart - initialTime.QuadPart;

	double currentSeconds = static_cast< double >( elapsedCountsSinceInitialTime ) * secondsPerCount;
	return currentSeconds;
}

//------------------------------------------------------------------------------------------------------------------------------
uint64_t GetCurrentTimeHPC()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return *(uint64_t*)&li;
}

//------------------------------------------------------------------------------------------------------------------------------
double GetHPCToSeconds(uint64_t hpc)
{
	static LARGE_INTEGER initialTime;
	static double secondsPerCount = InitializeTime(initialTime);

	return (double)hpc * secondsPerCount;
}

//------------------------------------------------------------------------------------------------------------------------------
std::string GetDateTime()
{
	time_t rawTime;
	struct tm * timeInfo = new struct tm;
	char buffer[80];

	time(&rawTime);
	localtime_s(timeInfo, &rawTime);

	strftime(buffer, 80, "Run_%d-%m-%Y_%H-%M-%S", timeInfo);

	return std::string(buffer);
}