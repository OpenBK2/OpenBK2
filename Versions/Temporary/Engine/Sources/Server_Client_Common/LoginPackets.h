#pragma once

#include "NetPacket.h"

class CLoginPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLoginPacket );
public:
	ZDATA
		string szNick;
		string szPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&szPassword); return 0; }

	CLoginPacket() { }
	CLoginPacket( const int nClient, const string &_szNick, const string &_szPassword )
		: CNetPacket( nClient ), szNick( _szNick ), szPassword( _szPassword ) {}
};

class CRegisterPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CRegisterPacket )
public:
	ZDATA
		string szNick;
		string szPassword;
		string szCDKey;
		string szEmail;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&szPassword); f.Add(4,&szCDKey); f.Add(5,&szEmail); return 0; }

	CRegisterPacket() { }
	CRegisterPacket( const int nClient, const string &_szNick, const string &_szPassword, const string &_szCDKey, const string &_szEmail )
		: CNetPacket( nClient ), szNick( _szNick ), szPassword( _szPassword ), szCDKey( _szCDKey ), szEmail( _szEmail ) {}
};

class CForgottenPasswordPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CForgottenPasswordPacket )
public:
	ZDATA
		string szNick;
		string szEMail;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&szEMail); return 0; }

	CForgottenPasswordPacket() {}
	CForgottenPasswordPacket( const string &_szNick, const string &_szEMail )
		: CNetPacket( 0 ), szNick( _szNick ), szEMail( _szEMail ) {}
};

class CForgottenPasswordAnswerPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CForgottenPasswordAnswerPacket )
public:
	enum EAnswer{ OK = 0, NICK_NOT_REGISTERED = 1, INVALID_EMAIL = 2, SORRY_SERVICE_IS_NOT_IMPLEMENTED_YET = 255 };
	ZDATA
		EAnswer eAnswer;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eAnswer); return 0; }

	CForgottenPasswordAnswerPacket() {}
	CForgottenPasswordAnswerPacket( const int nClient, const EAnswer _eAnswer )
		: CNetPacket( nClient ), eAnswer( _eAnswer ) {}
};

// packets for internal use!

class CCheckConnectPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CCheckConnectPacket )
public:
	ZDATA
		__int64 nNumber;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nNumber); return 0; }

	CCheckConnectPacket() { }
	CCheckConnectPacket( const __int64 _nNumber ) : nNumber( _nNumber ) { }
};

class CCheckConnectAnswerPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CCheckConnectAnswerPacket )
public:
	enum EConnectType { ECT_LOGIN, ECT_REGISTER };
	ZDATA
		int nPrime1, nPrime2;
		string szNick;
		string szPassword;
		string szCDKey;
		string szEmail;
		EConnectType eConnectType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nPrime1); f.Add(3,&nPrime2); f.Add(4,&szNick); f.Add(5,&szPassword); f.Add(6,&szCDKey); f.Add(7,&szEmail); f.Add(8,&eConnectType); return 0; }

	CCheckConnectAnswerPacket() { }
	CCheckConnectAnswerPacket( const int _nPrime1, const int _nPrime2,
			const EConnectType _eConnectType, 
			const string &_szNick, const string &_szPassword, const string &_szCDKey, const string &_szEmail )
		: nPrime1( _nPrime1 ), nPrime2( _nPrime2 ),
			eConnectType( _eConnectType ), szNick( _szNick ), szPassword( _szPassword ),
			szCDKey( _szCDKey ), szEmail( _szEmail ) { }
};


