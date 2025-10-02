#ifndef __GENERAL_HELPER__
#define __GENERAL_HELPER__

#include "GeneralInternalInterfaces.h"
#include "../System/RandomGen.h"

namespace NDb
{
	struct SAIGeneralParcel;
}

struct SGeneralHelper
{
	static bool IsUnitNearParcel( const class CCommonUnit *pUnit, const struct NDb::SAIGeneralParcel &parcel );
	static bool IsUnitInParcel( const class CCommonUnit *pUnit, const struct NDb::SAIGeneralParcel &parcel );
	static float CalcUnitSeverity( const class CCommonUnit *pUnit );
	//return true if any removed 
	static bool RemoveDead( CommonUnits *pUnits );
	static bool RemoveNotCurrentPlayer( CommonUnits *pUnits, int nPlayer );

	//
	struct SRandomFunctor 
	{
		int operator()( int N )
		{
			return NRandom::Random( N );
		}
	};

	struct SSeverityCountPredicate
	{
		float fCount;
		SSeverityCountPredicate() : fCount( 0 ) {  }
		void operator()( const CCommonUnit *pUnit );
	};
	//
	// 
	struct SFindByEnumeratorPredicate
	{
		IWorkerEnumerator * pEn;
		enum EForceType eType;
		SFindByEnumeratorPredicate( interface IWorkerEnumerator * pEnumerator, const enum EForceType eType ) : pEn( pEnumerator ), eType( eType ) {  }
		bool operator()( class CCommonUnit *pU1 );
	};
	//
	struct SFindBestByEnumeratorPredicate
	{
		IWorkerEnumerator * pEn;
		enum EForceType eType;
		float fRating;
		CPtr<CCommonUnit> pBest;
		SFindBestByEnumeratorPredicate( interface IWorkerEnumerator * pEn, const enum EForceType eType ) : pEn ( pEn ), eType( eType ) {  }
		void operator()( class CCommonUnit *pU1 );
	};
	//
	struct SCountPredicate
	{
		int nCount;
		SCountPredicate() : nCount( 0 ) {  }
		void operator() ( void * ) { ++nCount; }
	};
	//
	struct SDeadPredicate
	{
		// return true if unit is dead
		bool operator() ( class CCommonUnit * pUnit );
	};
	struct SInactiveUnitPredicate
	{
		int nParty;
		SInactiveUnitPredicate( int _nParty ) : nParty( _nParty ) {  }
		bool operator() ( class CCommonUnit * pUnit );
	};
};

#endif // __GENERAL_HELPER__

