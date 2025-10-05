#pragma once

struct IProgressHook : public CObjectBase
{
	// set number of steps in current subrange
	virtual void SetNumSteps( const int nNumSteps ) = 0;
	// lock subrange (crom current position to nLength) for subsequent SetNumSteps() call
	// !-----------!-----------!-----------!-----------! <-- lock a subrange from the whole range
	//             !---!---!---!---!---!---!             <-- lock a subrange from the current subrange
	//                     !-----!-----!                 <-- steps will be in the current sub-subrange
	virtual void LockRange( const int nLength ) = 0;
	// unlock range
	virtual void UnlockRange() = 0;
	// do one step in progress
	virtual void Step() = 0;
	// direct set a step in the current subrange
	virtual void SetCurrentStep( int nCurrentStep ) = 0;
};

namespace NProgressHook
{
	inline void SetNumSteps( IProgressHook *pProgressHook, const int nNumSteps )
	{
		if ( pProgressHook ) pProgressHook->SetNumSteps( nNumSteps );
	}

	inline void LockRange( IProgressHook *pProgressHook, const int nLength )
	{
		if ( pProgressHook ) pProgressHook->LockRange( nLength );
	}

	inline void UnlockRange( IProgressHook *pProgressHook )
	{
		if ( pProgressHook ) pProgressHook->UnlockRange();
	}

	inline void Step( IProgressHook *pProgressHook )
	{
		if ( pProgressHook ) pProgressHook->Step();
	}

	void DebugLock( const std::string &szFileName, const int nLine );
	void DebugUnLock( const std::string &szFileName, const int nLine );
	
	inline void LockRange( IProgressHook *pProgressHook, const int nLength, const std::string &szFileName, const int nLine )
	{
		if ( pProgressHook ) pProgressHook->LockRange( nLength );
		DebugLock( szFileName, nLine );
	}

	inline void UnlockRange( IProgressHook *pProgressHook, const std::string &szFileName, const int nLine )
	{
		if ( pProgressHook ) pProgressHook->UnlockRange();
		DebugUnLock( szFileName, nLine );
	}
}

#ifndef _FINALRELEASE
#define LOCK_RANGE( pProgressHook, nLength ) NProgressHook::LockRange( pProgressHook, nLength, __FILE__, __LINE__ );
#define UNLOCK_RANGE( pProgressHook ) NProgressHook::UnlockRange( pProgressHook, __FILE__, __LINE__ );
#else
#define LOCK_RANGE( pProgressHook, nLength ) NProgressHook::LockRange( pProgressHook, nLength );
#define UNLOCK_RANGE( pProgressHook ) NProgressHook::UnlockRange( pProgressHook );
#endif

