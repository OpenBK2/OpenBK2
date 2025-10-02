#if !defined(__INTERFACE__PROGRESS__)
#define __INTERFACE__PROGRESS__
#pragma once


namespace NProgress
{
	void Create( bool bShow );
	void Destroy();

	void SetTitle( const string &rszTitle );
	void SetMessage( const string &rszMessage );
	void SetRange( int nStart, int nFinish );
	void SetPosition( int nPosition );
	void IteratePosition();
};

#endif // !defined(__INTERFACE__PROGRESS__)


