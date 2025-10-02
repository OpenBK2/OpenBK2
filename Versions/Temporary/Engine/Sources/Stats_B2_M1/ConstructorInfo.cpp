#include "stdafx.h"

#include "DBConstructorProfile.h"
#include "ConstructorInfo.h"
#include "../Misc/nalgoritm.h"

bool CConstructorInfo::GetUnitPlatforms( const int nUniqueID, const vector<SUnitPlatform> **pPlatforms )
{
	hash_map<int, vector<SUnitPlatform> >::iterator posUnits = units.find( nUniqueID );
	if ( posUnits == units.end() )
		return false;
	*pPlatforms = &(posUnits->second);

	return true;
}

template<class T>
static void ConstructPlatforms( vector<CConstructorInfo::SUnitPlatform> *pPlatforms, const T *pProfile )
{
	pPlatforms->reserve( pProfile->platforms.size() );
	for ( int i = 0; i < pProfile->platforms.size(); ++i )
	{
		if ( pProfile->platforms[i].bAttached )
		{
			CConstructorInfo::SUnitPlatform &platform = 
				pPlatforms->size() <= i ? *(pPlatforms->insert( pPlatforms->end() )) :
				(*pPlatforms)[i].nPlatformIndex > i ? *(pPlatforms->insert( pPlatforms->begin() + i, CConstructorInfo::SUnitPlatform() )) : (*pPlatforms)[i];

			NI_VERIFY( platform.nPlatformIndex == -1 || platform.nPlatformIndex == i, "Wrong platform index", return );
		
			platform.nPlatformIndex = i;
			platform.gunIndexes.reserve( pProfile->platforms[i].guns.size() );
			for ( int j = 0; j < pProfile->platforms[i].guns.size(); ++j )
			{
				if ( pProfile->platforms[i].guns[j].bAttached )
				{
					if ( platform.gunIndexes.size() <= j )
						platform.gunIndexes.insert( platform.gunIndexes.end(), j );
					else if ( platform.gunIndexes[j] > j )
						platform.gunIndexes.insert( platform.gunIndexes.begin() + j, j );

					NI_VERIFY( platform.gunIndexes.size() > j || platform.gunIndexes[platform.gunIndexes.size()-1] == j, "Wrong gun index" ,return );
					NI_VERIFY( platform.gunIndexes.size() <= j || platform.gunIndexes[j] == j, "Wrong gun index" ,return );
				}
			}
		}
	}
}

void CConstructorInfo::ClearProfile( const int nUniqueID )
{
	units[nUniqueID].clear();
	slots[nUniqueID].clear();
}

void CConstructorInfo::ApplyProfile( const int nUniqueID, const NDb::SDBConstructorProfile *pProfile )
{
	NI_VERIFY( nUniqueID != -1, "id -1 is reserved for internal use", return );

	vector<SUnitPlatform> &platforms = units[nUniqueID];

	ConstructPlatforms( &platforms, pProfile );
	if ( !pProfile->slots.empty() )
	{
		vector<int> profileSlots( pProfile->slots );
		sort( profileSlots.begin(), profileSlots.end() );
		vector<int> oldSlots( slots[nUniqueID] );

		slots[nUniqueID].clear();
		slots[nUniqueID].resize( profileSlots.size() + oldSlots.size(), -1 );

		vector<int>::iterator iter = merge( oldSlots.begin(), oldSlots.end(), profileSlots.begin(), profileSlots.end(), slots[nUniqueID].begin() );
		slots[nUniqueID].erase( iter, slots[nUniqueID].end() );
	}
}

void CConstructorInfo::SetPlayerUnit( const int nUniqueID, CObjectBase *pPlayerUnit )
{
	playerUnits[nUniqueID] = pPlayerUnit;
}

CObjectBase* CConstructorInfo::GetPlayerUnit( const int nUniqueID )
{
	return playerUnits[nUniqueID];
}

int CConstructorInfo::GetSlotsSize( const int nUniqueID ) const
{
	hash_map<int, vector<int> >::const_iterator pos = slots.find( nUniqueID );
	if ( pos == slots.end() )
		return 0;
	else
		return pos->second.size();
}

int CConstructorInfo::GetSlotObject( const int nUniqueID, const int nSlot ) const
{
	hash_map<int, vector<int> >::const_iterator pos = slots.find( nUniqueID );
	if ( pos == slots.end() )
		return -1;
	else if ( nSlot < 0 || nSlot >= pos->second.size() )
		return -1;
	else
		return pos->second[nSlot];
}

CConstructorInfo* CreateConstructorInfo()
{
	return new CConstructorInfo();
}

REGISTER_SAVELOAD_CLASS( 0x3013EC01, CConstructorInfo );

