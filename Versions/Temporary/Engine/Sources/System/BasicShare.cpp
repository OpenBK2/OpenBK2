#include "StdAfx.h"
#include "BasicShare.h"

// CBasicShareBase

static CBasicShareBase *pList;
void RegisterBasicShareBase( CBasicShareBase *pBase )
{
	for ( CBasicShareBase *pTest = pList; pTest; pTest = pTest->pNext )
		ASSERT( pTest->nID != pBase->nID );
	pBase->pNext = pList;
	pList = pBase;
}

// CSharedHolder

void CreateSharedHolder( CSharedHolder *pHolder )
{
	for ( CBasicShareBase *pTest = pList; pTest; pTest = pTest->pNext )
		pTest->CreateHolder( &pHolder->objs );
}


void SerializeShared( IBinSaver *pFile )
{
	for ( CBasicShareBase *pTest = pList; pTest; pTest = pTest->pNext )
		(*pTest) & ( *pFile );
}

