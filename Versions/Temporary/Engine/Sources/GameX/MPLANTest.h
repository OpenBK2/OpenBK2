#pragma once

class CLANTester : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CLANTester )
	ZDATA
	ZONSERIALIZE
public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); return 0; }
private:	
	void OnSerialize( IBinSaver &f )
	{
		NI_ASSERT( false, "CLANTester should not be serialized!" )
	}

	bool bIsServer;
	CPtr<interface IMPToUIManager> pMPManager;
	int nMySlot;
	hash_map<int,bool> gameClientsReady;
	bool bStarted;
	int nPlayersToWait;
	bool bAcceptSent;
	void CreateGame();
	void RunShellCommand( const wstring& wszCommand );

public:
	CLANTester();
	void SetMPManager( IMPToUIManager *_pMPManager ) { pMPManager = _pMPManager; }
	bool IsStarted() const { return bStarted; }
	void Start();
	void ClientInfoChanged( const int nClientID, const bool bReady );
	void SetMySlot( const int _nMySlot ) { nMySlot = _nMySlot; }
	void ClientRemoved( const int nClientID );
	void NewGameFound( const int nID, const string &szName );
	void EndGame();
	void AsyncDetected();
};


