#pragma once

/*
typedef hash_map<string, ObjectFactoryNewFunc> CNewFuncsMap;
static CNewFuncsMap *pNewFuncs = 0;
static struct SNewFuncsMapAutoMagic
{
	~SNewFuncsMapAutoMagic()
	{
		if ( pNewFuncs )
			delete pNewFuncs;
		pNewFuncs = 0;
	}
} aNewFuncsMapAutoMagic;

void StartRegister()
{
	if ( pNewFuncs == 0 )
		pNewFuncs = new CNewFuncsMap();
}
void RegisterType( const string &szName, ObjectFactoryNewFunc pfnNewFunc )
{
	(*pNewFuncs)[szName] = pfnNewFunc;
}
void UnRegisterType( const string &szName )
{
	if ( pNewFuncs != 0 )
		pNewFuncs->erase( szName );
}

CObjectBase* CreateInternal( const string &szName )
{
	ObjectFactoryNewFunc pfnNewFunc = (*pNewFuncs)[szName];
	return pfnNewFunc != 0 ? (*pfnNewFunc)() : 0;
}

template <class TYPE>
TYPE* Create( const string &szName )
{
	return checked_cast<TYPE*>( CreateInternal(szName) );
}

bool CanCreate( const string &szName )
{
	return pNewFuncs != 0 ? ( pNewFuncs->find( szName ) != pNewFuncs->end() ) : false;
}

*/



#define DEFINE_FACTORY( TYPE )																											 \
namespace N##TYPE##Factory																													 \
{																																										 \
typedef hash_map<string, ObjectFactoryNewFunc> C##TYPE##NewFuncsMap;								 \
static C##TYPE##NewFuncsMap *pNewFuncs = 0;																					 \
static struct S##TYPE##NewFuncsMapAutoMagic																					 \
{																																										 \
	~S##TYPE##NewFuncsMapAutoMagic()																									 \
	{																																									 \
		if ( pNewFuncs )																																 \
			delete pNewFuncs;																															 \
		pNewFuncs = 0;																																	 \
	}																																									 \
} a##TYPE##NewFuncsMapAutoMagic;																										 \
void StartRegister##TYPE()																													 \
{																																										 \
	if ( pNewFuncs == 0 )																															 \
		pNewFuncs = new C##TYPE##NewFuncsMap();																					 \
}																																										 \
void Register##TYPE##Type( const string &szName, ObjectFactoryNewFunc pfnNewFunc )	 \
{																																										 \
	(*pNewFuncs)[szName] = pfnNewFunc;																								 \
}																																										 \
void UnRegister##TYPE##Type( const string &szName )																	 \
{																																										 \
	if ( pNewFuncs != 0 )																															 \
		pNewFuncs->erase( szName );																											 \
}																																										 \
I##TYPE* Create##TYPE( const string &szName )																				 \
{																																										 \
	ObjectFactoryNewFunc pfnNewFunc = (*pNewFuncs)[szName];														 \
	return pfnNewFunc != 0 ? checked_cast<I##TYPE##*>( (*pfnNewFunc)() ) : 0;					 \
}																																										 \
bool CanCreate##TYPE( const string &szName )																				 \
{																																										 \
	return pNewFuncs != 0 ? ( pNewFuncs->find( szName ) != pNewFuncs->end() ) : false; \
}																																										 \
};


