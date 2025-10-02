#include "StdAfx.h"
#include "MODs.h"
#include "../Main/MainLoop.h"
#include "../System/FileUtils.h"
#include "../System/VFSOperations.h"
#include "../System/WinVFS.h"
#include "../System/Text.h"
#include "../Misc/StrProc.h"
#include "../libdb/Db.h"

#define MOD_ATTACH_COMMAND 0x10268440

namespace NGameX
{
	void PostStorageInitialize();
	void InitDataDependentSingletons();
	void UnInitDataDependentSingletons();
}

namespace NMOD
{

void ReadMODInfo( SMOD *pMOD, const string &_szFullFolderName )
{
	string szFullFolderName = _szFullFolderName;
	if ( !szFullFolderName.empty() && szFullFolderName[szFullFolderName.size() - 1] != '\\' )
		szFullFolderName += '\\';
	//
	pMOD->szFullFolderPath = szFullFolderName;
	NFile::MakeRelativePath( &pMOD->szRelativePath, szFullFolderName, NMainLoop::GetBaseDir() + "MODs\\" );
	// name
	{
		CFileStream stream( szFullFolderName + "name.txt", CFileStream::WIN_READ_ONLY );
		NText::LoadUnicodeText( &pMOD->wszName, &stream );
	}
	// description
	{
		CFileStream stream( szFullFolderName + "desc.txt", CFileStream::WIN_READ_ONLY );
		NText::LoadUnicodeText( &pMOD->wszDesc, &stream );
	}
}

void GetAllMODs( vector<SMOD> *pMODs )
{
	const string szMODsBaseDir = NMainLoop::GetBaseDir() + "Mods\\";
	// iterate through all files by mask
	for ( NFile::CFileIterator it( (szMODsBaseDir + "*.*").c_str() ); !it.IsEnd(); ++it )
	{
		if ( it.IsDirectory() && !it.IsDots() )
		{
			SMOD mod;
			ReadMODInfo( &mod, it.GetFullName() );
			// check and add MOD to list
			if ( !mod.wszName.empty() )
				pMODs->push_back( mod );
		}
	}
}

void AttachMODInternal( const NFile::CFilePath &szBaseResourcePath, const NFile::CFilePath &szMODPath, NDb::EDatabaseMode eMode )
{
	NVFS::IVFS *pVFS = NVFS::GetMainVFS();
	NVFS::ICombinerVFS *pMainVFS = pVFS == 0 ? 0 : dynamic_cast<NVFS::ICombinerVFS *>( pVFS );
	CPtr<NVFS::IFileCreator> pMainFileCreator = NVFS::GetMainFileCreator();
	if ( pMainVFS == 0 )
		pMainVFS = NVFS::CreateCombinerVFS( pVFS );
	if ( pMainFileCreator == 0 )
		pMainFileCreator = NVFS::CreateWinFileCreator( szBaseResourcePath );
	//
	vector< CObj<NVFS::IVFS> > vfses = pMainVFS->GetVFSList();
	if ( vfses.empty() )
		vfses.push_back( NVFS::CreateWinVFS(szBaseResourcePath) );
	// remove all VFSes but one (base storage VFS)
	while ( vfses.size() > 1 )
		vfses.pop_back();
	// add new MOD VFS
	if ( !szMODPath.empty() && szMODPath != szBaseResourcePath )
	{
		pMainFileCreator = NVFS::CreateWinFileCreator( szMODPath );
		vfses.push_back( NVFS::CreateWinVFS(szMODPath) );
	}
	else if ( szMODPath.empty() )
		pMainFileCreator = NVFS::CreateWinFileCreator( szBaseResourcePath );
	//
	pMainVFS->SetVFSList( vfses );
	//
	NVFS::SetMainVFS( pMainVFS );
	NVFS::SetMainFileCreator( pMainFileCreator );
	//
	SMOD mod;
	if ( !szMODPath.empty() )
		ReadMODInfo( &mod, szMODPath );
	//
	NGlobal::SetVar( "current_attached_mod", mod.szRelativePath, STORAGE_USER );
	NGlobal::SetVar( "MOD.RelativePath", mod.szRelativePath, STORAGE_SAVE );
	NGlobal::SetVar( "MOD.FullPath", mod.szFullFolderPath, STORAGE_SAVE );
	NGlobal::SetVar( "MOD.Name", mod.wszName, STORAGE_SAVE );
	NGlobal::SetVar( "MOD.Desc", mod.wszDesc, STORAGE_SAVE );
}

class CICAttachMOD : public IInterfaceCommand
{
	OBJECT_BASIC_METHODS( CICAttachMOD );
	//
	NFile::CFilePath szFullFolderPath;
	NDb::EDatabaseMode eMode;
	//
	CICAttachMOD(): eMode( NDb::DATABASE_MODE_GAME ) {}
public:
	CICAttachMOD( NDb::EDatabaseMode _eMode ): eMode(_eMode) {}

