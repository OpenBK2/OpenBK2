#pragma once

// run message reaction
class CEffectorDirectRunReaction : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorDirectRunReaction )
	ZDATA
	CDBPtr<NDb::SUIDesc> pReactionBackward;
	CDBPtr<NDb::SUIDesc> pReactionForward;
	bool bFinished;
	bool bForward;
	std::string szAnimatedWindow;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pReactionBackward );
		f.Add( 3, &pReactionForward );
		f.Add( 4, &bFinished );
		f.Add( 5, &bForward );
		f.Add( 6, &szAnimatedWindow );
		return 0;
	}

public:
	CEffectorDirectRunReaction() {}

	bool IsFinished() const override;
	void Configure( const NDb::SUIStateBase *_pCmd,
	                struct IScreen *pScreen,
	                SWindowContext *pContext,
	                const std::string &szAnimatedWindow ) override;
	const int Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward ) override;

	void Visit( struct IUIVisitor *pVisitor ) override {}

	void Reverse() override;
};
