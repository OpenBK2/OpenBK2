#ifndef _EffectorRunReaction_h_Included_
#define _EffectorRunReaction_h_Included_


// run message reaction
class CEffectorRunReaction : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorRunReaction)
	//CD<SUIStateCommand> pCmd;
	string szFwd;
	string szBack;
	bool bFinished;
	bool bForward;
	string szAnimatedWindow;
public:
	CEffectorRunReaction() {  }
	virtual bool IsFinished() const;
	virtual void Configure( const NDb::SUIStateBase *_pCmd, interface IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, interface IScreen *pScreen, const bool bFastForward );
	virtual void Visit( interface IUIVisitor *pVisitor ) { }
	virtual void Reverse();
	virtual int operator&( IBinSaver &saver );
};

#endif //_EffectorRunReaction_h_Included_
