#ifndef __INTERFACE_NIVAL_NET_H__
#define __INTERFACE_NIVAL_NET_H__

#pragma ONCE

#include "InterfaceMPBase.h"

class CInterfaceNivalNet : public CInterfaceMPScreenBase, 
	public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceNivalNet );
	
	enum EState
	{
		ES_NORMAL,
		ES_REGISTER,
		ES_LOGIN,
		ES_RECOVER
	};

	ZDATA_(CInterfaceMPScreenBase)
	CPtr<IWindow> pMain;
	
	CPtr<IWindow> pFrameRegister; //three main windows.
	CPtr<IWindow> pFrameLogin;
	CPtr<IWindow> pFrameRecovery;
	
	CPtr<IButton> pLogin;
	CPtr<IButton> pBack;
	CPtr<IButton> pRegistration;
	CPtr<IButton> pRecovery;
	
	CPtr<IButton> pRegisterOk;
	CPtr<IButton> pRegisterCancel;

	CPtr<IButton> pRecoverCancel;
	CPtr<IButton> pRecoverOk;

	CPtr<IEditLine> pLoginLoginEdit;
	CPtr<IEditLine> pLoginPasswordEdit;
	CPtr<IEditLine> pUserNameRecovery;
	CPtr<IEditLine> pCDKey;
	CPtr<IEditLine> pConfirmEmail;
	CPtr<IEditLine> pConfirmPassword;
	CPtr<IEditLine> pEmail;
	CPtr<IEditLine> pRegistrationPassword;
	CPtr<IEditLine> pRegistrationUserName;

	CPtr<IWindow> pWaitWindow;
	
	EState eState;
	CPtr<IButton> pRememberPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceMPScreenBase*)this); f.Add(2,&pMain); f.Add(3,&pFrameRegister); f.Add(4,&pFrameLogin); f.Add(5,&pFrameRecovery); f.Add(6,&pLogin); f.Add(7,&pBack); f.Add(8,&pRegistration); f.Add(9,&pRecovery); f.Add(10,&pRegisterOk); f.Add(11,&pRegisterCancel); f.Add(12,&pRecoverCancel); f.Add(13,&pRecoverOk); f.Add(14,&pLoginLoginEdit); f.Add(15,&pLoginPasswordEdit); f.Add(16,&pUserNameRecovery); f.Add(17,&pCDKey); f.Add(18,&pConfirmEmail); f.Add(19,&pConfirmPassword); f.Add(20,&pEmail); f.Add(21,&pRegistrationPassword); f.Add(22,&pRegistrationUserName); f.Add(23,&pWaitWindow); f.Add(24,&eState); f.Add(25,&pRememberPassword); return 0; }
private:
	void RegisterObservers();
	
	void MakeInterior();
	void UpdateInterior();
	void UpdateInteriorLogin();
	void UpdateInteriorRegister();
	void UpdateInteriorRecover();
	bool IsEmailValid( const wstring &wszEmail );

	//{
	bool OnConnectResultMessage( SMPUIConnectResultMessage *pMsg );
	//}

	//{
	bool OnBackReaction( const string &szSender );
	bool OnLoginReaction( const string &szSender );
	bool OnLoginRegister();
	bool OnLoginRecovery();

	bool OnRegisterReaction( const string &szSender );

	bool OnRegisterOkReaction( const string &szSender );
	bool OnRegisterCancelReaction( const string &szSender );
	bool CheckRegistrationData( string &szReason );

	bool OnRecoveryOkReaction( const string &szSender );
	bool OnRecoveryCancelReaction( const string &szSender );
	//}
protected:
	~CInterfaceNivalNet();
public:
	CInterfaceNivalNet();

	bool Init();
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

#ifndef _SINGLE_DEMO
class CICNivalNet : public CInterfaceCommandBase<CInterfaceNivalNet>
{
	OBJECT_BASIC_METHODS( CICNivalNet );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};
#endif // _SINGLE_DEMO

#endif //__INTERFACE_NIVAL_NET_H__
