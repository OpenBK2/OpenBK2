#pragma once

#include "ManuverStateDesc.h"
#include "../System/RandomGen.h"
namespace NDb
{
	struct SAIGameConsts;
}


//	CManuverBuilder ::


typedef list<int> CManuverIndices;
typedef hash_map<int/*EPlanesAttitude*/, CManuverIndices> CManuvers;
class CManuverBuilder 
{
	static CManuverStateDesc state;				// temprorary value, used inside CreateManuver()
	static vector<int> suitableIndeces;
	static CManuvers manuvers;

	struct SGRoundAttackTarget
	{
		ZDATA
		int nMaxAttakers;
		SAIAngle wStartAngle;
		vector<int> attackers;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nMaxAttakers); f.Add(3,&wStartAngle); f.Add(4,&attackers); return 0; }
		WORD GetAngle( int nAttacker ) const
		{
			return wStartAngle + 65535 / nMaxAttakers * nAttacker;
		}
		struct IManuver* CreateManuver( class CPlanesFormation * pPos, const CVec3 &vPos, int nAttacker );
		void IncreaseMaxAttackers()
		{
			nMaxAttakers += 2;
			attackers.resize( nMaxAttakers, 0 );
		}
		SGRoundAttackTarget() : nMaxAttakers( 9 ), attackers( nMaxAttakers, 0 ), wStartAngle( NRandom::Random(65535) ) { }
	};

	typedef hash_map<int, SGRoundAttackTarget> CGroundAttacks;
	typedef vector<pair<int,CVec3> > CAttackerOffsets;
	typedef hash_map<int, CAttackerOffsets> CAviaAttacks; 
	
	ZDATA
	CDBPtr<NDb::SAIGameConsts> pConsts;
	CAviaAttacks attacks;
	CGroundAttacks groundAttacks;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pConsts); f.Add(3,&attacks); f.Add(4,&groundAttacks); return 0; }
	// manuvers

	// choose maneuver to perform. return 0 if none chosen
	const SManuverDescriptor * Choose( const CManuverStateDesc &current, const class CPlanePreferences &pref ) const;

	// default plane's behaviour. _must_ always return non null IManuver
	struct IManuver * CreateDefaultManuver( const enum EPlanesAttitude att, class CPlanesFormation *pPos, class CPlanesFormation *pEnemy );
	
	enum EPlanesAttitude GetAttitude( class CPlanesFormation *pPlane, class CPlanesFormation *pEnemy ) const;

	CVec3 CalcAttackerPoint( class CPlanesFormation *pAttacker, class CPlanesFormation *pEnemy );
	int RegisterAsAttacker( class CPlanesFormation *pAttacker, class CPlanesFormation *pEnemy );

public:
	void Init( const NDb::SAIGameConsts *pConsts );
	void Clear() 
	{ 
		attacks.clear();
		groundAttacks.clear();
	}

	// manuvers for air fight.
	struct IManuver* CreateManuver( class CPlanesFormation *pPos, class CPlanesFormation *pEnemy );

	// manuvers for ground attack
	struct IManuver* CreateManuver( class CPlanesFormation *pPos, const CVec3 &vPos, int nTargetUniqueID );

	// for travel to point. suitable for fighter patrol, bombers, etc.
	// if bToHorisontal == true then plane will first start horizonal move, then point move
	struct IManuver* CreatePointManuver ( class CPlanesFormation *pPos, const CVec3 &vPoint, const bool bToHorisontal );
};


