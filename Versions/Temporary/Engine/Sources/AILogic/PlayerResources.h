#ifndef __PLAYER_RESOURCES_H__
#define __PLAYER_RESOURCES_H__

#pragma once
/*


#define NUMBER_OF_UNIT_TYPES			37
#define NUMBER_OF_REINF_TYPES			15
namespace NDb
{
	struct SMapPlayerInfo;
}

class CPlayerResourcesTracker
{
	// reinforcement point
	struct SPoint
	{
		bool bEnabled;
		NDb::EReinforcementType eType;
		SPoint() : bEnabled( false ), eType( NDb::_RT_NONE ) {  }
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &bEnabled );
			saver.Add( 2, &eType );
			return 0;
		}
	};
	struct SReinforcementInfo
	{
		CDBPtr<NDb::SReinforcement> pReinforcement;
		CDBPtr<NDb::SDeployTemplate> pDeployTemplate;
		SReinforcementInfo() {  }
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &pReinforcement );
			saver.Add( 2, &pDeployTemplate );
			return 0;
		}
	};
	vector<SReinforcementInfo> reinforcementInfo;			// reinforcements by type (EReinforcementType)

	typedef hash_map<int, SPoint> CPoints;	
	typedef hash_map<int, CPoints> CFactories; 

	CFactories factories;

	float fMaxPoints;             // max points to own
	float fMaxPointsToOrder;
	float fPointsOrdered;
	float fCurrentPoints;
	int nPlayer;

	CArray2D<int>	unitAvailability;
	
	static float GetReinfPrice( const NDb::SReinforcement *pReinf );

	void BalanceReinfAdd( const NDb::SReinforcement *pReinf );
	void BalanceReinfSubtract( const NDb::SReinforcement *pReinf );
	
	
	void UpdateEntry( const SReinforcementInfo &entry, const int nFactoryID, const int nPositionID, const bool bEnabled ) const;
	pair<int,int> GetFactoryPointByType( const NDb::EReinforcementType eType );
public:

	CPlayerResourcesTracker() : nPlayer( -1 ), fMaxPoints( 0 ), fCurrentPoints( 0 ), fPointsOrdered( 0 ), fMaxPointsToOrder( 0 ) {}
	
	void Init( const int nPlayerIndex, const NDb::SMapPlayerInfo &playerInfo );
	// register available reinforcement type
	void AddAvailReinforcementType( const int nFactoryID, const int nPositionID, const NDb::EReinforcementType eType );
	// unregister unavailable reinforcement type
	void RemoveAvailReinforcementType( const int nFactoryID, const int nPositionID );
	// modify reinforcement in mission
	void SetReinforcement( const NDb::SReinforcement *pReinf );
	void RemoveReinforcement( const NDb::EReinforcementType eType );
	void SetDeploy( const NDb::SDeployTemplate *pTemplate );
	void SetPosition( const NDb::SReinforcementPosition &position );
	bool HasReinforcement( const NDb::EReinforcementType eType );
	const NDb::SDeployTemplate* GetDeploy( const NDb::EReinforcementType eType );
	// order
	void MakeOrder( const int nFactoryID, const NDb::EReinforcementType eType );
	const NDb::SHPObjectRPGStats * GetUnitSample( const int nFactoryID, const NDb::EReinforcementType eType, const int nEntry );
	// For AI calling reinfs
	NTimer::STime GetNextReinfTime();
	int GetUnitAvailability( const NDb::EDBUnitRPGType eUnit, const NDb::EReinforcementType eReinf );
	int  GetRefreshedFactoryIDForReinforcement( const NDb::EReinforcementType eType );
	// force unit update
	void RegisterUnit( const float fPrice );
	void UnRegisterUnit( const float fPrice );
	// client updates
	void Update( const int nFactoryID, const int nPositionID ) const;
	void Update() const;
	
	int operator&( IBinSaver &saver );
};

namespace NDb
{
	struct SMapPlayerInfo;
}

class CGlobalStock
{
	vector<CPlayerResourcesTracker> trackers;
	int nCurLinkID;
public:
	CGlobalStock() : nCurLinkID( 65535 ) {}
	void Init( const vector<NDb::SMapPlayerInfo> &info );
	void Clear() { trackers.resize( 0 ); }
	int GetLinkID() { ++nCurLinkID; return nCurLinkID; }
	void ForcedUpdate();
	int operator&( IBinSaver &saver );
	CPlayerResourcesTracker* operator[]( const int &nIndex );
};*/

#endif // __PLAYER_RESOURCES_H__
