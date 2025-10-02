
#pragma once
class CSoldier;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CRotatingFireplacesObject
{
	struct SUnitInfo
	{
		ZDATA
		CPtr<CSoldier> pSoldier;
		int nLastFireplace;
		NTimer::STime lastFireplaceChange;
		public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSoldier); f.Add(3,&nLastFireplace); f.Add(4,&lastFireplaceChange); return 0; }
		SUnitInfo() : pSoldier( 0 ), nLastFireplace( 0 ), lastFireplaceChange( 0 ) { }
	};

	ZDATA
	list<SUnitInfo> units;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&units); return 0; }
	//
	bool IsBetterToGoToFireplace( class CSoldier *pSoldier, const int nFireplace ) const;
public:
	CRotatingFireplacesObject() { }

	// вызывается после того, как юнита полностью добавили в объект
	// nFireplace - номер fireplace в том случае, если солдат добавляется в fireplace
	void AddUnit( class CSoldier *pSoldier, const int nFireplace );
	void DeleteUnit( class CSoldier *pSoldier );

	virtual void Segment();

	// можно ли менять слот у этого слодата
	virtual bool CanRotateSoldier( class CSoldier *pSoldier ) const = 0;
	// поставить солдата в place вместо сидящего там
	virtual void ExchangeUnitToFireplace( class CSoldier *pSoldier, int nFirePlace ) = 0;
	// количество fireplaces
	virtual const int GetNFirePlaces() const = 0;
	// солдат, сидящий в fireplace, если fireplace пуст, то возвращает 0
	virtual class CSoldier* GetSoldierInFireplace( const int nFireplace) const = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

