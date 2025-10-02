// EffectorMoveTo.h: interface for the CEffectorMoveTo class.
//


#pragma once

class CEffectorMoveTo : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorMoveTo)

	CPtr<CWindow> pElement;
	bool bFinished;
	CVec2 vMoveOffset;
	float fMoveTime;												// points per second
	CVec2 vSpeed;														// speed
	float fElapsedTime;											// time elapsed so far
	
	CVec2 vMoveFrom;
	bool bForward;

	const pair<CVec2,int> GetCur() const;
public:

	virtual int operator&( IBinSaver &ss );

	virtual bool IsFinished() const { return bFinished; }
	virtual void Configure( const NDb::SUIStateBase *pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse();
};

