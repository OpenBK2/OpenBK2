#pragma once



// UIScreen recieve command sequience, produce states sequience
// and track execution of states. when Effect is finished or
// no effect was produced by command, state sequience moves to next state

struct SUIState
{
	CDBPtr<NDb::SUIStateBase> pCmd;										// cmd that creates this state
	CObj<IUIEffector> pEffect;							// effect that was created by the command
	// null if no effect.
	CObj<CWindow> pCommandParent;						// window that must be notified after 
	SUIState() {  }
	SUIState( const NDb::SUIStateBase * _pCmd ) : pCmd( _pCmd ) {  }
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pCmd );
		saver.Add( 2, &pEffect );
		saver.Add( 3, &pCommandParent );
		return 0;
	}
};

class CStates 
{
	std::vector<SUIState> states;
	int nCurIndex;												// currently running effect
	bool bForward;												// effect direction 
	bool bEnd;														// all effects are finished
	CPtr<CWindow> pNotifySink;						// window that must be notified after
	CPtr<SWindowContext> pContext;
	CPtr<SWindowAnimationContext> pAnimationContext;
	bool bReversable;												// effect can be undone
	std::string szCmdName;
	std::pair<int, CPtr<CWindow> > id;
	WORD wKeyboardFlags;

	void CheckEnd();
	IUIEffector *CreateEffect( const NDb::SUIStateBase *pCmd, class CWindowScreen *pScreen, SWindowContext *pContext );
	SUIState &GetCur() { return states[nCurIndex]; }
	void Advance();
	void NotifyParent();
	void Reserve( const int nSize ) { states.reserve( nSize ); }
	void Add( const NDb::SUIStateBase * pCmd );
	void Play( const int timeDiff, class CWindowScreen *pScreen, const bool bFastForward );

public:
	CStates() : nCurIndex( 0 ), bForward( true ), bEnd( true ), bReversable( false ) {  }

	CStates( const NDb::SUIStateSequence &seq, 
					 const std::string &_szCmdName,
					 const bool _bReversable, 
					 WORD wFlags );

	void FastForward( const int timeDiff, class CWindowScreen *pScreen );
	const std::string &GetName() const { return szCmdName; }
	void SetNotifySink( class CWindow *pWindow ) { pNotifySink = pWindow; }
	void SetContext( SWindowContext *_pContext ) { pContext = _pContext; }
	
	// if this states are animation states - then they will need id.
	void SetAnimatedWindow( const int nID, class CWindow * pWindow );
	const std::pair<int, CPtr<CWindow> > GetID() const { return id; }

	// effect can be deleted already, all work is done
	const bool IsToBeDeleted() const;
	const bool IsEnd() const { return bEnd; }
	// run effects in reverse direction
	void Reverse();
	void Segment( const int timeDiff, class CWindowScreen *pScreen );
	bool IsReversable() const { return bReversable; }
	const bool IsForward() const { return bForward; }
	int operator&( IBinSaver &saver );
};


