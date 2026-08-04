#pragma once

enum EUpdateWarFogUnitInfoFlag
{
	UPD_CREATE_NEW_UNIT,			// level 0
	UPD_DELETE_UNIT,					// level 0
	UPD_CHANGE_PARTY,					// level 0
	UPD_UPDATE_VISIBILITY,		// level 1, need recalculate tiles' visibility
	UPD_UPDATE_PROPERTIES,		// level 2
	UPD_UPDATE_POSITION,			// level 3, 4 ( maxRadius x 2 ), 5 ( maxRadius / 2 )
	UPD_UPDATED,							// no need update
};

struct SUpdateUnitInfo
{
	EUpdateWarFogUnitInfoFlag updateFlag;
	float fDist;

	SUpdateUnitInfo() : updateFlag(), fDist( 0.0f ) {}
	SUpdateUnitInfo( const EUpdateWarFogUnitInfoFlag _updateFlag, const float _fDist )
		: updateFlag( _updateFlag ), fDist( _fDist ) {}

};

typedef det_map<int, SUpdateUnitInfo> CUpdateUnitList;

class CUpdateUnitContainer : public CAIObjectBase
{
	OBJECT_NOCOPY_METHODS( CUpdateUnitContainer );

	std::vector<CUpdateUnitList> updateLists;
	float f2MaxRadius, fMaxRadius2;

public:
	CUpdateUnitContainer() : f2MaxRadius( 0.0f ), fMaxRadius2( 0.0f ) { }
	void Init( const float fMaxRadius );
	void Clear();
	void Push( const int nID, const EUpdateWarFogUnitInfoFlag updateFlag, const float fDist );
	int Pop();
	bool IsEmpty();

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &updateLists );
		saver.Add( 2, &f2MaxRadius );
		saver.Add( 3, &fMaxRadius2 );
		return 0;
	}
};

