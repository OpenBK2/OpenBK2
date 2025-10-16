#include "stdafx.h"

#include "LoadImage.h"
#include "NoiseManager.h"
#include "Misc/StrProc.h"
#include "System/VFSOperations.h"

#include <fmt/format.h>

struct SLoadNoise
{
	std::string szFileName;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.Add( "FileName", &szFileName );
		return 0;
	}
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &szFileName );
		return 0;
	}
};

CNoiseManager::CNoiseManager()
{
	// load information about noises
	CFileStream stream( NVFS::GetMainVFS(), "Noises\\noises.xml" );
	NI_ASSERT( stream.IsOk(), "Can't load Noises\\noises.xml" );
	CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
	NI_ASSERT( pSaver, "Can't create XML saver for Noises\\noises.xml reading" );

	std::vector<SNoiseStored> vLoadNoises;

	pSaver->Add( "Noises", &vLoadNoises );

	noises.resize( vLoadNoises.size() );
	for ( int i = 0; i < noises.size(); ++i )
	{
		noises[i].szFileName = vLoadNoises[i].szFileName;
		noises[i].bLoaded = false;
	}
}

CNoiseAccessor CNoiseManager::GetNoise( unsigned int nNoiseNum )
{
	NI_ASSERT( nNoiseNum < noises.size(), fmt::format("Invalid noise number {} - available [0..{}]", nNoiseNum, noises.size() - 1) );
	if ( !noises[nNoiseNum].bLoaded )
		LoadNoise( nNoiseNum );

	return CNoiseAccessor( noises[nNoiseNum].noise );
}

CNoiseAccessor CNoiseManager::GetNoise( const std::string &_szName )
{
	std::string szName;
	NStr::ToLowerASCII( &szName, _szName );
	for ( int i = 0; i < noises.size(); ++i )
	{
		if ( noises[i].szFileName == szName ) 
			return GetNoise( i );
	}
	NI_ASSERT( false, fmt::format("Unknown noise \"{}\"", szName) );
	return GetNoise( 0 );
}

void CNoiseManager::LoadNoise( unsigned int nNoiseNum )
{
	CFileStream stream( NVFS::GetMainVFS(), noises[nNoiseNum].szFileName );
	NI_ASSERT( stream.IsOk(), fmt::format("Can't load noise: {}", noises[nNoiseNum].szFileName ) );

	LoadGrayTGAImage( &stream, noises[nNoiseNum].noise );

	NI_ASSERT( ( noises[nNoiseNum].noise.GetSizeX() == GetNextPow2( noises[nNoiseNum].noise.GetSizeX() ) ) &&
						 ( noises[nNoiseNum].noise.GetSizeY() == GetNextPow2( noises[nNoiseNum].noise.GetSizeY() ) ),
						 fmt::format("Noise {} has not powered two sizes", noises[nNoiseNum].szFileName ) );

	noises[nNoiseNum].bLoaded = true;
}


