#pragma once
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"

struct IScene;

namespace NDb
{
struct SComplexSoundDesc;
}

// play sound. may wait for sound to finish.
class CEffectorPlaySound : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorPlaySound )
	CDBPtr<NDb::SComplexSoundDesc> pSound;
	bool bForward;
	bool bFinished;

public:
	CEffectorPlaySound()
		: bFinished( false )
	{
	}

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
