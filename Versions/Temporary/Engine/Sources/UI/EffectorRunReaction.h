#pragma once

// run message reaction
class CEffectorRunReaction : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorRunReaction )
	//CD<SUIStateCommand> pCmd;
	std::string szFwd;
	std::string szBack;
	bool bFinished;
	bool bForward;
	std::string szAnimatedWindow;

public:
	CEffectorRunReaction() {}

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
