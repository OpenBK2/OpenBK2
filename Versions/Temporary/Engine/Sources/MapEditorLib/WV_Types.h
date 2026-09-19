
#pragma once
#include "System/RandomGen.h"
#include <fmt/format.h>
#include "Misc/Win32Random.h"

//Темплейт для создания векторов обьектов с весами,
//веса могут быть любыми неотрицательными целыми числами

namespace NWV
{
	// [0...n-1]
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SAIRandom
	{
		static int GetRandom( int nMax ) //[0...n-1]
		{
			RecordRandomCall(); return NRandom::Random( nMax ); //[0...n-1]
		}
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SClientRandom
	{
		static int GetRandom( int nMax ) //[0...n-1]
		{
			return NWin32Random::Random( nMax ); //[0...n-1]
		}
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <class TYPE, class TRandom>
	class CWeightVector
	{
		std::vector<TYPE> elements; //элементы (должны уметь упаковываться в контейнеры)
		std::vector<int> weights;		//веса ( разница между соседями - вес текущего элемента )

	public:
		//
		//конструкторы и операторы присваивания
		CWeightVector() {}
		//
		CWeightVector( const CWeightVector &rWeightVector ) : elements( rWeightVector.elements ), weights( rWeightVector.weights ) {}
		//
		CWeightVector& operator=( const CWeightVector &rWeightVector )
		{
			if( &rWeightVector != this )
			{
				elements =  rWeightVector.elements;
				weights = rWeightVector.weights;
			}
			return *this;
		}
		
		//
		// Indexed access (operator[], Get, Set), per-element weights (GetWeight,
		// SetWeight) and erase are gone: nothing called them, and they asserted
		// through NI_ASSERT_T and NStr::Format, which do not exist, so they had
		// never been instantiated.

		//
		//методы аналогичные std::vector методам
		void push_back( const TYPE &rElement, int nWeight )
		{
			elements.push_back( rElement );
			if ( !weights.empty() )
			{
				weights.push_back( weights[weights.size() - 1] + nWeight );
			}
			else
			{
				weights.push_back( nWeight );
			}
		}
		//
		inline int size() const { return elements.size(); }
		inline int weight() const { return !weights.empty() ? weights[weights.size() - 1 ] : 0; }
		//
		inline void clear() { elements.clear(); weights.clear(); }
		//
		inline bool empty() const { return elements.empty(); }

		//
		//получить рандомный элемент
		int GetRandomIndex( bool bBinarySearch = true ) const
		{
			if ( weights.empty() || weights[ weights.size() - 1 ] == 0 )
			{
				return ( -1 );
			}
			
			int nWeight = TRandom::GetRandom( weights[ weights.size() - 1 ] );
			int nMinIndex = 0;
			int nMaxIndex = weights.size() - 1;

			if ( bBinarySearch )
			{
				//бинарный поиск:
				while ( ( nMaxIndex - nMinIndex ) > 1 )
				{
					int nElementIndex = ( nMinIndex + nMaxIndex ) / 2;
					if ( weights[nElementIndex] > nWeight )
					{
						nMaxIndex = nElementIndex;
					}
					else
					{
						nMinIndex = nElementIndex;
					}
				}
			}
			//простой поиск
			while ( nWeight >= weights[nMinIndex] ) ++nMinIndex;
			return nMinIndex;
		}

		//
		const TYPE& GetRandom( bool bBinarySearch = true ) const
		{
			int nElementIndex = GetRandomIndex( bBinarySearch );
			NI_ASSERT( ( nElementIndex >=0 ) && ( nElementIndex < elements.size() ),
								fmt::format( "Index ({}) miss in SWeightVector ({})", nElementIndex, elements.size() ) );
			return elements[nElementIndex];
		}
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};


