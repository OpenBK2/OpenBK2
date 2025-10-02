#ifndef __EFFECTOR_BUTTON_STATE_H__
#define __EFFECTOR_BUTTON_STATE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


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
	virtual void Configure( const NDb::SUIStateBase *_pCmd, interface IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, interface IScreen *pScreen, const bool bFastForward );
	virtual void Visit( interface IUIVisitor *pVisitor ) { }
	virtual void Reverse();
	virtual int operator&( IBinSaver &saver );
};

#endif //__EFFECTOR_BUTTON_STATE_H__
