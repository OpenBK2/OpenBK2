#pragma once
namespace NDb
{
	struct SMultiplayerMap;
}

struct SMPSlot
{
	ZDATA
	int nClientID;		// 
	bool bPresent;		// 
	std::string szName;
	int nTeam;				// 0 or 1
	int nCountry;			// from Sides table in MultiplayerConsts
	int nColour;
	bool bAccept;			// true means no changes possible (including closed slot)
	bool bRandomCountry;
	int nPing;				// -1 means no data
	WORD wConnectedTo;

	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nClientID); f.Add(3,&bPresent); f.Add(4,&szName); f.Add(5,&nTeam); f.Add(6,&nCountry); f.Add(7,&nColour); f.Add(8,&bAccept); f.Add(9,&bRandomCountry); f.Add(10,&nPing); f.Add(11,&wConnectedTo); return 0; }

	SMPSlot() : nClientID(-1), bPresent(false), szName(""), nTeam(0), nCountry(0), bRandomCountry(true)
		, nColour(0), bAccept(false), nPing(-1) , wConnectedTo(0) {}

	void Clear() 
	{
		nClientID = -1;
		bPresent = false;
		szName.clear();
		nTeam = 0;
		nCountry = 0;
		bRandomCountry = true;
		nColour = 0;
		bAccept = false;
		nPing = -1;
		wConnectedTo = 0;
	}

	SMPSlot& operator=( const SMPSlot &src ) 
	{ 
		//nClientID = src.nClientID;		// Client ID not assigned, it is different in each case
		bPresent = src.bPresent;
		szName = src.szName;
		nTeam = src.nTeam;
		nCountry = src.nCountry;
		bRandomCountry = src.bRandomCountry;
		nColour = src.nColour;
		bAccept = src.bAccept;
		nPing = src.nPing;
		wConnectedTo = src.wConnectedTo;
		return *this;
	}
};

struct SB2GameSpecificData 
{
public:
	enum EGameType
	{
		EGT_FLAG_CONTROL_2,
	};

	ZDATA		
	CDBPtr<NDb::SMultiplayerMap> pMPMap;
	int nPlayers;					// Actual number of players allowed
	int nTimeLimit;
	int nGameSpeed;
	int nTechLevel;
	bool bUnitExp;
	int nCaptureTime;
	EGameType eType;
	bool bRandomPlacement;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMPMap); f.Add(3,&nPlayers); f.Add(4,&nTimeLimit); f.Add(5,&nGameSpeed); f.Add(6,&nTechLevel); f.Add(7,&bUnitExp); f.Add(8,&nCaptureTime); f.Add(9,&eType); f.Add(10,&bRandomPlacement); return 0; }
	SB2GameSpecificData() : nPlayers( 0 ), nTimeLimit( 0 ), nGameSpeed( 0 ), nTechLevel( 0 ), 
		bUnitExp( false ), nCaptureTime( 0 ), eType( EGT_FLAG_CONTROL_2 ), bRandomPlacement( true ) {}
	SB2GameSpecificData& operator=( const SB2GameSpecificData &src )
	{
		pMPMap = src.pMPMap;
		nPlayers = src.nPlayers;
		nTimeLimit = src.nTimeLimit;
		nGameSpeed = src.nGameSpeed;
		nTechLevel = src.nTechLevel;
		bUnitExp = src.bUnitExp;
		nCaptureTime = src.nCaptureTime;
		eType = src.eType;
		bRandomPlacement = src.bRandomPlacement;
		return *this;
	}
};


