
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHitsStore
{
	ZDATA
	std::vector<CArray2D<BYTE> > hits;
	BYTE curIndex;
	NTimer::STime timeOfIndexBegin;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&hits); f.Add(3,&curIndex); f.Add(4,&timeOfIndexBegin); return 0; }

public:
	enum EHitTypes { EHT_OPEN_SIGHT = 0x1, EHT_OVER_SIGHT = 0x10, EHT_ANY = 0x11 };

	CHitsStore()
	{
		hits.resize( 2 );
	}
	void Init( const int nMapSizeX, const int nMapSizeY );
	void Clear();
	void Segment();

	void AddHit( const CVec2 &center, const EHitTypes eHitType );
	bool WasHit( const CVec2 &center, const float fR, const EHitTypes eHitType ) const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

