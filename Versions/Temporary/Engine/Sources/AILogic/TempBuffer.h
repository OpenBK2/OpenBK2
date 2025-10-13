#pragma once

#include <cstdint>

// ************************************************************************************************************************ //
// **
// ** temporal buffer - special storage for temporal (fire'n'forgot) data
// **
// **
// **
// ************************************************************************************************************************ //

extern std::vector<uint8_t> buffer;

template< class TYPE> 
TYPE* GetLocalTempBuffer( int nAmount ) 
{ 
	buffer.reserve( nAmount*sizeof(TYPE) );
	return reinterpret_cast<TYPE*>( &buffer[0] ); 
}
