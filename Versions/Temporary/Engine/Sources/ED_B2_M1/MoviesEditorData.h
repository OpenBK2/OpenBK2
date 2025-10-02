#pragma once

#include "Stats_B2_M1/DBMapInfo.h"
#include "Misc/BitData.h"

struct SScriptMovieEditorData
{
	enum EMoviesEditorLastAction
	{
		ME_UNKNOWN,
		ME_NO_ACTIONS,
		ME_JUMP_FIRST_KEY,
		ME_JUMP_LAST_KEY,
		ME_STEP_PREV_KEY,
		ME_STEP_NEXT_KEY,
		ME_PLAY,
		ME_PAUSE,
		ME_STOP,
		ME_ADD_SEQ,
		ME_DEL_SEQ,
		ME_MOVIE_SWITCH,
		ME_CHANGE_TIME,
		ME_CHANGE_SPEED,
		ME_INSERT_KEY,
		ME_SAVE_KEY,
		ME_KEY_SETTINGS,
		ME_DELETE_KEYS,
		ME_MOVE_KEYS,
		ME_RESIZE,
		ME_CLEAR_MARKERS,
		ME_DRAW_MARKERS,
		ME_SELECT_CAMERA
	};
	EMoviesEditorLastAction eLastAction;

	float fTimeLinePercentage;

	float fCursorTime;
	bool bIsPlaying;
	int nActiveMovie;
	CArray1Bit activeKeysList;

	NDb::SScriptMovies scriptMoviesData;

	int nSpeed;

	float fNewLength;
	float fMoveValue;

	SScriptMovieEditorData()
	{
		Clear();
	}
	//
	void Clear()
	{
		eLastAction = ME_NO_ACTIONS;
		bIsPlaying = false;
		nActiveMovie = -1;
		fTimeLinePercentage = 0.0f;

		fCursorTime = 0.0f;

		nSpeed = 0;

		fNewLength = 0.0f;
		fMoveValue = 0.0f;

		activeKeysList.SetSize( 0 );
	}
	//
	void Reset()
	{
		eLastAction = ME_NO_ACTIONS;
	}
	//
	void SetLastAction( EMoviesEditorLastAction eAction )
	{
		eLastAction = eAction;
	}
};



