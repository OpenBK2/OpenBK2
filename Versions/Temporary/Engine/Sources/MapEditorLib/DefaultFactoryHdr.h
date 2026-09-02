#pragma once
#include "MapEditorLib_export.h"

#define DECLARE_FACTORY( TYPE )																													\
struct I##TYPE;																																			\
namespace N##TYPE##Factory																															\
{																																												\
	MAPEDITORLIB_EXPORT void Register##TYPE##Type( const std::string &szName, ObjectFactoryNewFunc pfnNewFunc );		\
	MAPEDITORLIB_EXPORT void UnRegister##TYPE##Type( const std::string &szName );																	\
	MAPEDITORLIB_EXPORT I##TYPE* Create##TYPE( const std::string &szName );																				\
	MAPEDITORLIB_EXPORT bool CanCreate##TYPE( const std::string &szName );																					\
	MAPEDITORLIB_EXPORT void StartRegister##TYPE();																														\
};

#define REGISTER_ME_OBJECT_IN_EXE( TYPE, name, classname )															\
	void StartRegisterHookRegisterMEObject##TYPE##name##classname() {}                    \
static struct SRegister##TYPE##name##classname##AutoMagic																\
{																																												\
	SRegister##TYPE##name##classname##AutoMagic()																					\
	{																																											\
		N##TYPE##Factory::StartRegister##TYPE();																						\
		N##TYPE##Factory::Register##TYPE##Type( #name, classname::New##classname );					\
	}																																											\
	~SRegister##TYPE##name##classname##AutoMagic()																				\
	{																																											\
		N##TYPE##Factory::UnRegister##TYPE##Type( #name );																	\
	}																																											\
} aRegister##TYPE##name##classname##AutoMagic;

void RegisterMapEditorTypeDelayed( const std::string &szType, const std::string &szName, ObjectFactoryNewFunc pfnNewFunc );

#define REGISTER_ME_OBJECT_IN_DLL( TYPE, name, classname ) REGISTER_ME_OBJECT_IN_EXE( TYPE, name, classname )
	/*
#define REGISTER_ME_OBJECT_IN_DLL( TYPE, name, classname )										\
static struct SRegister##TYPE##name##classname##AutoMagicDelayed							\
{																																							\
	SRegister##TYPE##name##classname##AutoMagicDelayed()												\
	{																																						\
		RegisterMapEditorTypeDelayed( #TYPE, #name, classname::New##classname );	\
	}																																						\
} aRegister##TYPE##name##classname##AutoMagicDelayed;
*/


