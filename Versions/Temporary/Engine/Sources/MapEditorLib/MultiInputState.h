#if !defined(__MULTI_INPUT_STATE__)
#define __MULTI_INPUT_STATE__
#pragma once

#include "Interface_InputState.h"

class CMultiInputState : public IInputState
{
	typedef vector<IInputState*> CInputStateList;
	CInputStateList inputStateList;	
	int nActiveInputState;

public:
	//IInputState interface
	virtual void Enter();
	virtual void Leave();
	//
	virtual void Draw( class CPaintDC *pPaintDC );
	virtual void PostDraw( class CPaintDC *pPaintDC );
	//
	virtual void OnSetFocus				( class CWnd* pNewWnd );
	virtual void OnKillFocus			( class CWnd* pOldWnd );
	//
	virtual void OnMouseMove			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual bool OnMouseWheel			( UINT nFlags, short zDelta, CTPoint<int> &rMousePoint );
	//
	virtual void OnLButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnLButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnLButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	virtual void OnRButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnRButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnRButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	virtual void OnMButtonDown		( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnMButtonUp			( UINT nFlags, const CTPoint<int> &rMousePoint );
	virtual void OnMButtonDblClk	( UINT nFlags, const CTPoint<int> &rMousePoint );
	//
	virtual void OnKeyDown				( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnKeyUp					( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnChar						( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnSysKeyDown			( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnSysKeyUp				( UINT nChar, UINT nRepCnt, UINT nFlags );
	virtual void OnSysChar				( UINT nChar, UINT nRepCnt, UINT nFlags );
	///
	virtual void OnContextMenu( const CTPoint<int> &rMousePoint );

public:
	CMultiInputState() : nActiveInputState( INVALID_INPUT_STATE_INDEX ) {}
	~CMultiInputState()
	{
		for ( int nInputStateIndex = 0; nInputStateIndex < inputStateList.size(); ++nInputStateIndex )
		{
			if ( inputStateList[nInputStateIndex] )
			{
				delete ( inputStateList[nInputStateIndex] );
				inputStateList[nInputStateIndex] = 0;
			}
		}
	}
	//	
	int GetCount() { return inputStateList.size(); }
	//
	int SetActiveInputState( int nNewInputState, bool bSwitchStates, bool bForceSwitch )
	{
		if ( bSwitchStates )
		{
			if ( ( nActiveInputState != nNewInputState ) || bForceSwitch )
			{
				if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
				{
					inputStateList[nActiveInputState]->Leave();
				}
			}
		}
		const int nOldInputState = nActiveInputState;
		nActiveInputState = nNewInputState;
		if ( bSwitchStates )
		{
			if ( ( nOldInputState != nActiveInputState ) || bForceSwitch )
			{
				if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
				{
					inputStateList[nActiveInputState]->Enter();
				}
			}
		}
		return nOldInputState;
	}
	//	
	template<class TINPUTSTATE>
	int AddInputState( TINPUTSTATE* pDummyInputState ) 
	{
		TINPUTSTATE *pNewInputState = pDummyInputState;
		if ( !pNewInputState )
		{
			pNewInputState = new TINPUTSTATE();
		}
		inputStateList.push_back( pNewInputState );
		//SetActiveInputState( inputStateList.size() - 1, true, false );
		return inputStateList.size() - 1;
	}
	
	//
	int RemoveInputState( int nInputStateIndex ) 
	{
		if ( ( nInputStateIndex >= 0 ) && ( nInputStateIndex < inputStateList.size() ) )
		{
			if ( nActiveInputState > nInputStateIndex )
			{
				--nActiveInputState;
			}
			else if ( nActiveInputState == nInputStateIndex )
			{
				inputStateList[nActiveInputState]->Leave();
				nActiveInputState = INVALID_INPUT_STATE_INDEX;
			}
			delete ( inputStateList[nInputStateIndex] );
			inputStateList[nInputStateIndex] = 0;
			inputStateList.erase( inputStateList.begin() + nInputStateIndex );
		}
		return nActiveInputState;
	}
	//
	int GetActiveInputStateIndex()
	{
		return nActiveInputState;
	}
	//
	IInputState* GetActiveInputState()
	{
		if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
			return inputStateList[nActiveInputState];
		else
			return 0;
	}
	//
	IInputState* GetInputState( int nInputStateIndex )
	{
		if ( ( nInputStateIndex >= 0 ) && ( nInputStateIndex < inputStateList.size() ) )
			return inputStateList[nInputStateIndex];
		else
			return 0;
	}
};

#endif // !defined(__MULTI_INPUT_STATE__)
