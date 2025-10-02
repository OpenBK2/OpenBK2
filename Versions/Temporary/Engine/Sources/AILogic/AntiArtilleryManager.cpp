#include "stdafx.h"

#include "AntiArtilleryManager.h"
#include "AntiArtillery.h"
#include "Diplomacy.h"

extern CDiplomacy theDipl;
CAntiArtilleryManager theAAManager;
extern NTimer::STime curTime;

void CAntiArtilleryManager::Init()
{
	antiArtilleries.resize( 3 );
}

void CAntiArtilleryManager::AddAA( CAntiArtillery *pAA )
{
	antiArtilleries[pAA->nParty].insert( pAA->GetUniqueId() );
}

void CAntiArtilleryManager::RemoveAA( CAntiArtillery *pAA )
{
	antiArtilleries[pAA->nParty].erase( pAA->GetUniqueId() );
}

// выстрел из pAntiArt был слышен строной nParty
bool CAntiArtilleryManager::IsHeardForParty( CAntiArtillery *pAntiArt, const int nParty )
{
	const NTimer::STime lastShotTime = pAntiArt->lastShotTime[nParty];
	return 
		lastShotTime != 0 && curTime - lastShotTime <= SConsts::AUDIBILITY_TIME;
}

void CAntiArtilleryManager::Segment()
{
	for ( int i = 0; i < 3; ++i )
	{
		if ( !antiArtilleries[i].empty() )
		{
			list<int> aa2Delete;
			for ( CAntiArtilleries::iterator iter = antiArtilleries[i].begin(); iter != antiArtilleries[i].end(); ++iter )
			{
				bool bDelete = true;
				CAntiArtillery *pAntiArt = GetObjectByUniqueIdSafe<CAntiArtillery>( *iter );
				if ( pAntiArt && pAntiArt->IsRefValid() )
				{
					int j = 0;
					while ( bDelete && j < 3 )
					{
						if ( theDipl.GetDiplStatusForParties( i, j ) == EDI_ENEMY && IsHeardForParty( pAntiArt, j ) )
							bDelete = false;
						else
							++j;
					}
				}

				if ( bDelete )
					aa2Delete.push_back( *iter );
			}

			for ( list<int>::iterator iter = aa2Delete.begin(); iter != aa2Delete.end(); ++iter )
				antiArtilleries[i].erase( *iter );
		}
	}
}

void CAntiArtilleryManager::Clear()
{
	antiArtilleries.clear();
}

//*******************************************************************
//*									CAntiArtilleryManager::SIterator								*
//*******************************************************************

CAntiArtilleryManager::CIterator::CIterator( const int nParty )
: nIterParty( nParty ), nCurParty( 0 )
{
	while ( nCurParty < 3 && ( theDipl.GetDiplStatusForParties( nCurParty, nIterParty ) != EDI_ENEMY || theAAManager.antiArtilleries[nCurParty].empty() ) )
		++nCurParty;

	if ( !IsFinished() )
	{
		curIter = theAAManager.antiArtilleries[nCurParty].begin();
		CAntiArtillery *pAntiArt = GetObjectByUniqueIdSafe<CAntiArtillery>( *curIter );
		if ( !pAntiArt || !pAntiArt->IsRefValid() || !IsHeardForParty( pAntiArt, nIterParty ) )
			Iterate();
	}
}

const CCircle CAntiArtilleryManager::CIterator::operator*() const
{
	CAntiArtillery *pAntiArt = GetObjectByUniqueIdSafe<CAntiArtillery>( *curIter );
	return pAntiArt->GetRevealCircle( nIterParty );
}

CAntiArtillery* CAntiArtilleryManager::CIterator::GetAntiArtillery() const
{
	return GetObjectByUniqueIdSafe<CAntiArtillery>( *curIter );
}

void CAntiArtilleryManager::CIterator::Iterate()
{
	++curIter;

	while ( nCurParty < 3 )
	{
		CAntiArtillery *pAntiArt = 0;
		if ( curIter != theAAManager.antiArtilleries[nCurParty].end() )
			pAntiArt = GetObjectByUniqueIdSafe<CAntiArtillery>( *curIter );

		if ( curIter == theAAManager.antiArtilleries[nCurParty].end() ||
				 !pAntiArt || !pAntiArt->IsRefValid() || !IsHeardForParty( pAntiArt, nIterParty ) )
		{
			if ( curIter != theAAManager.antiArtilleries[nCurParty].end() )
				++curIter;
			else
			{
				do
				{
					++nCurParty;
				} while ( nCurParty < 3 && ( theDipl.GetDiplStatusForParties( nCurParty, nIterParty ) != EDI_ENEMY || theAAManager.antiArtilleries[nCurParty].empty() ) );

				if ( nCurParty < 3 )
					curIter = theAAManager.antiArtilleries[nCurParty].begin();
			}
		}
		else
			break;
	}
}

bool CAntiArtilleryManager::CIterator::IsFinished() const
{
	return nCurParty >= 3;
}

