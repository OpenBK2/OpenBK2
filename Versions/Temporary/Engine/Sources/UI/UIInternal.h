#pragma once

////#include "UIMain.h"

// broadcast message (old fashioned), visits all windows untill some of them processed it
struct SBUIMessage
{
	string szMessageID;							// message ID.
	string szParam;									// string parameter
	int nParam;														// int parameter
	
	SBUIMessage() {  }
	SBUIMessage( const string &_szMessageID ) : szMessageID( _szMessageID ), nParam( 0 ) { }
	SBUIMessage( const string &_szMessageID, const string &_szParam ) 
		: szMessageID( _szMessageID ), szParam( _szParam ), nParam( 0 ) { }
	SBUIMessage( const string &_szMessageID, const string &_szParam, const int _nParam ) 
		: szMessageID( _szMessageID ), szParam( _szParam ), nParam( _nParam ) { }
	
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &szMessageID );
		saver.Add( 2, &szParam );
		saver.Add( 3, &nParam );
		return 0;
	}
};



