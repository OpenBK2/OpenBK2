// EffectorMoveTo.h: interface for the CEffectorMoveTo class.
//

#pragma once

class CEffectorMoveTo : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorMoveTo )

	CPtr<CWindow> pElement;
	bool bFinished;
	CVec2 vMoveOffset;
	float fMoveTime;    // points per second
	CVec2 vSpeed;       // speed
	float fElapsedTime; // time elapsed so far

	CVec2 vMoveFrom;
	bool bForward;

	const std::pair<CVec2, int> GetCur() const;

public:
	int operator&( IBinSaver &ss ) override;

	bool IsFinished() const override
	{
		return bFinished;
	}

	void Configure( const NDb::SUIStateBase *pCmd,
	                struct IScreen *pScreen,
	                SWindowContext *pContext,
	                const std::string &szAnimatedWindow ) override;
	const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward ) override;

	void Visit( struct IUIVisitor *pVisitor ) override {}

	void Reverse() override;
};
