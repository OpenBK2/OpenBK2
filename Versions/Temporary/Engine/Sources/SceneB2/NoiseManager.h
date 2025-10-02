#pragma once

#include "Misc/2DArray.h"

class CNoiseAccessor
{
	friend class CNoiseManager;

	const CArray2D<BYTE> &noise;
	const unsigned int nXOffset;
	const unsigned int nYOffset;

	CNoiseAccessor( const CArray2D<BYTE> &_noise ) 
		: noise(_noise), 
			nXOffset( (float)rand() / RAND_MAX * _noise.GetSizeX() ), 
			nYOffset( (float)rand() / RAND_MAX * _noise.GetSizeY() ) {}
public:
	CNoiseAccessor( const CNoiseAccessor &acc )
		: noise( acc.noise ), nXOffset( acc.nXOffset ), nYOffset( acc.nYOffset ) {}

	CNoiseAccessor& operator=( const CNoiseAccessor &acc )
	{
		CNoiseAccessor::~CNoiseAccessor();
		::new( this ) CNoiseAccessor( acc );
		return *this;
	}

	BYTE GetValue( unsigned int x, unsigned int y )
	{
		NI_ASSERT( ( noise.GetSizeX() > 0 ) && ( noise.GetSizeY() > 0 ), "Empty noise" );
		return noise[(y + nYOffset) & ( noise.GetSizeY() - 1 )][(x + nXOffset) & ( noise.GetSizeX() - 1 )];
	}
};

class CNoiseManager
{
	struct SNoiseStored
	{
		string szFileName;
		//
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

	struct SLoadedNoise
	{
		string szFileName;
		CArray2D<BYTE> noise;
		BYTE bLoaded;
	};

	vector<SLoadedNoise> noises;

	void LoadNoise( unsigned int nNoiseNum );

public:
	CNoiseManager();

	CNoiseAccessor GetNoise( unsigned int nNoiseNum );
	CNoiseAccessor GetNoise( const string &szName );
};


