#include "StdAfx.h"
#include "../Main/MainLoop.h"
#include "../System/VFSOperations.h"
#include "../System/DG.h"
#include "../libdb/Db.h"
#include "../SceneB2/Scene.h"
#include "../Main/MODs.h"
#include "../Sound/SoundScene.h"
#include "../3Dmotor/Locale.h"
#include "../3Dmotor/GLocale.h"
#include "GetConsts.h"
#include "DBGameRoot.h"

namespace NGameX
{
	void PostStorageInitialize();
	void InitDataDependentSingletons();
	void UnInitDataDependentSingletons();
}

namespace NMOD
{

class CICMODPreShutdown : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICMODPreShutdown );
	//
	void Exec()
	{
		NMainLoop::ResetStack();
		//
		if ( IScene *pScene = Scene() )
			pScene->Clear();
		if ( ISoundScene *pSoundScene = Singleton<ISoundScene>() )
			pSoundScene->ClearSounds();
		if ( NGScene::CTextLocaleInfo *pLocale = NGScene::GetTextLocaleInfo() )
			pLocale->ClearAllFonts();
		//
		NGameX::UnInitDataDependentSingletons();
		ClearHoldQueue();
		NDb::CloseDatabase();
	}
};

class CICMODPostSetup : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICMODPostSetup );
	//
	NDb::EDatabaseMode eMode;
	//
public:
	CICMODPostSetup(): eMode( NDb::DATABASE_MODE_GAME ) {}
	void Exec()
	{
		NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), eMode );
		NGameX::InitDataDependentSingletons();
		NGameX::PostStorageInitialize();
		if ( NGScene::CTextLocaleInfo *pLocale = NGScene::GetTextLocaleInfo() )
		{
			if ( const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot() )
			{
				for ( int i = 0; i < pGameRoot->fonts.size(); ++i )
					pLocale->AddFont( pGameRoot->fonts[i] );
			}
		}
		if ( ISoundScene *pSoundScene = Singleton<ISoundScene>() )
			pSoundScene->ClearSounds();
	}
	void Configure( const char *pszConfig )
	{
		if ( pszConfig != 0 )
		{
			if ( strcmp(pszConfig, MOD_MODE_GAME) == 0 )
				eMode = NDb::DATABASE_MODE_GAME;
			else if ( strcmp(pszConfig, MOD_MODE_EDITOR) == 0 )
				eMode = NDb::DATABASE_MODE_EDITOR;
		}
	}
};

}

using namespace NMOD;
REGISTER_SAVELOAD_CLASS( MOD_PRE_SHUTDOWN, CICMODPreShutdown );
REGISTER_SAVELOAD_CLASS( MOD_POST_SETUP, CICMODPostSetup );
