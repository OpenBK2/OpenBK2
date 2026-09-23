#pragma once

#include <cstdint>
#include <vector>

namespace NDb
{
namespace NMetaInfo
{
// Keep the actual vector type at the reflection boundary. Moving its bytes
// bypasses string/reference constructors and corrupts short strings on Linux.
struct SArrayOperations
{
	int (*size)( const void * );
	uint8_t *(*element)( void *, int );
	void (*insert)( void *, int, int );
	void (*remove)( void *, int, int );
};

template <class T>
const SArrayOperations *ArrayOperations()
{
	static const SArrayOperations operations = {
		[]( const void *p ) { return int( static_cast<const std::vector<T>*>( p )->size() ); },
		[]( void *p, int index ) { return reinterpret_cast<uint8_t*>( &(*static_cast<std::vector<T>*>( p ))[index] ); },
		[]( void *p, int index, int count ) {
			auto &values = *static_cast<std::vector<T>*>( p );
			values.insert( values.begin() + index, count, T{} );
		},
		[]( void *p, int index, int count ) {
			auto &values = *static_cast<std::vector<T>*>( p );
			values.erase( values.begin() + index, values.begin() + index + count );
		}
	};
	return &operations;
}
}
}
