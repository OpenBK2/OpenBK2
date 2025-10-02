#pragma once

// ************************************************************************************************************************ //
// **
// ** diplomacy info
// **
// **
// **
// ************************************************************************************************************************ //

enum EDiplomacyInfo
{
	EDI_ENEMY		=	0x01,
	EDI_FRIEND	=	0x02,
	EDI_NEUTRAL = 0x04
};

#pragma pack( 1 )
struct SSegment2Trench
{
	ZDATA
		int nSegmentID;													// маленький кусочек 
		int nEntrenchID;												// весь окоп
	ZEND public: int operator&( IBinSaver &f ) { f.Add(2,&nSegmentID); f.Add(3,&nEntrenchID); return 0; } private:
public:
	SSegment2Trench() : nSegmentID( 0 ), nEntrenchID( 0 ) { }
	SSegment2Trench( const int _nSegmentID, const int _nEntrenchID )
		: nSegmentID( _nSegmentID ), nEntrenchID( _nEntrenchID ) { }
};
struct SSoldier2Formation
{
	ZDATA
		int nSoldierID;
		int nFormationID;
	ZEND public: int operator&( IBinSaver &f ) { f.Add(2,&nSoldierID); f.Add(3,&nFormationID); return 0; } private:
public:
	SSoldier2Formation() : nSoldierID( 0 ), nFormationID( 0 ) { }
	SSoldier2Formation( const int _nSoldierID, const int _nFormationID )
		: nSoldierID( _nSoldierID ), nFormationID( _nFormationID ) { }
};
struct SShootArea
{
	enum EShootAreaType
	{
		ESAT_BALLISTIC = 0,
		ESAT_AA = 1,
		ESAT_LINE = 2,
		ESAT_RANGE_AREA = 3,
	};

	EShootAreaType eType;
	
	CVec3 vCenter3D;
	float fMinR, fMaxR;

	// углы задают конус стрельбы - против часовой стрелки
	WORD wStartAngle;
	WORD wFinishAngle;
	
	//
	SShootArea()
		: vCenter3D( VNULL3 ), fMinR( 0.0f ), fMaxR( 0.0f ), 
			wStartAngle( 65535 ), wFinishAngle( 65535 ), eType( ESAT_LINE ) { }

	const DWORD GetColor() const
	{
		static const DWORD colors[] = { 0x0000ff00, 0x000000ff, 0x00ff0000, 0x0000ff00 };
		NI_ASSERT( int( eType ) < 4, StrFmt( "Wrong type of area (%d)", (int)eType ) );
		return colors[eType];
	}

	const WORD GetMiniMapCircleColor() const
	{
		static const WORD colors[] = { 0xf0f0, 0xf00a, 0xff00, 0xf0f0 };
		NI_ASSERT( int( eType ) < 4, StrFmt( "Wrong type of area (%d)", (int)eType ) );
		return colors[eType];
	}

	const WORD GetMiniMapSectorColor() const
	{
		static const WORD colors[] = { 0xf0f0, 0xf00a, 0xff00, 0xf0f0 };
		NI_ASSERT( int( eType ) < 4, StrFmt( "Wrong type of area (%d)", (int)eType ) );
		return colors[eType];
	}
	
	bool operator!=( const SShootArea &arg ) const
	{
		return eType != arg.eType || vCenter3D != arg.vCenter3D || fMinR != arg.fMinR || fMaxR != arg.fMaxR ||
			wStartAngle != arg.wStartAngle || wFinishAngle != arg.wFinishAngle;
	}
	
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &eType );
		saver.Add( 2, &vCenter3D );
		saver.Add( 3, &fMinR );
		saver.Add( 4, &fMaxR );
		saver.Add( 5, &wStartAngle );
		saver.Add( 6, &wFinishAngle );
		return 0;	
	}
};

struct SShootAreas
{
	// выводить - последовательно, накладывая друг на друга, сначала areas[0], потом areas[1] и т.д.
	list<SShootArea> areas;

	virtual int operator&( interface IBinSaver &saver )
	{
		
		saver.Add( 1, &areas );
		return 0;
	}
};
#pragma pack()