	void Configure( const char *pszConfig ) 
	{ 
		szFullFolderPath = pszConfig == 0 ? "" : pszConfig; 
		if ( szFullFolderPath.size() > 1 && 
			   szFullFolderPath[szFullFolderPath.size() - 1] != '/' && 
				 szFullFolderPath[szFullFolderPath.size() - 1] != '\\' )
		{
			szFullFolderPath += "\\";
		}
	}
	//
	void Exec()
	{
		AttachMODInternal( NMainLoop::GetBaseDir() + "Data\\", szFullFolderPath, eMode );
	}
};

void InstantAttachMOD( const NFile::CFilePath &path, NDb::EDatabaseMode _eMode )
{
	const char *pszMODMode = _eMode == NDb::DATABASE_MODE_GAME ? MOD_MODE_GAME : MOD_MODE_EDITOR;
	if ( CPtr<IInterfaceCommand> pPreShutdownCmd = MakeObject<IInterfaceCommand>( MOD_PRE_SHUTDOWN ) )
	{
		pPreShutdownCmd->Configure( pszMODMode );
		pPreShutdownCmd->Exec();
	}
	//
	if ( CObj<IInterfaceCommand> pAttachMODCmd = new CICAttachMOD( _eMode ) )
	{
		pAttachMODCmd->Configure( path.c_str() );
		pAttachMODCmd->Exec();
	}
	//
	if ( CPtr<IInterfaceCommand> pPostSetupCmd = MakeObject<IInterfaceCommand>( MOD_POST_SETUP ) )
	{
		pPostSetupCmd->Configure( pszMODMode );
		pPostSetupCmd->Exec();
	}
}

NFile::CFilePath GetAbsolutePath( const NFile::CFilePath &path )
{
	if ( path.empty() )
		return path;
	if ( !IsPathRelative( path ) )
		return path;
	const string szMODsBaseDir = NMainLoop::GetBaseDir() + "Mods\\";
	NFile::CFilePath pathAbsolute;
	MakeFullPath( &pathAbsolute, path, szMODsBaseDir );
	NormalizePath( &pathAbsolute, pathAbsolute );
	return pathAbsolute;
}

void AttachMOD( const NFile::CFilePath &_path )
{
	NMainLoop::Command( MOD_PRE_SHUTDOWN, MOD_MODE_GAME );
	NMainLoop::Command( MOD_ATTACH_COMMAND, GetAbsolutePath(_path).c_str() );
	NMainLoop::Command( MOD_POST_SETUP, MOD_MODE_GAME );
}

void DetachAllMODs()
{
	NMainLoop::Command( MOD_PRE_SHUTDOWN, MOD_MODE_GAME );
	NMainLoop::Command( MOD_ATTACH_COMMAND, "" );
	NMainLoop::Command( MOD_POST_SETUP, MOD_MODE_GAME );
}

bool DoesMODAttached( const NFile::CFilePath &_path )
{
	NFile::CFilePath path = GetAbsolutePath( _path );
	const NFile::CFilePath szCurrentMODFullPath = NStr::ToMBCS( NGlobal::GetVar( "MOD.FullPath", "" ).GetString() );
	return szCurrentMODFullPath == path;
}

bool DoesAnyMODAttached()
{
	NVFS::ICombinerVFS *pMainVFS = dynamic_cast<NVFS::ICombinerVFS *>( NVFS::GetMainVFS() );
	return pMainVFS != 0 && pMainVFS->GetVFSList().size() > 1;
}

bool GetAttachedMOD( SMOD *pMOD )
{
	pMOD->szRelativePath = NStr::ToMBCS( NGlobal::GetVar( "MOD.RelativePath", "" ).GetString() );
	pMOD->szFullFolderPath = NStr::ToMBCS( NGlobal::GetVar( "MOD.FullPath", "" ).GetString() );
	pMOD->wszName = NGlobal::GetVar( "MOD.Name", "" ).GetString();
	pMOD->wszDesc = NGlobal::GetVar( "MOD.Desc", "" ).GetString();
	return !pMOD->szFullFolderPath.empty();
}

}
using namespace NMOD;
REGISTER_SAVELOAD_CLASS( MOD_ATTACH_COMMAND, CICAttachMOD );