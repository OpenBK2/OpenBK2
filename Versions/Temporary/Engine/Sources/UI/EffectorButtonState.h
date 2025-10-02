
#pragma once

// change visual button state
class CEffectorButtonState : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorButtonState)
	bool bFinished;
	bool bForward;
	
	NDb::EButtonSubstateType eSubstate;
	float fWaitTime;
	float fElapsedTime;
	CPtr<CWindow> pWindow;
	bool bStarted;
public:
	CEffectorButtonState() {  }
	virtual bool IsFinished() const;
	virtual void Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse();
	virtual int operator&( IBinSaver &saver );
};


