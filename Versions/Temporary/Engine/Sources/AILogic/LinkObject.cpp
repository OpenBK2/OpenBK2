#include "stdafx.h"

#include "LinkObject.h"
#include "Stats_B2_M1/AIUnitCmd.h"

#include <fmt/format.h>

SLinkObjData * SLinkObjDataAutoMagic::pLinkObjData = 0;
SLinkObjDataAutoMagic magic;


extern NTimer::STime curTime;

CObjectBase * GetObjectByCmd( const SAIUnitCmd &cmd )
{
	if ( cmd.nObjectID )
		return CLinkObject::GetObjectByUniqueIdSafe( cmd.nObjectID );
	else
		return 0;
}

CLinkObject::CLinkObject()
: nLink( -1 ), nUniqueID( -1 )
{
}

CLinkObject::~CLinkObject()
{
	if ( GetLink() > 0 && SLinkObjDataAutoMagic::pLinkObjData && !SLinkObjDataAutoMagic::pLinkObjData->link2object.empty() )
	{
		NI_ASSERT( SLinkObjDataAutoMagic::pLinkObjData->link2object.size() > GetLink(), "Wrong size" );
		SLinkObjDataAutoMagic::pLinkObjData->deletedObjects.push_back( GetLink() );

		NI_ASSERT( nUniqueID != 0, "wrong unique id" );
		SLinkObjDataAutoMagic::pLinkObjData->deletedUniqueObjects.push_back( nUniqueID );
	}
}

void CLinkObject::SetUniqueIdForObjects()
{
	nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
}

void CLinkObject::SetUniqueIdForUnits( const int _nUniqueID )
{
	nUniqueID = _nUniqueID;
}

void CLinkObject::SetLink( const int _nLink )
{
	nLink = _nLink;
	// CRAP{ чтобы грузились старые карты
	if ( _nLink > 0 )
	// }CRAP
	{
//		NI_ASSERT( SLinkObjDataAutoMagic::pLinkObjData->link2object.size() <= _nLink || SLinkObjDataAutoMagic::pLinkObjData->link2object[_nLink] == 0, StrFmt( "Repeated link %d", _nLink ) );

		if ( SLinkObjDataAutoMagic::pLinkObjData->link2object.size() <= _nLink )
			SLinkObjDataAutoMagic::pLinkObjData->link2object.resize( ( nLink + 1 ) * 1.5 );

		SLinkObjDataAutoMagic::pLinkObjData->link2object[nLink] = this;
	}
}

void CLinkObject::Mem2UniqueIdObjs()
{ 
	NI_ASSERT( nUniqueID > 0, "Unique id isn't set" );
	SLinkObjDataAutoMagic::pLinkObjData->unitsID2object[nUniqueID] = this;
}

CLinkObject* CLinkObject::GetObjectByLink( const int nLink )
{
	if ( nLink >= SLinkObjDataAutoMagic::pLinkObjData->link2object.size() || nLink <= 0 )
		return 0;
	else
		return SLinkObjDataAutoMagic::pLinkObjData->link2object[nLink];
}

void CLinkObject::Segment()
{
	for ( std::list<int>::iterator iter = SLinkObjDataAutoMagic::pLinkObjData->deletedObjects.begin(); iter != SLinkObjDataAutoMagic::pLinkObjData->deletedObjects.end(); ++iter )
		SLinkObjDataAutoMagic::pLinkObjData->link2object[*iter] = 0;
	for ( std::list<int>::iterator iter = SLinkObjDataAutoMagic::pLinkObjData->deletedUniqueObjects.begin(); iter != SLinkObjDataAutoMagic::pLinkObjData->deletedUniqueObjects.end(); ++iter )
	{
		int nID = *iter;
		if ( SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.find( nID ) != SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.end() )
			SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.erase( nID );
	}
	SLinkObjDataAutoMagic::pLinkObjData->deletedObjects.clear();
}

void CLinkObject::Clear()
{
	for ( int i = 0; i < SLinkObjDataAutoMagic::pLinkObjData->link2object.size(); ++i )
	{
		if ( SLinkObjDataAutoMagic::pLinkObjData->link2object[i] != 0 && SLinkObjDataAutoMagic::pLinkObjData->link2object[i]->IsRefValid() )
			checked_cast<CLinkObject*>( SLinkObjDataAutoMagic::pLinkObjData->link2object[i].GetPtr() )->SetLink( -1 );
	}
	
	SLinkObjDataAutoMagic::pLinkObjData->link2object.clear();
	SLinkObjDataAutoMagic::pLinkObjData->deletedObjects.clear();
	SLinkObjDataAutoMagic::pLinkObjData->deletedUniqueObjects.clear();
	SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.clear();

	SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID = 0;
}

void CLinkObject::ClearLinks()
{
	for ( int i = 0; i < SLinkObjDataAutoMagic::pLinkObjData->link2object.size(); ++i )
	{
		if ( SLinkObjDataAutoMagic::pLinkObjData->link2object[i] != 0 && SLinkObjDataAutoMagic::pLinkObjData->link2object[i]->IsRefValid() )
			checked_cast<CLinkObject*>( SLinkObjDataAutoMagic::pLinkObjData->link2object[i].GetPtr() )->SetLink( -1 );
	}
	
	SLinkObjDataAutoMagic::pLinkObjData->link2object.clear();
}

CLinkObject* CLinkObject::GetObjectByUniqueId( const int nUniqueID )
{ 
	NI_ASSERT( nUniqueID > 0, "Wrong object" );
	NI_ASSERT( SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.find( nUniqueID ) != SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.end(), fmt::format( "Wrong unique id ({})", nUniqueID ) );
	det_map<int, CObj<CLinkObject> >::iterator pos = SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.find( nUniqueID );
	return pos == SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.end() ? 0 : pos->second;
}

void CLinkObject::GetFreeLinks( std::list<int> *pLinks, const int nSize )
{
	pLinks->clear();
	for ( int i = 1; i < SLinkObjDataAutoMagic::pLinkObjData->link2object.size() && pLinks->size() < nSize; ++i )
	{
		if ( GetObjectByLink( i ) == 0 )
			pLinks->push_back( i );
	}

	if ( pLinks->size() < nSize )
	{
		int nLink = (std::max)( (int)SLinkObjDataAutoMagic::pLinkObjData->link2object.size(), 1 );
		while ( pLinks->size() < nSize )
		{
			pLinks->push_back( nLink );
			++nLink;
		}
	}
}

CLinkObject* CLinkObject::GetObjectByUniqueIdSafe( const int nUniqueID )
{
	NI_ASSERT( nUniqueID > 0, "Wrong object" );
	if ( SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.find( nUniqueID ) == SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.end() )
		return 0;
	else
		return SLinkObjDataAutoMagic::pLinkObjData->unitsID2object[nUniqueID];
}

bool CLinkObject::IsLinkObjectExists( const int nUniqueID )
{
	return SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.find( nUniqueID ) != SLinkObjDataAutoMagic::pLinkObjData->unitsID2object.end();
}


