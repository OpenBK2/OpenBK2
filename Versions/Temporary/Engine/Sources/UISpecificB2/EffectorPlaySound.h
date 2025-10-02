#pragma once
#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "../UI/UI.h"


interface IScene;
namespace NDb
{
	struct SComplexSoundDesc;
}

// play sound. may wait for sound to finish.
class CEffectorPlaySound :	public IUIEffector
{
	OBJECT_BASIC_METHODS(CEffectorPlaySound)
	CDBPtr<NDb::SComplexSoundDesc> pSound;
	bool bForward;
	bool bFinished;
public:
	CEffectorPlaySound() : bFinished( false ) {  }
	virtual bool IsFinished() const;
	virtual void Configure( const NDb::SUIStateBase *_pCmd, interface IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, interface IScreen *pScreen, const bool bFastForward );
	virtual void Visit( interface IUIVisitor *pVisitor ) { }
	virtual void Reverse();
	int operator&( IBinSaver &saver );
};


