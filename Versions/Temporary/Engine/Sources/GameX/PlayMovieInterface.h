
#pragma once

#include "InterfaceScreenBase.h"

//#include "..\Scene\Scene.h"

class CPlayMovieInterface : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CPlayMovieInterface );

	ZDATA_( CInterfaceScreenBase )
	// next command
	std::string szNextCommand;
	CPtr<IPlayer> pPlayerControl;
	bool bProlog;
	bool bFadeIn;
	bool bFadeOut;
	std::string szFileName;
	CPtr<IWindow> pBackground;
	float fFadeIn;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &szNextCommand );
		f.Add( 3, &pPlayerControl );
		f.Add( 4, &bProlog );
		f.Add( 5, &bFadeIn );
		f.Add( 6, &bFadeOut );
		f.Add( 7, &szFileName );
		f.Add( 8, &pBackground );
		f.Add( 9, &fFadeIn );
		return 0;
	}

private:
	void StartNextInterface();
	bool StepLocal( bool bAppActive ) override;
	// messages processing
	void MsgSkipSequence( const struct SGameMessage &msg );
	void MsgSkipMovie( const struct SGameMessage &msg );
	~CPlayMovieInterface() override;

public:
	CPlayMovieInterface();
	//
	void LoadMovieSequence( const std::string &szFileName );
	void SetNextInterface( const std::string &szNextCommand );
	//
	bool Init() override;
	void OnGetFocus( bool bFocus ) override;
};

class CICPlayMovie : public CInterfaceCommandBase<CPlayMovieInterface>
{
	OBJECT_NOCOPY_METHODS( CICPlayMovie );
	//
	std::string szSequenceName;
	std::string szNextCommand;

	//
	void PreCreate() override {}

	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
	//
	int operator&( IBinSaver &saver ) override;
};
