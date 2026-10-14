
#pragma once

// change visual button state
class CEffectorButtonState : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorButtonState )
	bool bFinished;
	bool bForward;

	NDb::EButtonSubstateType eSubstate;
	float fWaitTime;
	float fElapsedTime;
	CPtr<CWindow> pWindow;
	bool bStarted;

public:
	CEffectorButtonState() {}

	bool IsFinished() const override;
	void Configure( const NDb::SUIStateBase *_pCmd,
	                struct IScreen *pScreen,
	                SWindowContext *pContext,
	                const std::string &szAnimatedWindow ) override;
	const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward ) override;

	void Visit( struct IUIVisitor *pVisitor ) override {}

	void Reverse() override;
	int operator&( IBinSaver &saver ) override;
};
