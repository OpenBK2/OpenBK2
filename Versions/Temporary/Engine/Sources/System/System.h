#pragma once

#include "System_export.h"


template <int N> 
struct SInt2Type { enum { value = N }; };

// ************************************************************************************************************************ //
// **
// ** some types to be friend of :)
// **
// **
// **
// ************************************************************************************************************************ //

struct IBinSaver;

// ************************************************************************************************************************ //
// **
// ** objects factory
// **
// **
// **
// ************************************************************************************************************************ //

// функция для создания нового объекта
class CObjectBase;
typedef CObjectBase* (*ObjectFactoryNewFunc)(); 

namespace NObjectFactory
{
	// create new object by typeID
	SYSTEM_EXPORT CObjectBase *MakeObject( int nTypeID );
	// register type for further creation
	SYSTEM_EXPORT void RegisterType( int nObjectTypeID, ::ObjectFactoryNewFunc pfnNewFunc, const std::type_info *pTypeInfo );
	void UnRegisterType( int nObjectTypeID, const std::type_info *pTypeInfo );
	// get object's typeID (for save/load system)
	SYSTEM_EXPORT int GetObjectTypeID( CObjectBase *pObj );
	SYSTEM_EXPORT int GetObjectTypeID( const std::type_info &rtti );
	// check, is type registered
	SYSTEM_EXPORT bool IsRegistered( int nObjectTypeID );
	// start register type
	SYSTEM_EXPORT void StartRegister();
	// 
	template <class TT>
		void RegisterTypeName( int nTypeID, ObjectFactoryNewFunc func, TT* ) { NObjectFactory::RegisterType( nTypeID, func, &typeid(TT) ); }
	template <class TT>
		void UnRegisterTypeName( int nTypeID, TT* ) { NObjectFactory::UnRegisterType( nTypeID, &typeid(TT) ); }
};
// MACROses to register class
#define REGISTER_CLASS( N, name ) { NObjectFactory::StartRegister(); NObjectFactory::RegisterTypeName( N, name##::New##name, (name*)0 ); }
#define UNREGISTER_CLASS( N, name ) NObjectFactory::UnRegisterTypeName( N, (name*)0 );

#define REGISTER_TEMPL_CLASS( N, name, className ) { NObjectFactory::StartRegister(); NObjectFactory::RegisterTypeName( N, name##::New##className, (name*)0 ); }
#define UNREGISTER_TEMPL_CLASS( N, name ) NObjectFactory::UnRegisterTypeName( N, (name*)0 );

#define REGISTER_CLASS_NM( N, name, nmspace ) { NObjectFactory::StartRegister(); NObjectFactory::RegisterTypeName( N, nmspace::name##::New##name, (nmspace::name*)0 ); }
#define UNREGISTER_CLASS_NM( N, name, nmspace ) NObjectFactory::UnRegisterTypeName( N, (nmspace::name*)0 );
// make object by TypeID from objects factory
template <class TYPE> 
	inline TYPE* MakeObject( int nTypeID ) { return checked_cast<TYPE*>( NObjectFactory::MakeObject(nTypeID) ); }
template <class TYPE> 
	inline TYPE* MakeObjectVirtual( int nTypeID ) { return dynamic_cast<TYPE*>( NObjectFactory::MakeObject(nTypeID) ); }

// ************************************************************************************************************************ //
// **
// ** singleton
// **
// **
// **
// ************************************************************************************************************************ //

namespace NSingleton
{
	SYSTEM_EXPORT CObjectBase *Singleton( const int nTypeID );
	SYSTEM_EXPORT void RegisterSingleton( CObjectBase *pObj, const int nTypeID );
	SYSTEM_EXPORT void UnRegisterSingleton( const int nTypeID );
	SYSTEM_EXPORT void DoneSingletons();
	SYSTEM_EXPORT void Serialize( const char chunkID, IBinSaver &saver );
	SYSTEM_EXPORT void GetAllSingletonIDs( std::vector<int> *pRes );
};
// получить singleton по ID из глобального хранилища.
// singleton должен иметь enum с одним полем 'tidTypeID', которое содержит его константу
// и под этой константой он уже зарегистрирован в глобальном хранилище
template <class TYPE>
inline TYPE* Singleton() { return checked_cast<TYPE*>( NSingleton::Singleton(TYPE::tidTypeID) ); }

// serialization
namespace NSystem
{
	SYSTEM_EXPORT void Serialize( const char chunkID, IBinSaver &saver );
};


