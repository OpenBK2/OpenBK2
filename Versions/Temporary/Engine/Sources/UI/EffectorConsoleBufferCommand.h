#pragma once

class CEffectorConsoleBufferCommand : public IUIEffector
{
	OBJECT_BASIC_METHODS( CEffectorConsoleBufferCommand )
	bool bFinished;
	std::string szEditBoxName;

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

	void Reverse() override {}
};
