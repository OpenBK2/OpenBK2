#pragma once

#include "SceneB2_export.h"

#include "Camera.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/DG.h"
#include "Main/GameTimer.h"

class CScriptMovieMutator : public NCamera::CCameraPlacement
{
	OBJECT_NOCOPY_METHODS( CScriptMovieMutator )
	//
	ZDATA
protected:
	NCamera::CCameraPlacement placementStart;
	NCamera::CCameraPlacement placementFinish;
	NTimer::STime timeStart;
	NTimer::STime timeFinish;
	//NTimer::STime timeCurrent;
	CDGPtr< CFuncBase<STime> > pTimer;
	bool bSmoothEnd;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&placementStart); f.Add(3,&placementFinish); f.Add(4,&timeStart); f.Add(5,&timeFinish); f.Add(6,&pTimer); f.Add(7,&bSmoothEnd); return 0; }

protected:
	bool NeedUpdate() { return pTimer != 0 ? pTimer.Refresh() : false; }

public:
	CScriptMovieMutator() { }

	bool IsPaused() { return false; }
};


// just direct flight for some time
class CSCamDMoveFlightMutator : public CScriptMovieMutator
{
	OBJECT_NOCOPY_METHODS( CSCamDMoveFlightMutator )
	//
	ZDATA
	CVec3 vAnchorStart, vAnchorDir;
	float fLengStart, fLengFinish;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vAnchorStart); f.Add(3,&vAnchorDir); f.Add(4,&fLengStart); f.Add(5,&fLengFinish); return 0; }
	
protected:
	void Recalc();

public:
	CSCamDMoveFlightMutator() {}
	CSCamDMoveFlightMutator( const NCamera::CCameraPlacement &_start, const NCamera::CCameraPlacement &_finish,
													 NTimer::STime _timeStart, const float _timeLength, CFuncBase<STime> *_pTimer, bool _bSmoothEnd = true )
	{
		timeStart = _timeStart;
		timeFinish = _timeStart + _timeLength;
		pTimer = _pTimer;
		bSmoothEnd = _bSmoothEnd;

		placementStart = _start;
		placementFinish = _finish;

		const float fStartYaw = ToRadian( placementStart.fYaw );
		const float fStartPitch = ToRadian( placementStart.fPitch );
		const CVec3 vStartDir( sin(-fStartYaw)*cos(-fStartPitch),
													 cos(-fStartYaw)*cos(-fStartPitch), 
													 sin(-fStartPitch) );
		fLengStart = fabs( placementStart.vPosition.z / vStartDir.z );
		vAnchorStart = placementStart.vPosition + vStartDir * fLengStart;
		//
		const float fFinishYaw = ToRadian( placementFinish.fYaw );
		const float fFinishPitch = ToRadian( placementFinish.fPitch );
		const CVec3 vFinishDir( sin(-fFinishYaw)*cos(-fFinishPitch),
														cos(-fFinishYaw)*cos(-fFinishPitch), 
														sin(-fFinishPitch) );
		fLengFinish = fabs( placementFinish.vPosition.z / vFinishDir.z );
		const CVec3 vFinishAnchor = placementFinish.vPosition + vFinishDir * fLengFinish;
		vAnchorDir = vFinishAnchor - vAnchorStart;
	}
};


// direct flight with following target object 
class CSCamDFollowFlightMutator : public CScriptMovieMutator
{
	OBJECT_NOCOPY_METHODS( CSCamDFollowFlightMutator )
	//
	ZDATA
	CDGPtr<NCamera::CCameraPlacement> pBaseFlight;
	int nTargetID;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseFlight); f.Add(3,&nTargetID); return 0; }
	
protected:
	void Recalc();

public:
	CSCamDFollowFlightMutator() {}
	CSCamDFollowFlightMutator( const NCamera::CCameraPlacement &_start, const NCamera::CCameraPlacement &_finish,
														 NTimer::STime _timeStart, NTimer::STime _timeLength, int _nTargetID,
														 CFuncBase<STime> *_pTimer, bool _bSmoothEnd = true )
	{
		timeStart = _timeStart;
		timeFinish = _timeStart + _timeLength;
		pTimer = _pTimer;
		bSmoothEnd = _bSmoothEnd;

		placementStart = _start;
		placementFinish = _finish;

		pBaseFlight = new CSCamDMoveFlightMutator( _start,	_finish, _timeStart, _timeLength,	_pTimer, _bSmoothEnd );
	}
};


// direct flight for some time with rotation 
class CSCamDRotateFlightMutator : public CScriptMovieMutator
{
	OBJECT_NOCOPY_METHODS( CSCamDRotateFlightMutator )
	//
	ZDATA
	CDGPtr<NCamera::CCameraPlacement> pBaseFlight;
	float fAngle, fStartYaw;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseFlight); f.Add(3,&fAngle); f.Add(4,&fStartYaw); return 0; }

protected:
	void Recalc();

public:
	CSCamDRotateFlightMutator() {}
	CSCamDRotateFlightMutator( const NCamera::CCameraPlacement &_start, const NCamera::CCameraPlacement &_finish,
														 NTimer::STime _timeStart, NTimer::STime _timeLength, float _fAngle,
														 CFuncBase<STime> *_pTimer, bool _bSmoothEnd = true )
	{
		timeStart = _timeStart;
		timeFinish = _timeStart + _timeLength;
		pTimer = _pTimer;
		bSmoothEnd = _bSmoothEnd;

		fAngle = ToRadian(_fAngle );
		fStartYaw = ToRadian( _start.fYaw );

		pBaseFlight = new CSCamDMoveFlightMutator( _start, _finish, _timeStart, _timeLength, _pTimer, _bSmoothEnd );
	}
};


