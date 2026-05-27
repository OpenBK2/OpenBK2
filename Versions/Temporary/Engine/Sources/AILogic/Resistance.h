#pragma once

#include <algorithm>

//
// очаги сопротивления
struct SResistance
{
ZDATA
	int nCellNumber;
	int nWeight;
	int nWeightExceed;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&nCellNumber); f.Add(3,&nWeight); f.Add( 4, &nWeightExceed);return 0; }
public:
	SResistance() { Clear(); }
	SResistance( const int _nCellNumber, const float fWeight, int _nWeightExceed ) 
		: nCellNumber( _nCellNumber ), nWeight( fWeight * 100 ), nWeightExceed( _nWeightExceed ) { }

	void Clear(){ nCellNumber = -1; nWeight = -1.0f; }
	const float GetWeight() const { return (float)nWeight / 100.0f; }
	bool IsWeightExceed() const { return nWeightExceed != 0; }
	const bool IsInitted() const { return nWeight != -1; }
	const int GetCellNumber() const { return nCellNumber; }
	const CVec2 GetResistanceCellCenter() const { return GetResistanceCellCenter( nCellNumber ); }
	void SetWeight( float fWeight ) { nWeight = fWeight * 100; }

	static const CVec2 GetResistanceCellCenter( const int nCell );

	bool operator==( const SResistance &res ) const
	{
		return nCellNumber == res.nCellNumber;
	}
};

struct SResistanceCmp
{
	bool operator()( const SResistance &r1, const SResistance &r2 ) const
	{
		if (r1.GetWeight() != r2.GetWeight())
			return r1.GetWeight() > r2.GetWeight();
		
		return r1.nCellNumber > r2.nCellNumber;
	}
};

typedef std::vector<SResistance> CResistance;

class CResistancesContainer
{
	struct SSellInfo
	{
		float fCellWeight;									// net weight for this cell
		bool bInUse;												// this cell is being attacked
		bool bAllowShoot;										// shooting to the cell is allowed

		SSellInfo() : fCellWeight( 0.0f ), bInUse( false ), bAllowShoot( true ) { }
		SSellInfo( const float _fCellWeight, const bool _bInUse, const bool _bAllowShoot ) : fCellWeight( _fCellWeight ), bInUse( _bInUse ), bAllowShoot( _bAllowShoot ) { }
	};
	typedef std::unordered_map<int, SSellInfo> CCellsWeights;

	typedef std::unordered_map<int, int> CCellsWithGoodWeight;

	ZDATA
	CResistance resistances;

	CCellsWeights cellsWeights;
	std::list<CCircle> excluded;				// general will not shoot to these circles
	CCellsWithGoodWeight goodWeight;

	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&resistances); f.Add(3,&cellsWeights); f.Add(4,&excluded); f.Add(5,&goodWeight); return 0; }
	class CIter
	{
		CResistance::iterator iter;
		CResistancesContainer *pContainter;

		CIter() { }
		void IterateToNotInUse();
	public:
		CIter( CResistancesContainer *_pContainter )
			: pContainter( _pContainter )
		{
			SResistanceCmp cmp;
			std::sort( pContainter->resistances.begin(), pContainter->resistances.end(), cmp );
			iter = pContainter->resistances.begin(); 
			IterateToNotInUse(); 
		}

		void Iterate();
		bool IsFinished() const { return iter == pContainter->resistances.end(); }
		const SResistance& operator*() const { NI_ASSERT( !IsFinished(), "Can't call operator *" ); return *iter;	}
	};

	const int GetResistanceCellNumber( const CVec2 &vPos );
	bool IsCellExcluded( const CVec2 &vCellCenter );

	
public:
	CResistancesContainer() { }

	void Clear() { resistances.clear(); cellsWeights.clear(); }

	void UpdateEnemyUnitInfo( class CAIUnitInfoForGeneral *pInfo,
			const NTimer::STime lastVisibleTimeDelta, const CVec2 &vLastVisiblePos,
			const NTimer::STime lastAntiArtTimeDelta, const CVec2 &vLastVisibleAntiArtCenter, const float fDistToLastVisibleAntiArt );			

	void UnitDied( CAIUnitInfoForGeneral *pInfo );
	void UnitChangedParty( CAIUnitInfoForGeneral *pInfo );

	bool IsEmpty() const { return resistances.empty(); }

	void SetCellInUse( const int nResistanceCellNumber, bool bInUse );
	bool IsInUse( const int nResistanceCellNumber );

	void RemoveExcluded( const CVec2 &vCenter );
	void AddExcluded( const CVec2 &vCenter, const float fRadius );
	bool IsInResistanceCircle( const CVec2 &vCenter ) const;

	typedef CIter iterator;
	friend class CIter;

	iterator begin() { return CIter( this ); }
};


