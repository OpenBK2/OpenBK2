#pragma once

#include "ListsSet.h"
#include "System/FreeIDs.h"
#include "Stats_B2_M1/AIUnitCmd.h"
#include "Common_RTS_AI/AIClasses.h"

struct IStaticPathFinder;
struct IStaticPath;
class CGroupMover;

struct SGroupPathInfo
{
	ZDATA
	CPtr<IStaticPath> pPath;
	CPtr<IStaticPathFinder> pPathFinder;
	int nSubGroup;
	BYTE cTileSize;
	EAIClasses aiClass;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPath); f.Add(3,&pPathFinder); f.Add(4,&nSubGroup); f.Add(5,&cTileSize); f.Add(6,&aiClass); return 0; }
public:
	SGroupPathInfo() : nSubGroup( -1 ), cTileSize( 0 ), aiClass( EAC_NONE ) { }
};

class CAICommand : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CAICommand );

	static CQueuesSet<SGroupPathInfo> paths;
	static CFreeIds cmdIds;

	ZDATA
		SAIUnitCmd unitCmd;
		int id;
		int nFlag;
		CObj<CGroupMover> pMover;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&unitCmd); f.Add(3,&id); f.Add(4,&nFlag); f.Add(5,&pMover); return 0; }
private:
	//
	void InitCmdId();
public:
	CAICommand() : id( 0 ), nFlag( -1 ) { }
	CAICommand( const SAIUnitCmd &unitCmd );
	CAICommand( const CAICommand &cmd );
	
	static void Clear() { paths.Clear(); cmdIds.Clear(); }

	SAIUnitCmd& ToUnitCmd() { return unitCmd; }
	const SAIUnitCmd& ToUnitCmd() const { return unitCmd; }
	
	bool IsFromAI()const{return unitCmd.bFromAI;}
	void SetFromAI( const bool bFromAI ){ unitCmd.bFromAI = bFromAI; }

	~CAICommand() 
	{ 
		if ( id != 0 )
		{
			paths.DelQueue( id );
			cmdIds.Return( id );
		}
	}

	const int GetID() const { return id; }

	struct IStaticPath* CreateStaticPath( class CCommonUnit *pUnit );

	const int GetFlag() const { return nFlag; }
	const void SetFlag( const int _nFlag ) { nFlag = _nFlag; }

	void AddUnit( CCommonUnit *pUnit );
	void DeleteUnit( const int nUnitID );

	friend class CGroupLogic;
	friend class CStaticMembers;
};


