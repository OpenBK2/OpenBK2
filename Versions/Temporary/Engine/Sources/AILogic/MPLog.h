#ifndef __MP_LOG_H__
#define __MP_LOG_H__

#pragma ONCE

extern NTimer::STime timeToLogStart;
extern NTimer::STime timeToLogFinish;

extern NTimer::STime curTime;

// non-reentrant
inline void MPLog( const char *pszInfo, ... )
{
  static char buff[2048];

	if ( curTime >= timeToLogStart && curTime <= timeToLogFinish )
	{
		va_list va;
		// 
		va_start( va, pszInfo );
		vsprintf( buff, pszInfo, va );
		va_end( va );
		
		CONSOLE_BUFFER_LOG2( 10, buff, 0, true );
	}
}

#endif // __MP_LOG_H__

