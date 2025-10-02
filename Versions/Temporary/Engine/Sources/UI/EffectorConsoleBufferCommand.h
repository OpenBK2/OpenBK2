#pragma once



class CEffectorConsoleBufferCommand : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorConsoleBufferCommand)
	bool bFinished;
	string szEditBoxName;
public:
	virtual int operator&( IBinSaver &ss );

	virtual bool IsFinished() const { return bFinished; }
	virtual void Configure( const NDb::SUIStateBase *pCmd, interface IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, interface IScreen *pScreen, const bool bFastForward );
	virtual void Visit( interface IUIVisitor *pVisitor ) { }
	virtual void Reverse() {  }
};

