#include "stdafx.h"

#include "LoadImage.h"
#include "NoiseManager.h"
#include "Misc/StrProc.h"
#include "System/VFSOperations.h"

struct SLoadNoise
{
	string szFileName;
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

	vector<SNoiseStored> vLoadNoises;

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
	NI_ASSERT( nNoiseNum < noises.size(), StrFmt("Invalid noise number %d - available [0..]", nNoiseNum, noises.size() - 1) );
	if ( !noises[nNoiseNum].bLoaded )
		LoadNoise( nNoiseNum );

	return CNoiseAccessor( noises[nNoiseNum].noise );
}

CNoiseAccessor CNoiseManager::GetNoise( const string &_szName )
{
	string szName;
	NStr::ToLowerASCII( &szName, _szName );
	for ( int i = 0; i < noises.size(); ++i )
	{
		if ( noises[i].szFileName == szName ) 
			return GetNoise( i );
	}
	NI_ASSERT( false, StrFmt("Unknown noise \"%s\"", szName.c_str()) );
	return GetNoise( 0 );
}

void CNoiseManager::LoadNoise( unsigned int nNoiseNum )
{
	CFileStream stream( NVFS::GetMainVFS(), noises[nNoiseNum].szFileName );
	NI_ASSERT( stream.IsOk(), StrFmt("Can't load noise: %s", noises[nNoiseNum].szFileName.c_str() ) );

	LoadGrayTGAImage( &stream, noises[nNoiseNum].noise );

	NI_ASSERT( ( noises[nNoiseNum].noise.GetSizeX() == GetNextPow2( noises[nNoiseNum].noise.GetSizeX() ) ) &&
						 ( noises[nNoiseNum].noise.GetSizeY() == GetNextPow2( noises[nNoiseNum].noise.GetSizeY() ) ),
						 StrFmt("Noise %s has not powered two sizes", noises[nNoiseNum].szFileName.c_str() ) );

	noises[nNoiseNum].bLoaded = true;
}


