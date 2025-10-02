#ifndef __INTERFACE_MP_LADDER_STATISTICS_H__
#define __INTERFACE_MP_LADDER_STATISTICS_H__
#pragma once
#include "InterfaceScreenBase.h"
#include "InterfaceMPBase.h"

namespace NDb
{
	struct SMedal;
}

class CInterfaceMPLadderStatistics : public CInterfaceScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPLadderStatistics );

	struct SMedalDesc
	{
		string szControlName;
		CDBPtr<NDb::SMedal> pMedal;
	};

	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	CPtr<CInterfaceMPScreenBase> pPrevMPScreen;
	CPtr<ITextView> pWaiting;
	CPtr<IScrollableContainer> pAdvList;
	CPtr<IWindow> pAdv4Template;
	CPtr<IWindow> pAdv2Template;
	CPtr<ITextView> pAdv1Template;
	vector<wstring> countryNames;
	vector<SMedalDesc> medals;
	CPtr<IWindow> pMedalPopup;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(3,&pPrevMPScreen); f.Add(4,&pWaiting); f.Add(5,&pAdvList); f.Add(6,&pAdv4Template); f.Add(7,&pAdv2Template); f.Add(8,&pAdv1Template); f.Add(9,&countryNames); return 0; }
private:
	bool OnBackReaction(); 
	void OnLadderStatsMessage( struct SMPUILadderStatsMessage *pMsg );
	void Add4Line( const wstring &wszItem1, const wstring &wszItem2, const wstring &wszItem3, const wstring &wszItem4 );
	void Add2Line( const wstring &wszItem1, const wstring &wszItem2 );
	void Add1Line( const wstring &wszItem );
	void AddWinLoseSummary( const SLadderStatistics &info, const wstring &wszName, int nUseSolo, int nUseTeam );
	bool OnShowMedal( const string &szSender );

protected:
	~CInterfaceMPLadderStatistics();
public:
	CInterfaceMPLadderStatistics();
	bool Init();
	bool StepLocal( bool bAppActive );
	void RequestInfo( const string &szNick );
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;	
	//}
};

class CICMPLadderStatistics : public CInterfaceCommandBase<CInterfaceMPLadderStatistics>
{
	OBJECT_BASIC_METHODS( CICMPLadderStatistics );
	//
	string szNick;
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

#endif //__INTERFACE_MP_LADDER_STATISTICS_H__

