#pragma once
#include "B2_M1_Terrain/DBVSO.h"

struct SRailRoadSystem
{
	struct SRRInstance
	{
		ZDATA
		CDBPtr<SVSODesc>	pDescriptor;
		vector<SVSOPoint> points;
		float							fPointLength;					// Length between two points
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pDescriptor); f.Add(3,&points); f.Add(4,&fPointLength); return 0; }

		void Decompose( const float fPos, int *pPoint, float *pFraction ) const;
		const CVec2 GetPoint( const float fPos ) const;
		const float GetTrackLength() const { return ( points.size() - 1 ) * fPointLength; }
	};
	typedef vector< SRRInstance > CRRSegments;
	ZDATA
	CRRSegments segments;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&segments); return 0; }

	void Clear() { segments.clear(); }

	void AddRailRoad( const NDb::SVSOInstance *pVSO );
};

