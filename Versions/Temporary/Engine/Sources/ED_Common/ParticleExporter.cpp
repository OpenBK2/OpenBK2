#include "stdafx.h"

#include "3Dmotor/GParticleFormat.h"
#include "MapEditorLib/CommonExporterMethods.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/MessageBoxes.h"
#include "MapEditorLib/SourceFilePath.h"
#include "Misc/StrProc.h"
#include "ParticleExporter.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <cmath>
#include <cstring>
#include <filesystem>

REGISTER_EXPORTER_IN_DLL( Particle, CParticleExporter )

std::string CParticleExporter::ImportClip( IManipulator *pManipulator,
                                           const std::string &source,
                                           NVFS::IFileCreator *destination,
                                           bool force )
{
	std::string extension = std::filesystem::u8path( source ).extension().u8string();
	NStr::ToLowerASCII( &extension );
	if ( extension == ".mb" || extension == ".ma" )
	{
		return "Maya particle export is no longer supported. Set SrcName to a baked .ptc file.";
	}
	if ( extension != ".ptc" )
	{
		return "Set SrcName to a BK2 particle clip (.ptc).";
	}

	CFileStream stream( source, CFileStream::WIN_READ_ONLY );
	if ( !stream.IsOk() )
	{
		return "Cannot read " + source;
	}
	CObj<NGScene::CParticlesInfo> clip = new NGScene::CParticlesInfo;
	if ( !clip->ReadClip( stream ) || clip->nParticles == 0 || clip->fTEnd <= 0 )
	{
		return "Invalid or empty BK2 particle clip.";
	}
	SBound bound;
	clip->CalcBound( &bound );
	if ( !std::isfinite( bound.s.fRadius ) || !std::isfinite( bound.s.ptCenter.x ) ||
	     !std::isfinite( bound.s.ptCenter.y ) || !std::isfinite( bound.s.ptCenter.z ) )
	{
		return "Particle bounds exceed the supported range.";
	}

	const std::string path = BuildDestFilePath( pManipulator, "bin/Effects/" );
	bool write = true;
	if ( !force )
	{
		// SrcName may have changed to an older file, so compare bytes, not timestamps.
		CFileStream installed( NVFS::GetWritePath( destination, path ), CFileStream::WIN_READ_ONLY );
		write = !installed.IsOk() || installed.GetSize() != stream.GetSize() ||
		        memcmp( installed.GetBuffer(), stream.GetBuffer(), stream.GetSize() ) != 0;
	}
	// Calculate Bound from the same bytes we install. A cached UID resource may
	// still describe the previous clip. Restore the metadata if replacement fails.
	CVariant previous;
	const CVariant next( &bound, sizeof( bound ) );
	if ( !pManipulator->GetValue( "Bound", &previous ) )
	{
		return "Cannot read the particle Bound.";
	}
	const bool changed = !( previous == next );
	if ( changed && !pManipulator->SetValue( "Bound", next ) )
	{
		return "Cannot update the particle Bound.";
	}
	std::string error;
	if ( write && !NVFS::WriteFile( destination, path, stream, &error ) )
	{
		if ( changed )
		{
			pManipulator->SetValue( "Bound", previous );
		}
		return error;
	}
	return {};
}

EXPORT_RESULT CParticleExporter::ExportObject( IManipulator *pManipulator,
                                               const std::string &rszObjectTypeName,
                                               const std::string &rszObjectName,
                                               bool bForce,
                                               EXPORT_TYPE exportType )
{
	if ( exportType != ET_BEFORE_REF && exportType != ET_NO_REF )
	{
		return ER_SUCCESS;
	}
	std::string source;
	CManipulatorManager::GetValue( &source, pManipulator, "SrcName" );
	const std::string root = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION );
	CObj<NVFS::IFileCreator> creator = NVFS::CreateWinFileCreator( root );
	const std::string error = ImportClip( pManipulator, NEditorSource::Resolve( source ), creator, bForce );
	if ( !error.empty() )
	{
		const std::string message = rszObjectName + "\n" + source + "\n" + error;
		NLog::Log( LT_ERROR, "%s\n", message.c_str() );
		NMessage::Error( message, "Particle export" );
		return ER_FAIL;
	}
	return ER_SUCCESS;
}
