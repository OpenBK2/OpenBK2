
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAntiArtillery;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAntiArtilleryManager
{
	// круги от выстрелов от собственной артиллерии для каждой из сторон
	typedef std::unordered_set<int> CAntiArtilleries;
	ZDATA
	std::vector<CAntiArtilleries> antiArtilleries;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&antiArtilleries); return 0; }
	static bool IsHeardForParty( CAntiArtillery *pAntiArt, const int nParty );
public:
	void Init();
	void Clear();

	void AddAA( CAntiArtillery *pAA );
	void RemoveAA( CAntiArtillery *pAA );
	void Segment();

	// не сэйвится!
	class CIterator
	{
		int nIterParty;
		int nCurParty;
		CAntiArtilleries::iterator curIter;

		public:
			CIterator( const int nParty );

			const CCircle operator*() const;
			CAntiArtillery* GetAntiArtillery() const;

			void Iterate();
			bool IsFinished() const;
	};

	friend class CIterator;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

