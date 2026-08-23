#pragma once

#include "Main_export.h"

#include "System/FilePath.h"

#define MOD_MODE_GAME "game"
#define MOD_MODE_EDITOR "editor"

struct IXmlSaver;
namespace NDb
{
	enum EDatabaseMode : int;
}
namespace NMOD
{

struct SMOD
{
	ZDATA
	std::wstring wszName;											// MOD short name
	std::wstring wszDesc;											// MOD long description
	NFile::CFilePath szFullFolderPath;		// full folder path to MOD
	NFile::CFilePath szRelativePath;			// path, relative to MODs base
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszName); f.Add(3,&wszDesc); f.Add(4,&szFullFolderPath); f.Add(5,&szRelativePath); return 0; }
};

//! get list of all available mods
MAIN_EXPORT void GetAllMODs( std::vector<SMOD> *pMODs );
//! instantly attach mod (in game use it only for start)
MAIN_EXPORT void InstantAttachMOD( const NFile::CFilePath &path, NDb::EDatabaseMode _eMode );
//! correctly attach mod through game mechanisms
MAIN_EXPORT void AttachMOD( const NFile::CFilePath &path );
//! detach all mods
MAIN_EXPORT void DetachAllMODs();
//! check, does specified mod attached
MAIN_EXPORT bool DoesMODAttached( const NFile::CFilePath &path );
//! check, does any mod attached
MAIN_EXPORT bool DoesAnyMODAttached();
//! get currently attached MOD descriptor
MAIN_EXPORT bool GetAttachedMOD( SMOD *pMOD );

enum EModSetupClassIDs
{
	MOD_PRE_SHUTDOWN = 0x10278340,
	MOD_POST_SETUP = 0x10278341,
};

}
