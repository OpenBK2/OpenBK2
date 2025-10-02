#pragma once


// sends UI message
// SBUIMessage( cmd.szParam1, 
//							cmd.szParam2,
//							bForward ? LOWORD(cmd.nParam1) : HIWORD(cmd.nParam1) )
class CEffectorSendUIMessage : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorSendUIMessage );
	CPtr<CWindow> pElement;
	string szMessageID;
	string szParam;
	int nForwardParam;
	int nBackParam;
	string szAnimatedWindow;

	bool bForward;
	bool bFinished;
public:
	CEffectorSendUIMessage() {  }
	virtual int operator&( IBinSaver &saver );
	virtual bool IsFinished() const { return bFinished; }
	virtual void Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse();
};


