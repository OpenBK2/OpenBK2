#pragma once

////#include "UIMain.h"

// broadcast message (old fashioned), visits all windows untill some of them processed it
struct SBUIMessage
{
	std::string szMessageID;							// message ID.
	std::string szParam;									// string parameter
	int nParam;														// int parameter
	
	SBUIMessage() {  }
	SBUIMessage( const std::string &_szMessageID ) : szMessageID( _szMessageID ), nParam( 0 ) { }
	SBUIMessage( const std::string &_szMessageID, const std::string &_szParam )
		: szMessageID( _szMessageID ), szParam( _szParam ), nParam( 0 ) { }
	SBUIMessage( const std::string &_szMessageID, const std::string &_szParam, const int _nParam )
		: szMessageID( _szMessageID ), szParam( _szParam ), nParam( _nParam ) { }
	
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &szMessageID );
		saver.Add( 2, &szParam );
		saver.Add( 3, &nParam );
		return 0;
	}
};



