#pragma once

#include "System/System.h"

#include <stdexcept>
#include <string>
#include <typeinfo>

namespace NUITest
{
// Build a UI description record through the object factory, refusing to hand back
// a null.
//
// NObjectFactory::MakeObject returns 0 for a type ID the factory does not know,
// and MakeObject<T> is only a checked_cast over that, which is a static_cast in
// this configuration, so the null came back typed. Every call site here writes a
// member immediately afterwards, so the null became a write through a null
// pointer: GCC's -Wnull-dereference reported three of those writes directly and
// another three at System/Basic.h:411,412, where the assignment into a CDBPtr
// member inlines down to CPtrBase::SetObject.
//
// A type ID the factory does not know means the test binary was linked without
// the module that registers it, which is a build failure rather than something a
// test can continue past. Throwing turns the segfault that used to follow into a
// named GoogleTest failure, since gtest catches exceptions and reports the
// message. Every call site passed T::typeID for its own T, so the type ID is
// derived here rather than repeated.
template <class T>
CPtr<T> MakeDescription()
{
	T *const pObject = MakeObject<T>( T::typeID );
	if ( pObject == 0 )
	{
		throw std::runtime_error( std::string( "the object factory does not know " ) + typeid( T ).name() +
		                          ", so the test binary is missing the module that registers it" );
	}
	return pObject;
}
} // namespace NUITest
