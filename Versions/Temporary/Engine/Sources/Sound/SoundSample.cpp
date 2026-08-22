#include "stdafx.h"

#include "SoundSample.h"
#include "DBSoundDesc.h"
#include "System/VFSOperations.h"

#include "Sound_export.h"

bool CSoundSample::b3DSoundShare = false;

// ************************************************************************************************************************ //
// **
// ** base shared sound sample resource
// **
// **
// **
// ************************************************************************************************************************ //

CSoundSample::CSoundSample() 
: sample( 0 )
{
}

CSoundSample::~CSoundSample() 
{ 
	Close(); 
}

void CSoundSample::Close() 
{ 
	if ( sample ) 
		FSOUND_Sample_Free( sample ); 
	sample = 0; 
}

void CSoundSample::SetSample( FSOUND_SAMPLE *_sample ) 
{ 
	Close(); 
	sample = _sample; 
	if ( sample )
		FSOUND_Sample_SetMinMaxDistance( sample, 45, 1000000000.0f );
}

FSOUND_SAMPLE* CSoundSample::GetInternalContainer() 
{ 
	return sample; 
}

void CSoundSample::SetLoop( bool bEnable ) 
{ 
	if ( sample )
		FSOUND_Sample_SetMode( sample, bEnable ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF ); 
}

void CSoundSample::SetKey( const CDBID &dbid )
{
	dbidSound = dbid;
	// load info
	const NDb::SSoundDesc *pDesc = NDb::Get<NDb::SSoundDesc>( dbidSound );
	NI_ASSERT( pDesc != 0, fmt::format( "wrong sound Desc DBID \"{}\"", dbidSound.ToString() ) );
	if ( pDesc )
	{
		CFileStream stream( NVFS::GetMainVFS(), pDesc->szSoundPath );
		if ( stream.IsOk() )
		{
			const int nSize = stream.GetSize();
			std::vector<char> data(nSize);
			stream.Read( &data[0], nSize );
			FSOUND_SAMPLE *pSample = FSOUND_Sample_Load( FSOUND_UNMANAGED, &data[0], /*( b3DSoundShare ? FSOUND_HW3D : FSOUND_2D ) |*/ FSOUND_LOADMEMORY, 0, nSize );
			SetSample( pSample );
		}
	}
}

int CSoundSample::operator&( IBinSaver &saver )
{
	saver.Add( 5, &dbidSound );
	if ( saver.IsReading() )
	{
		sample = 0;
		SetKey( dbidSound );
	}
	return 0;
}

REGISTER_SAVELOAD_CLASS( SOUND, 0x110B2C00, CSoundSample );

