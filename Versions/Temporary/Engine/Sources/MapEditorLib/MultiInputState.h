#pragma once

#include "Interface_InputState.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CMultiInputState : public IInputState
{
	typedef std::vector<IInputState *> CInputStateList;
	CInputStateList inputStateList;
	int nActiveInputState;

public:
	//IInputState interface
	void Enter() override;
	void Leave() override;
	//
	void Draw( IPaintContext *pPaintDC ) override;
	void PostDraw( IPaintContext *pPaintDC ) override;
	//
	void OnSetFocus( IWidget *pNewWnd ) override;
	void OnKillFocus( IWidget *pOldWnd ) override;
	//
	void OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	bool OnMouseWheel( unsigned nFlags, short zDelta, CTPoint<int> &rMousePoint ) override;
	//
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	//
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	//
	void OnMButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	void OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rMousePoint ) override;
	//
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
	void OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags ) override;
	///
	void OnContextMenu( const CTPoint<int> &rMousePoint ) override;

public:
	CMultiInputState()
		: nActiveInputState( INVALID_INPUT_STATE_INDEX )
	{
	}

	~CMultiInputState() override
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
	int GetCount()
	{
		return inputStateList.size();
	}

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
	template <class TINPUTSTATE>
	int AddInputState( TINPUTSTATE *pDummyInputState )
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
	IInputState *GetActiveInputState()
	{
		if ( ( nActiveInputState >= 0 ) && ( nActiveInputState < inputStateList.size() ) )
		{
			return inputStateList[nActiveInputState];
		}
		else
		{
			return 0;
		}
	}

	//
	IInputState *GetInputState( int nInputStateIndex )
	{
		if ( ( nInputStateIndex >= 0 ) && ( nInputStateIndex < inputStateList.size() ) )
		{
			return inputStateList[nInputStateIndex];
		}
		else
		{
			return 0;
		}
	}
};
