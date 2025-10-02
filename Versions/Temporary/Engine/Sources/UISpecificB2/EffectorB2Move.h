#pragma once

#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../UI/UI.h"

struct SWindowContextB2Move : public SWindowContext
{
	OBJECT_BASIC_METHODS( SWindowContextB2Move );
public:
	ZDATA_(SWindowContext)
	bool bGoOut;
	float fMaxMoveTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SWindowContext*)this); f.Add(2,&bGoOut); f.Add(3,&fMaxMoveTime); return 0; }
	
	SWindowContextB2Move() : bGoOut( true ), fMaxMoveTime( 0.0f ) {}
};

class CEffectorB2Move : public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorB2Move)

	enum EState
	{
		ES_NONE,
		ES_GO_OUT,
		ES_GO_IN,
		ES_BOUNCE_OUT,
		ES_BOUNCE_IN,
		ES_BORDER_OUT,
		ES_BORDER_IN,
	};
	
	ZDATA
	CPtr<IWindow> pElement;
	bool bFinished;
	bool bForward;

	float fElapsedTime;											// time elapsed so far

	CVec2 vMoveOffset;
	float fMoveTime;												// points per second

	CVec2 vSpeed;														// speed
	CVec2 vAccel;														// accel
	
	CVec2 vMoveOffset2;
	float fMoveTime2;												// points per second

	CVec2 vSpeedBounceOut;														// speed
	CVec2 vAccelBounceOut;														// accel
	
	CVec2 vSpeedBounceIn;														// speed
	CVec2 vAccelBounceIn;														// accel
	
	CVec2 vInitialPos;
	CVec2 vOuterPos;
	CVec2 vBouncePos;
	
	EState eState;
	
	CVec2 vInitialSize;
	CVec2 vBounceSize;
	
	float fWaitTime;
	bool bGoOut;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pElement); f.Add(3,&bFinished); f.Add(4,&bForward); f.Add(5,&fElapsedTime); f.Add(6,&vMoveOffset); f.Add(7,&fMoveTime); f.Add(8,&vSpeed); f.Add(9,&vAccel); f.Add(10,&vMoveOffset2); f.Add(11,&fMoveTime2); f.Add(12,&vSpeedBounceOut); f.Add(13,&vAccelBounceOut); f.Add(14,&vSpeedBounceIn); f.Add(15,&vAccelBounceIn); f.Add(16,&vInitialPos); f.Add(17,&vOuterPos); f.Add(18,&vBouncePos); f.Add(19,&eState); f.Add(20,&vInitialSize); f.Add(21,&vBounceSize); f.Add(22,&fWaitTime); f.Add(23,&bGoOut); return 0; }
private:
	CVec2 GetDelta( const CVec2 &vSpeed, const CVec2 &vAccel, float fTime ) const;
	void CalcSpeedAccel( CVec2 *pSpeed, CVec2 *pAccel, const CVec2 &vMoveOffset, const CVec2 &vAccelCoeff, float fTime ) const;
public:
	CEffectorB2Move() : 
		bFinished( false ), bForward( false ), fElapsedTime( 0.0f ),
		fMoveTime( 0.0f ), fMoveTime2( 0.0f ), fWaitTime( 0.0f ), bGoOut( 0.0f )	
	{ }

	virtual bool IsFinished() const { return bFinished; }
	virtual void Configure( const NDb::SUIStateBase *pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse();
};


