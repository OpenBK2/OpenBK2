#pragma once

#include "NetPacket.h"
#include "LadderStatistics.h"
#include "System/XmlSaver.h"

class CLadderInfoPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderInfoPacket )
	ZDATA
public:
		int nSide; // 0 - random, 1 - allies, 2 - fascist
	bool bHistoricity;
	int nTeamSize; // 0 - random
	std::list<int> techLevels;
	std::list<int> maps;
	unsigned uCheckSum;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSide); f.Add(3,&bHistoricity); f.Add(4,&nTeamSize); f.Add(5,&techLevels); f.Add(6,&maps); f.Add(7,&uCheckSum); return 0; }
	int operator&( IXmlSaver &f ) { 
		f.Add( "nSide", &nSide ); f.Add( "bHistoricity", &bHistoricity ); f.Add( "nTeamSize", &nTeamSize );
		f.Add( "techLevels", &techLevels ); f.Add( "maps", &maps ); 
		return 0;
	}
	CLadderInfoPacket() {}
	CLadderInfoPacket( const int nClientID, const int _nSide, const bool _bHistoricity, const int _nTeamSize, const std::list<int> &_techLevels, const std::list<int> &_maps, const unsigned _uCheckSum )
		: CNetPacket( nClientID ), nSide( _nSide ), bHistoricity( _bHistoricity ), nTeamSize( _nTeamSize ), techLevels( _techLevels ), maps( _maps ), uCheckSum( _uCheckSum ) {} 
};

class CLadderGameResultPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderGameResultPacket )
		ZDATA
public:
	int nGameID;
	std::unordered_set<int> winners;
	std::unordered_map<int,int> races;
	std::unordered_map<int,std::vector<int> > reinfUsed;
	std::unordered_map<int,int> unitsKilled;
	std::unordered_map<int,int> unitsLost;
	std::unordered_map<int,int> playerUnitEff;
	std::unordered_map<int,int> playerKeyPointEff;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&winners); f.Add(4,&races); f.Add(5,&reinfUsed); f.Add(6,&unitsKilled); f.Add(7,&unitsLost); f.Add(8,&playerUnitEff); f.Add(9,&playerKeyPointEff); return 0; }
	int operator&( IXmlSaver &f ) { 
		f.Add( "nGameID", &nGameID ); f.Add( "winners", &winners ); f.Add( "races", &races ); f.Add( "units", &reinfUsed );
		return 0;
	}
	CLadderGameResultPacket() {}
	CLadderGameResultPacket( const int nClientID, const int _nGameID, 
		const std::unordered_set<int> &_winners, const std::unordered_map<int,int> &_races )
		: CNetPacket( nClientID ),  nGameID( _nGameID ), winners( _winners ), races( _races ) {}
	CLadderGameResultPacket( const int nClientID, const int _nGameID, 
		const std::unordered_set<int> &_winners, const std::unordered_map<int,int> &_races, const std::unordered_map<int, std::vector<int> > &_reinfUsed )
		: CNetPacket( nClientID ),  nGameID( _nGameID ), winners( _winners ), races( _races ), reinfUsed( _reinfUsed ) {}

};

class CLadderInvitePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderInvitePacket )
	ZDATA
public:
	int nGameID;
	int nMapID;
	int nTechLevel;
	std::list<int> team1;				// If Historicity is OFF, these are not Germans
	std::list<int> team2;				// If Historicity is ON, these are Germans
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&nMapID); f.Add(4,&nTechLevel); f.Add(5,&team1); f.Add(6,&team2); return 0; }
	int operator&( IXmlSaver &f ) { 
		f.Add( "nGameID", &nGameID );
		f.Add( "nMapID", &nMapID ); f.Add( "nTechLevel", &nTechLevel );
		return 0;
	}
	CLadderInvitePacket() {}
	CLadderInvitePacket( const int nClientID, const int _nGameID, const int _nMapID, const int _nTechLevel, 
		const std::list<int> &_team1, const std::list<int> &_team2 )
		: CNetPacket( nClientID ), nGameID( _nGameID ), nMapID( _nMapID ), nTechLevel( _nTechLevel ),
			team1( _team1 ), team2( _team2 ) 	{} 
};

class CLadderStatisticsRequestPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderStatisticsRequestPacket );
	ZDATA
public:
	std::string szNick;
	bool bSendFullStatistics;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&bSendFullStatistics); return 0; }
	CLadderStatisticsRequestPacket() {}
	CLadderStatisticsRequestPacket( int nClientID, const std::string &_szNick, const bool &_bSendFullStatistics )
		: CNetPacket( nClientID ), szNick( _szNick ), bSendFullStatistics( _bSendFullStatistics ) {}
};

class CLadderStatisticsPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderStatisticsPacket )
	ZDATA
public:
	std::string szNick;
	SLadderStatistics info;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&info); return 0; }
	CLadderStatisticsPacket() {}
	CLadderStatisticsPacket( int nClientID, const std::string &_szNick, const SLadderStatistics &_info )
		: CNetPacket( nClientID ), szNick( _szNick ), info( _info ) {}
};

class CLadderShortStatisticsPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderShortStatisticsPacket )
	ZDATA
public:
	std::string szNick;
	int nLevel;
	int nRace;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&nLevel); f.Add(4,&nRace); return 0; }
	CLadderShortStatisticsPacket() {}
	CLadderShortStatisticsPacket( const int nClientID, const std::string &_szNick, const int _nLevel, const int _nRace )
		: CNetPacket( nClientID ), szNick( _szNick ), nLevel( _nLevel ), nRace( _nRace ) {}
};

class CLadderSurrenderPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderSurrenderPacket )
	ZDATA
public:
	int nGameID;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); return 0; }
	CLadderSurrenderPacket() {}
	CLadderSurrenderPacket( const int nClientID, const int _nGameID ) : CNetPacket( nClientID ), nGameID( _nGameID ) {}
};

class CLadderInvalidStatisticsPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLadderInvalidStatisticsPacket )
	ZDATA
public:
	int nGameID;
	int nReason;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&nReason); return 0; }
	CLadderInvalidStatisticsPacket() {}
	CLadderInvalidStatisticsPacket( const int nClientID, const int _nGameID, const int _nReason ) 
		: CNetPacket( nClientID ), nGameID( _nGameID ), nReason( _nReason	) {}
};


