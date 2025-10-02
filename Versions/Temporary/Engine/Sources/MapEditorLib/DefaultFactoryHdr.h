#pragma once

#define DECLARE_FACTORY( TYPE )																													\
interface I##TYPE;																																			\
namespace N##TYPE##Factory																															\
{																																												\
	void Register##TYPE##Type( const string &szName, ObjectFactoryNewFunc pfnNewFunc );		\
	void UnRegister##TYPE##Type( const string &szName );																	\
	I##TYPE* Create##TYPE( const string &szName );																				\
	bool CanCreate##TYPE( const string &szName );																					\
	void StartRegister##TYPE();																														\
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

void RegisterMapEditorTypeDelayed( const string &szType, const string &szName, ObjectFactoryNewFunc pfnNewFunc );

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


