#pragma once


namespace NProgress
{
	void Create( bool bShow );
	void Destroy();

	void SetTitle( const std::string &rszTitle );
	void SetMessage( const std::string &rszMessage );
	void SetRange( int nStart, int nFinish );
	void SetPosition( int nPosition );
	void IteratePosition();
};