class CSCamSplineMutator : public CScriptMovieMutator
{
	OBJECT_NOCOPY_METHODS( CSCamSplineMutator )
	//
	ZDATA
	CVec3 vAnchorStart, vAnchorFinish, vAnchorDir;
	CVec3 vStartDir, vFinishDir;
	float fLengStart, fLengFinish;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vAnchorStart); f.Add(3,&vAnchorFinish); f.Add(4,&vAnchorDir); f.Add(5,&vStartDir); f.Add(6,&vFinishDir); f.Add(7,&fLengStart); f.Add(8,&fLengFinish); return 0; }
	
protected:
	void Recalc();

public:
	CSCamSplineMutator() {}
	CSCamSplineMutator(	const NCamera::CCameraPlacement &_start, const NCamera::CCameraPlacement &_finish,
											NTimer::STime _timeStart, NTimer::STime _timeLength, float fSpline1, float fSpline2,
											CFuncBase<STime> *_pTimer, bool _bSmoothEnd = true )
	{
		timeStart = _timeStart;
		timeFinish = _timeStart + _timeLength;
		pTimer = _pTimer;
		bSmoothEnd = _bSmoothEnd;

		placementStart = _start;
		placementFinish = _finish;
	
		const float fStartYaw = ToRadian( placementStart.fYaw );
		const float fStartPitch = ToRadian( placementStart.fPitch );
		vStartDir = CVec3(	sin(-fStartYaw)*cos(-fStartPitch),
												cos(-fStartYaw)*cos(-fStartPitch), 
												sin(-fStartPitch) );
		fLengStart = fabs( placementStart.vPosition.z / vStartDir.z );
		vStartDir *= fLengStart * fSpline1;
		vAnchorStart = placementStart.vPosition + vStartDir;
		//
		const float fFinishYaw = ToRadian( placementFinish.fYaw );
		const float fFinishPitch = ToRadian( placementFinish.fPitch );
		vFinishDir = CVec3( sin(-fFinishYaw)*cos(-fFinishPitch),
												cos(-fFinishYaw)*cos(-fFinishPitch), 
												sin(-fFinishPitch) );
		fLengFinish = fabs( placementFinish.vPosition.z / vFinishDir.z );
		vFinishDir *= fLengFinish * fSpline2;
		vAnchorFinish = placementFinish.vPosition + vFinishDir;

		vAnchorDir = vAnchorFinish - vAnchorStart;
	}
};


//
class SCENEB2_EXPORT CScriptMoviesMutatorHolder : public CVersioningBase
{
	OBJECT_NOCOPY_METHODS( CScriptMoviesMutatorHolder )

	enum EPlaybackMode
	{
		PM_PLAYING,
		PM_PAUSED,
		PM_STOPPED,
		PM_FINISHED
	};

private:
	ZDATA
	NDb::SScriptMovies moviesData;
	int nMovieIndex;

	NTimer::STime movieStartTime;
	float fCurrTime;
	CVec3 vAnchorStart, vAnchorDir;
	float fLengStart, fLengFinish;

	bool bLoopPlayback;
	float fSpeedCoeff;
	CDGPtr< CFuncBase<STime> > pTimer;
	NCamera::CCameraPlacement placement;
	vector<CQuat> cameraQuats;
	vector<CVec3> cameraAnchors;
	vector<float> cameraDists;

	EPlaybackMode eMode;
	int nLastKeyID;
	string szCallbackFuncName;

public:
	ZEND
	int operator&( IBinSaver &f );

private:
	void ScriptCallback( int nKeyID );

public:
	CScriptMoviesMutatorHolder() {}
	CScriptMoviesMutatorHolder( const NDb::SScriptMovies &rMoviesData, int _nMovieIndex, CFuncBase<STime> *_pTimer );

	void Recalc();
	bool NeedUpdate() { return pTimer != 0 ? pTimer.Refresh() : false; }

	// controls
	void JumpFirstKey();
	void JumpLastKey();
	void StepNextKey();
	void StepPrevKey();

	void Play() { eMode = PM_PLAYING; }
	void Pause() { eMode = PM_PAUSED; }
	void Stop();

	bool IsPlaying() const { return (eMode == PM_PLAYING); }
	bool IsPaused() const { return (eMode == PM_PAUSED); }
	bool IsStopped() const { return (eMode == PM_STOPPED); }
	bool IsFinished() const { return (eMode == PM_FINISHED); }

	void SetSpeed( float _fSpeed ) { fSpeedCoeff = _fSpeed; }
	void SetLoopMode( bool _bLoopPlayback ) { bLoopPlayback = _bLoopPlayback; }
	void SetTime( float _fTime );

	const NCamera::CCameraPlacement GetValue() const;
	float GetTime() const { return fCurrTime; }
	float GetLength() const;
	void SetCallbackFuncName( const string &_szCallbackFuncName );

	const CVec3 GetAnchor() const;
	const CVec3 GetPos() const;

	const float GetDistance() const;
	const float GetYaw() const;
	const float GetPitch() const;
	const float GetFOV() const;

	void GetPlacement( float *pfDist, float *pfPitch, float *pfYaw ) const;
};



