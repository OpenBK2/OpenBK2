#pragma once
#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "UI/UI.h"


struct IScene;
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
	virtual void Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow );
	virtual const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward );
	virtual void Visit( struct IUIVisitor *pVisitor ) { }
	virtual void Reverse();
	int operator&( IBinSaver &saver );
};


