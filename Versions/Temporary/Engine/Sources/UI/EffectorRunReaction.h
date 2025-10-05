#pragma once


// run message reaction
class CEffectorRunReaction : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorRunReaction)
	//CD<SUIStateCommand> pCmd;
	std::string szFwd;
	std::string szBack;
	bool bFinished;
	bool bForward;
	std::string szAnimatedWindow;
public:
	CEffectorRunReaction() {  }
	virtual bool IsFinished() const;
	virtual void Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const std::string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse();
	virtual int operator&( IBinSaver &saver );
};


