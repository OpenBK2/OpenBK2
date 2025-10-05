#pragma once



class CEffectorConsoleBufferCommand : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorConsoleBufferCommand)
	bool bFinished;
	std::string szEditBoxName;
public:
	virtual int operator&( IBinSaver &ss );

	virtual bool IsFinished() const { return bFinished; }
	virtual void Configure( const NDb::SUIStateBase *pCmd, struct IScreen *pScreen, SWindowContext *pContext, const std::string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse() {  }
};


