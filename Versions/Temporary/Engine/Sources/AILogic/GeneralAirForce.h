#ifndef __GENERAL_AIR_FORCE__
#define __GENERAL_AIR_FORCE__

#include "..\System\FreeIDs.h"
#include "Commander.h"

namespace NDb
{
	enum EReinforcementType;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
class CEnemyRememberer;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// управляет самолетами и иx вылетом для всей стороны
class CGeneralAirForce : public CCommander
{
	friend class CGeneralAirForceLaunchFighters;
	OBJECT_BASIC_METHODS(CGeneralAirForce);
	
	struct SSameEnemyPointPredicate
	{
		bool operator()( const CVec2 &v1, const CVec2 &v2 ) {	return fabs2(v1-v2) < sqr(SConsts::PLANE_GUARD_STATE_RADIUS/2); }
	};

	typedef hash_map< int, CPtr<CEnemyRememberer> > AntiAviation;

	interface IEnemyContainer *pEnemyContainer;

	struct SSupportInfo
	{
		CVec2 vPoint;												// куда вызвали
		int nResistanceCellNumber;					// если вызвали пробомбить точку сопротивления, то > 0

		SSupportInfo() : vPoint( VNULL2 ), nResistanceCellNumber( -1 ) { }
	};

	ZDATA_(CCommander)
	int nParty;
	vector<int> players;							// номера игроков, которые находятся под управлением 
	CFreeIds requestsID;

	AntiAviation antiAviation;

	vector<CPtr<CAIUnit> > createdAviation;
	NTimer::STime timeWaitForReinforceSystem;
	EForceType nCurrentRequest;									// current aviation, that general is wating for
	bool bOurTurn;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CCommander*)this); f.Add(2,&nParty); f.Add(3,&players); f.Add(4,&requestsID); f.Add(5,&antiAviation); f.Add(6,&createdAviation); f.Add(7,&timeWaitForReinforceSystem); f.Add(8,&nCurrentRequest); f.Add(9,&bOurTurn); return 0; }
public:
	typedef hash_map< int /*request ID*/, SSupportInfo > Requests;
	typedef hash_map<int, Requests> RequestsByForceType;
private:

	RequestsByForceType requests;

	void LaunchScoutFree( const int nPlayer, const NDb::EReinforcementType eType );
	bool PrepeareFighters( int nPlayer );
	void CallReinforcement( const int nPlayer, NDb::EReinforcementType eType, RequestsByForceType *pRequests, EForceType eForceType );
	void GiveOrders( const int nPlayer );

	void LaunchPlane( EForceType eType, const list<CVec2> &vPoints, const int nPlayer );

	// returns 0 if line is safe to fly.
	// otherwize returns severty( how many planes will die while flying by this line )
	float CheckLineForSafety( const CVec2 &vStart, const CVec2 &vFinish, const float fFlyHeight );

	void InitCheckPeriod();
	void InitFighterCheckPeriod();
public:
	CGeneralAirForce() {  }
	CGeneralAirForce( const int nPlayer, IEnemyContainer *pEnemyContainer ) ;

	void Segment();
	void Give( CCommonUnit *pWorker );
	void EnumWorkers( const EForceType eType, IWorkerEnumerator *pEnumerator ) {  }


	int /*request ID*/RequestForSupport( const CVec2 &vSupportCenter, enum EForceType eType, int nResistanceCellNumber = -1 );
	void CancelRequest( int nRequestID, enum EForceType eType );

	void SetEnemyContainer( IEnemyContainer * _pEnemyConatainer )
	{
		pEnemyContainer = _pEnemyConatainer;
	}

	void SetAAVisible( class CAIUnit *pUnit, const bool bVisible );
	void DeleteAA( class CAIUnit * pUnit );

	void PassTurn();
	bool TurnReturned() const;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __GENERAL_AIR_FORCE__
