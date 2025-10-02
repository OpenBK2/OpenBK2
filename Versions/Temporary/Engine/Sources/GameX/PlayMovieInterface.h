#ifndef __PLAYMOVIEINTERFACE_H__
#define __PLAYMOVIEINTERFACE_H__

#pragma ONCE

#include "InterfaceScreenBase.h"
//#include "..\Scene\Scene.h"

class CPlayMovieInterface : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CPlayMovieInterface );

	ZDATA_(CInterfaceScreenBase)
	// next command
	string szNextCommand;
	CPtr<IPlayer> pPlayerControl;
	bool bProlog;
	bool bFadeIn;
	bool bFadeOut;
	string szFileName;
	CPtr<IWindow> pBackground;
	float fFadeIn;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&szNextCommand); f.Add(3,&pPlayerControl); f.Add(4,&bProlog); f.Add(5,&bFadeIn); f.Add(6,&bFadeOut); f.Add(7,&szFileName); f.Add(8,&pBackground); f.Add(9,&fFadeIn); return 0; }
private:
	void StartNextInterface();
	bool StepLocal( bool bAppActive );
	// messages processing
	void MsgSkipSequence( const struct SGameMessage &msg );
	void MsgSkipMovie( const struct SGameMessage &msg );
	~CPlayMovieInterface();
public:	
	CPlayMovieInterface();
	//
	void LoadMovieSequence( const string &szFileName );
	void SetNextInterface( const string &szNextCommand );
	//
	bool Init();
	void OnGetFocus( bool bFocus );
};

class CICPlayMovie : public CInterfaceCommandBase<CPlayMovieInterface>
{
	OBJECT_NOCOPY_METHODS( CICPlayMovie );
	//
	string szSequenceName;
	string szNextCommand;
	//
	void PreCreate() {}
	void PostCreate( IInterface *pInterface );
public:
	virtual void Configure( const char *pszConfig );
	//
	virtual int operator&( IBinSaver &saver );
};

#endif // __PLAYMOVIEINTERFACE_H__
