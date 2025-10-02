#include "StdAfx.h"

#include "RandomGen.h"

namespace NSystem
{

struct SSerialize
{
	CPtr<IRandomSeed> pRandomSeed;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pRandomSeed );
		return 0;
	}
};
void Serialize( const char chunkID, IBinSaver &saver )
{
	// prepare holder
	static SSerialize holder;
	holder.pRandomSeed = NRandom::CreateRandomSeedCopy();
	// serialize
	saver.Add( chunkID, &holder );
	// restore some things
	if ( saver.IsReading() ) 
	{
		NRandom::SetRandomSeed( holder.pRandomSeed );
	}
}

};

