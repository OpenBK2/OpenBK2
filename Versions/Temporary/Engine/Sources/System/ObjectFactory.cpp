#include "stdafx.h"
#include "ObjectFactory.h"

namespace NObjectFactory
{

// class factory
static CClassFactory<CObjectBase> *pSSClasses = 0;

struct SBasicChunkInit {
	~SBasicChunkInit() { if ( pSSClasses ) delete pSSClasses; }
} init;

void StartRegister()
{
	if ( !pSSClasses )
		pSSClasses = new CClassFactory<CObjectBase>;
}

// создать объект по его typeID
CObjectBase *MakeObject( int nTypeID )
{
//	NI_ASSERT( pSSClasses->IsRegistered( nTypeID ), StrFmt("Unregistered object of type 0x%x - no new-function", nTypeID) );
	return pSSClasses->CreateObject( nTypeID );
}

void RegisterType( int nObjectTypeID, ObjectFactoryNewFunc pfnNewFunc, const std::type_info *pTypeInfo )
{
	pSSClasses->RegisterTypeBase( nObjectTypeID, pfnNewFunc, pTypeInfo );
}

void UnRegisterType( int nObjectTypeID, const std::type_info *pObjectTypeInfo )
{
	if ( pSSClasses )
		pSSClasses->UnregisterTypeBase( nObjectTypeID, pObjectTypeInfo );
}

int GetObjectTypeID( CObjectBase *pObj )
{
	return pSSClasses->GetObjectTypeID( pObj );
}

int GetObjectTypeID( const std::type_info &rtti )
{
	return pSSClasses->GetObjectTypeID( &rtti );
}

bool IsRegistered( int nObjectTypeID )
{
	return pSSClasses ? pSSClasses->IsRegistered( nObjectTypeID ) : false;
}
}

