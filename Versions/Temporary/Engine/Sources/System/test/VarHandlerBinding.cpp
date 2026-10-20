// Pins which variable types each console-variable handler will accept.
//
// NGlobal::RegisterVar erases the variable to a void * and the handler stores
// through it, so a handler pointed at the wrong type leaves the variable
// holding the other type's bit pattern. Eight registrations in this tree had
// exactly that, three of them live on every start, and none of it was visible
// at the call site. TVarBindingAccepts is what makes it a compile error now.
//
// These are static_asserts rather than runtime checks because the thing under
// test is a decision the compiler makes. The test exists because a predicate
// that answers true to everything would pass every build silently: the cases
// that must be REFUSED are the point, and they are the half that a widened
// trait would quietly lose.

// BinSaver.h first: GlobalVars.h declares CValue with the ZDATA and ZEND
// markers and an IBinSaver operator, which the module prelude supplies in a
// normal translation unit and a test has to ask for.
#include "System/BinSaver.h"
#include "System/GlobalVars.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

template <class TBound, class TVar>
constexpr bool Accepts()
{
	return NGlobal::TVarBindingAccepts<TBound, TVar>::value;
}

// A handler may write the type it is for.
static_assert( Accepts<bool, bool>(), "the bool handler must accept a bool" );
static_assert( Accepts<int, int>(), "the int handler must accept an int" );
static_assert( Accepts<float, float>(), "the float handler must accept a float" );
static_assert( Accepts<std::string, std::string>(), "the string handler must accept a string" );
static_assert( Accepts<std::wstring, std::wstring>(), "the wide handler must accept a wstring" );

// And the signed or unsigned variant of that type, because accessing an object
// through that variant is allowed. Three registrations depend on this, two of
// them NTimer::STime, which is uint32_t.
static_assert( Accepts<int, unsigned int>(), "the int handler must accept the unsigned variant" );
static_assert( Accepts<int, std::uint32_t>(), "uint32_t is that variant on every target here" );

// The refusals. Each line is a bug that reached the shipped tree.
static_assert( !Accepts<int, float>(), "AI.Common.GuardStateRadius was this: an int stored into a float" );
static_assert( !Accepts<float, int>(), "AI.Weather.TimeToFadeOff was this: a float stored into an int" );
static_assert( !Accepts<int, bool>(), "ft_zoom was this: four bytes stored into a one-byte object" );
static_assert( !Accepts<bool, int>(), "and the same the other way round" );

// Same size is not the same type. DWORD is unsigned long on MSVC, which is a
// distinct type from int whatever its width, and gfx_fsaa was that case: legal
// here only because the DXVK headers make DWORD a uint32_t.
static_assert( sizeof( long ) == sizeof( int ) || sizeof( long ) != sizeof( int ),
               "this file makes no assumption about the width of long" );
static_assert( !Accepts<int, unsigned long>(), "unsigned long is not the unsigned variant of int" );
static_assert( !Accepts<int, long>(), "nor is long a variant of int" );

// Unrelated types, which would be the next mistake of this shape.
static_assert( !Accepts<std::string, std::wstring>(), "the narrow and wide handlers are not interchangeable" );
static_assert( !Accepts<float, std::string>(), "a float handler cannot write a string" );
static_assert( !Accepts<bool, float>(), "a bool handler cannot write a float" );

// The tags really do carry the type their function writes. If one of these is
// ever respelled, every registration naming it starts checking the wrong type
// and nothing else in the suite would notice.
static_assert( std::is_same<decltype( NGlobal::VarBoolHandler )::TBound, bool>::value, "VarBoolHandler writes a bool" );
static_assert( std::is_same<decltype( NGlobal::VarIntHandler )::TBound, int>::value, "VarIntHandler writes an int" );
static_assert( std::is_same<decltype( NGlobal::VarFloatHandler )::TBound, float>::value,
               "VarFloatHandler writes a float" );
static_assert( std::is_same<decltype( NGlobal::VarStrHandler )::TBound, std::string>::value,
               "VarStrHandler writes a string" );
static_assert( std::is_same<decltype( NGlobal::VarWStrHandler )::TBound, std::wstring>::value,
               "VarWStrHandler writes a wstring" );

// The other half of the same idea: REGISTER_VAR supplies no context, so it
// takes a handler that has no context parameter. These two function types must
// not convert, or a handler expecting a void * could be registered with
// nothing to give it and would write through a null pointer on the first
// setvar. The old macro passed a hard-coded 0 and accepted exactly that.
static_assert( !std::is_convertible<NGlobal::VarHandler, NGlobal::VarHandlerNoContext>::value,
               "a handler that takes a context must not be registrable without one" );
static_assert( !std::is_convertible<NGlobal::VarHandlerNoContext, NGlobal::VarHandler>::value,
               "and a context-free handler must not be called with one" );

// A null is still allowed there, and means the variable only stores its value
// for GetVar to read. Eight registrations do that.
static_assert( std::is_constructible<NGlobal::VarHandlerNoContext, std::nullptr_t>::value,
               "a value-only variable registers with no handler at all" );

// The tag maps to the function that writes that type, which is the other half
// of the binding and is not covered by the TBound checks above.
TEST( VarHandlerBinding, EachTagResolvesToItsOwnFunction )
{
	EXPECT_EQ( NGlobal::GetVarHandlerFn( NGlobal::VarBoolHandler ), &NGlobal::VarBoolHandlerFn );
	EXPECT_EQ( NGlobal::GetVarHandlerFn( NGlobal::VarIntHandler ), &NGlobal::VarIntHandlerFn );
	EXPECT_EQ( NGlobal::GetVarHandlerFn( NGlobal::VarFloatHandler ), &NGlobal::VarFloatHandlerFn );
	EXPECT_EQ( NGlobal::GetVarHandlerFn( NGlobal::VarStrHandler ), &NGlobal::VarStrHandlerFn );
	EXPECT_EQ( NGlobal::GetVarHandlerFn( NGlobal::VarWStrHandler ), &NGlobal::VarWStrHandlerFn );
}

// And that each function writes what its tag claims, so the pairing above is
// checked against behaviour rather than against itself.
TEST( VarHandlerBinding, EachFunctionWritesItsOwnType )
{
	bool bFlag = false;
	NGlobal::VarBoolHandlerFn( "t", NGlobal::CValue( 1.0f ), &bFlag );
	EXPECT_TRUE( bFlag );

	int nValue = 0;
	NGlobal::VarIntHandlerFn( "t", NGlobal::CValue( 640.0f ), &nValue );
	EXPECT_EQ( nValue, 640 );

	// The one that matters: as a float this reads 640, not the denormal the
	// int handler used to leave in SConsts::GUARD_STATE_RADIUS.
	float fValue = 0.0f;
	NGlobal::VarFloatHandlerFn( "t", NGlobal::CValue( 640.0f ), &fValue );
	EXPECT_FLOAT_EQ( fValue, 640.0f );
}

} // namespace
