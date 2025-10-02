#pragma once

#include "NetDriver.h"
#include "NetLowest.h"
/////////////////////////////////////////////////////////////////////////////////////
namespace NNet
{
/////////////////////////////////////////////////////////////////////////////////////
class CLoginSupport
{
public:
	typedef int TServerID;
	enum EState
	{
		INACTIVE,
		LOGIN,
		ACCEPTED,
		REJECTED
	};
	struct SLoginInfo
	{
		int nLoginAttempt;
		bool bWrongVersion;
		CNodeAddressSet localAddr;
		CMemoryStream pwd;
	};

	void Step( float fDeltaTime );
	bool CanSend() const { return fLoginTimeLeft < 0; }
	EState GetState() const { return state; }
	int GetRejectReason() const { return nLastReject; }
	void WriteLogin( CBitStream *pBits, const CNodeAddressSet &localAddr );
	bool ProcessLogin( const CNodeAddress &addr, CBitStream &bits, SLoginInfo *pRes );
	void AcceptLogin( const CNodeAddress &addr, CBitStream *pBits, const SLoginInfo &info, int *pnClientID, const CNodeAddressSet &localAddr );
	void RejectLogin( const CNodeAddress &addr, CBitStream *pBits, const SLoginInfo &info, int nReason );
	bool HasAccepted( const CNodeAddress &addr, const SLoginInfo &info );
	void ProcessAccepted( const CNodeAddress &addr, CBitStream &bits );
	void ProcessRejected( const CNodeAddress &addr, CBitStream &bits );
	void StartLogin( const CNodeAddress &addr, const CMemoryStream &buf );
	const CNodeAddress& GetLoginTarget() const { return server; }
	int GetSelfClientID() const { return nSelfClientID; }
	const CNodeAddressSet& GetTargetLocalAddr() const { return serverLocalAddr; }
	TServerID GetUniqueServerID() const { return uniqueServerID; }

	CLoginSupport( APPLICATION_ID _applicationID );
	void Init( APPLICATION_ID _applicationID ) { applicationID = _applicationID; } 
private:
	struct SAcceptedLogin
	{
		CNodeAddress addr;
		int nLoginAttempt;
		float fTimeLeft;
		int nClientID;
	};
	APPLICATION_ID applicationID;
	float fLoginTimeLeft, fLoginInterval;
	int nLoginAttempt;
	CNodeAddress server;
	CMemoryStream pwd;
	EState state;
	int nLastReject;
	list<SAcceptedLogin> acceptedList;
	int nClientIDTrack;
	int nSelfClientID;
	CNodeAddressSet serverLocalAddr;
	TServerID uniqueServerID;

	SAcceptedLogin& GetAcceptedLogin( const CNodeAddress &addr, const SLoginInfo &info );
};
/////////////////////////////////////////////////////////////////////////////////////
}

