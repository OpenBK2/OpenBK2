// EffectorMoveTo.h: interface for the CEffectorMoveTo class.
//

#if !defined(AFX_EFFECTORMOVETO_H__61F18B52_A5E3_4375_B24C_A297C73DFF29__INCLUDED_)
#define AFX_EFFECTORMOVETO_H__61F18B52_A5E3_4375_B24C_A297C73DFF29__INCLUDED_

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
	virtual void Configure( const NDb::SUIStateBase *pCmd, interface IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, interface IScreen *pScreen, const bool bFastForward );
	virtual void Visit( interface IUIVisitor *pVisitor ) { }
	virtual void Reverse();
};
#endif // !defined(AFX_EFFECTORMOVETO_H__61F18B52_A5E3_4375_B24C_A297C73DFF29__INCLUDED_)
