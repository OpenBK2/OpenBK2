#include "stdafx.h"

#include "../3Dmotor/Interpolate.h"
#include "../SceneB2/Scene.h"
#include "CameraScriptMutators.h"

void SLERPAngles( float *pfYaw, float *pfPitch, float *pfRoll, 
									const float fYaw1, const float fPitch1, const float fRoll1,
									const float fYaw2, const float fPitch2, const float fRoll2, 
									const float fdt )
{
	CQuat q1, q2, qRes;
	q1.FromEulerAngles( fYaw1, fPitch1, fRoll1 );
	q2.FromEulerAngles( fYaw2, fPitch2, fRoll2 );
	qRes.Slerp( fdt, q1, q2 );
	qRes.DecompEulerAngles( pfYaw, pfPitch, pfRoll );
}


void CSCamDMoveFlightMutator::Recalc()
{
	static bool bIsFinishing = false;
	if ( pTimer == 0 )
		return;

	NTimer::STime currTime = pTimer->GetValue();
	if ( IsPaused() )
		currTime = timeStart;

	if ( currTime <= timeStart )
	{
		this->vPosition = placementStart.vPosition;
		this->fYaw = placementStart.fYaw;
		this->fPitch = placementStart.fPitch;
		this->fFOV = placementStart.fFOV;
	}
	else if ( currTime >= timeFinish )
	{
		this->vPosition = placementFinish.vPosition;
		this->fYaw = placementFinish.fYaw;
		this->fPitch = placementFinish.fPitch;
		this->fFOV = placementFinish.fFOV;
		if ( !bSmoothEnd )
			Camera()->FinishMovie( false );
	}
	else
	{
		const float fCoeff = float( currTime - timeStart ) / float( timeFinish - timeStart );
		const CVec3 vAnchor = vAnchorStart + vAnchorDir*fCoeff;
		//
		{
			float fYaw, fPitch, fRoll;
			SLERPAngles( &fYaw, &fPitch, &fRoll,
									 ToRadian(placementStart.fYaw), ToRadian(placementStart.fPitch), 0, 
									 ToRadian(placementFinish.fYaw), ToRadian(placementFinish.fPitch), 0, 
									 fCoeff );
			this->fYaw = ToDegree( fYaw );
			this->fPitch = ToDegree( fPitch );
		}
		TLinearInterpolate interp;
		//
		const float fLeng = interp( fLengStart, fLengFinish, fCoeff );
		const CVec3 vDir( sin(ToRadian(-this->fYaw))*cos(ToRadian(-this->fPitch)),
											cos(ToRadian(-this->fYaw))*cos(ToRadian(-this->fPitch)), 
											sin(ToRadian(-this->fPitch)) );
		this->vPosition = vAnchor - vDir*fLeng;
		this->fFOV = interp( placementStart.fFOV, placementFinish.fFOV, fCoeff );
	}

	// convert to game's units
	this->fPitch = ToRadian( 180.0f ) + ToRadian( 90.0f - this->fPitch );
	this->fYaw = ToRadian( this->fYaw );
	AI2Vis( &this->vPosition );
};


void CSCamDFollowFlightMutator::Recalc()
{
	if ( pTimer == 0 || pBaseFlight == 0 )
		return;

	NTimer::STime currTime = pTimer->GetValue();
	if ( IsPaused() )
		currTime = timeStart;

	if ( currTime < timeStart )
	{
		this->vPosition = placementStart.vPosition;
		this->fYaw = placementStart.fYaw;
		this->fPitch = placementStart.fPitch;
		this->fFOV = placementStart.fFOV;
	}
	else if ( currTime <= timeFinish )
	{
		const NCamera::CCameraPlacement &basePlacement = pBaseFlight->GetValue();

		this->vPosition = basePlacement.vPosition;
		Vis2AI( &this->vPosition );
		this->fFOV = basePlacement.fFOV;
		//
		SFBTransform transform;
		Scene()->GetVisObjPlacement( nTargetID, &transform );
		CVec3 vAnchor = transform.forward.GetTrans3();
		Vis2AI( &vAnchor );
		CVec3 vDir = vAnchor - this->vPosition;
		Normalize( &vDir );
		//
		this->fPitch = ToDegree( -asinf(vDir.z) );
		this->fYaw = ToDegree( atan2(vDir.y, vDir.x) - FP_PI2 );
	}
	else
	{
		if ( !bSmoothEnd )
			Camera()->FinishMovie( false );
		return;
	}

	// convert to game's units
	this->fPitch = ToRadian( 180.0f ) + ToRadian( 90.0f - this->fPitch );
	this->fYaw = ToRadian( this->fYaw );
	AI2Vis( &this->vPosition );
};


void CSCamDRotateFlightMutator::Recalc()
{
	if ( pTimer == 0 || pBaseFlight == 0 )
		return;

	const NCamera::CCameraPlacement &basePlacement = pBaseFlight->GetValue();

	NTimer::STime currTime = pTimer->GetValue();
	if ( IsPaused() )
		currTime = timeStart;

	if ( currTime <= timeStart )
		this->fYaw = basePlacement.fYaw;
	else if ( currTime > timeFinish )
	{
		currTime = timeFinish;
		if ( !bSmoothEnd )
			Camera()->FinishMovie( false );
	}

	this->vPosition = basePlacement.vPosition;
	this->fPitch = basePlacement.fPitch;
	this->fFOV = basePlacement.fFOV;
	this->fYaw = basePlacement.fYaw;
	// convert to editor's units
	this->fPitch = 90.0f - ToDegree( this->fPitch - ToRadian(180.0f) );
	this->fYaw = ToDegree( this->fYaw );
	Vis2AI( &this->vPosition );


	const float fPitch = ToRadian(this->fPitch);
	const float fYaw = ToRadian(this->fYaw);

	CVec3 vOldDir = CVec3(	sin(-fYaw)*cos(-fPitch),
													cos(-fYaw)*cos(-fPitch), 
													sin(-fPitch) );

	if ( vOldDir.z != 0 ) //	We cannot rotate horizontal camera
		vOldDir *= fabs( Vis2AI(basePlacement.vPosition.z) / vOldDir.z );

	const float fCoeff = float( currTime - timeStart ) / float( timeFinish - timeStart );
	const float fNewYaw = fStartYaw + fAngle*fCoeff;
	this->fYaw = ToDegree( fNewYaw );
	CVec3 vNewDir = CVec3(	sin(-fNewYaw)*cos(-fPitch),
													cos(-fNewYaw)*cos(-fPitch), 
													sin(-fPitch) );
	vNewDir *= fabs( vOldDir );

	this->vPosition = ( CVec3(Vis2AI(basePlacement.vPosition.x), Vis2AI(basePlacement.vPosition.y), Vis2AI(basePlacement.vPosition.z)) + vOldDir ) - vNewDir;

	// convert to game's units
	this->fPitch = ToRadian( 180.0f ) + ToRadian( 90.0f - this->fPitch );
	this->fYaw = ToRadian( this->fYaw );
	AI2Vis( &this->vPosition );
};


void CSCamSplineMutator::Recalc()
{
	if ( pTimer == 0 )
		return;

	NTimer::STime currTime = pTimer->GetValue();
	if ( IsPaused() )
		currTime = timeStart;

	if ( currTime <= timeStart )
	{
		this->vPosition = placementStart.vPosition;
		this->fYaw = placementStart.fYaw;
		this->fPitch = placementStart.fPitch;
		this->fFOV = placementStart.fFOV;
	}
	else if ( currTime >= timeFinish )
	{
		this->vPosition = placementFinish.vPosition;
		this->fYaw = placementFinish.fYaw;
		this->fPitch = placementFinish.fPitch;
		this->fFOV = placementFinish.fFOV;
		if ( !bSmoothEnd )
			Camera()->FinishMovie( false );
	}
	else
	{
		const float fCoeff = float( currTime - timeStart ) / float( timeFinish - timeStart );
		//
		{
			float fYaw, fPitch, fRoll;
			SLERPAngles( &fYaw, &fPitch, &fRoll,
									 ToRadian(placementStart.fYaw), ToRadian(placementStart.fPitch), 0, 
									 ToRadian(placementFinish.fYaw), ToRadian(placementFinish.fPitch), 0, 
									 fCoeff );
			this->fYaw = ToDegree( fYaw );
			this->fPitch = ToDegree( fPitch );
		}
		TLinearInterpolate interpLin;

		this->fFOV = interpLin( placementStart.fFOV, placementFinish.fFOV, fCoeff );
		//
		THermitInterpolate interpHerm;
		this->vPosition = interpHerm( fCoeff, placementStart.vPosition, placementFinish.vPosition, vStartDir, vFinishDir );
	}
	// convert to game's units
	this->fPitch = ToRadian( 180.0f ) + ToRadian( 90.0f - this->fPitch );
	this->fYaw = ToRadian( this->fYaw );
	AI2Vis( &this->vPosition );
};


CScriptMoviesMutatorHolder::CScriptMoviesMutatorHolder( const NDb::SScriptMovies &rMoviesData, int _nMovieIndex, CFuncBase<STime> *_pTimer )
	: moviesData( rMoviesData ),
	nMovieIndex( _nMovieIndex ),
	pTimer( _pTimer ),
	fCurrTime( 0.0f ),
	fSpeedCoeff( 1.0f ),
	bLoopPlayback( true ),
	eMode( PM_STOPPED ),
	nLastKeyID( -1 )
{
	pTimer.Refresh();
	movieStartTime = pTimer->GetValue();

	NI_VERIFY( (moviesData.scriptMovieSequences.size() > 0) &&
						 (nMovieIndex >= 0) &&
						 (nMovieIndex < moviesData.scriptMovieSequences.size()), "Invalid Movie", return )

	cameraQuats.resize( 0 );
	for ( vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKey = moviesData.scriptMovieSequences[nMovieIndex].posKeys.begin();
																												itPosKey != moviesData.scriptMovieSequences[nMovieIndex].posKeys.end(); ++itPosKey )
	{
		NI_VERIFY( (itPosKey->nPositionIndex >= 0) &&
							 (itPosKey->nPositionIndex < moviesData.scriptCameraPlacements.size()), "Invalid movie key", return )

		const NDb::SScriptCameraPlacement& camPlacement = moviesData.scriptCameraPlacements[itPosKey->nPositionIndex];

		// calculate Quaternion
		CQuat quat;
		quat.FromEulerAngles( ToRadian(camPlacement.fYaw), ToRadian(camPlacement.fPitch), 0.0f );
		cameraQuats.push_back( quat );

		// calculate Anchor
		const CVec3 vDir( sin(ToRadian(-camPlacement.fYaw))*cos(ToRadian(-camPlacement.fPitch)),
											cos(ToRadian(-camPlacement.fYaw))*cos(ToRadian(-camPlacement.fPitch)), 
											sin(ToRadian(-camPlacement.fPitch)) );
		const float fDist = fabs( camPlacement.vPosition.z / vDir.z );
		const CVec3 vAnchor = camPlacement.vPosition + vDir * fDist;
		cameraAnchors.push_back( vAnchor );
		cameraDists.push_back( fDist );
	}
	szCallbackFuncName = "";
}


void CScriptMoviesMutatorHolder::Recalc()
{
	NI_VERIFY( pTimer, "CScriptMoviesMutatorHolder::Recalc() >> pTimer == 0!", return );

	if ( IsFinished() )
		return;

	pTimer.Refresh();
	NTimer::STime timeCurrent = pTimer->GetValue();

	if ( IsPlaying() )
	{
		fCurrTime = fSpeedCoeff * ((float)(timeCurrent) - movieStartTime)/1000.0f;

		vector<NDb::SScriptMovieKeyPos>::const_iterator itEnd = moviesData.scriptMovieSequences[nMovieIndex].posKeys.end();
		--itEnd;
		const float f1 = itEnd->fStartTime;

		if ( fCurrTime >= f1 )
		// finished movie
		{
			if ( bLoopPlayback )
			{
				fCurrTime = 0.0f;
				movieStartTime = timeCurrent;
			}
			else
			{
				ScriptCallback( -1 );

				fCurrTime = f1;
				eMode = PM_FINISHED;
			}
		}
	}

	const NDb::SScriptMovieSequence &movieKeys = moviesData.scriptMovieSequences[nMovieIndex];

	if ( movieKeys.posKeys.size() >= 2 )
	{
		int nStartPos = 0;
		int nFinishPos = 0;

		vector<NDb::SScriptMovieKeyPos>::const_iterator itKeyPosPrev = movieKeys.posKeys.begin();
		vector<NDb::SScriptMovieKeyPos>::const_iterator itKeyPos = itKeyPosPrev;
		++itKeyPos;
		int i = 0;

		while ( itKeyPos != movieKeys.posKeys.end() )
		{
			// find previous and next nodes
			if ( (itKeyPosPrev->fStartTime <= fCurrTime) && (itKeyPos->fStartTime >= fCurrTime) )
			{
				nStartPos = itKeyPosPrev->nPositionIndex;
				nFinishPos = itKeyPos->nPositionIndex;
				//
				const NDb::SScriptCameraPlacement& startCamera = moviesData.scriptCameraPlacements[nStartPos];
				const NDb::SScriptCameraPlacement& finishCamera = moviesData.scriptCameraPlacements[nFinishPos];

				// fCoeff lies in [0..1]
				const float fCoeff = ( fCurrTime - itKeyPosPrev->fStartTime ) / ( itKeyPos->fStartTime - itKeyPosPrev->fStartTime );
				TLinearInterpolate interp;

				CQuat quat;
				quat.Slerp( fCoeff, cameraQuats[i], cameraQuats[i + 1] );
				float fRoll;
				quat.DecompEulerAngles( &placement.fYaw, &placement.fPitch, &fRoll );
				placement.fPitch = FP_PI2 * 3.0f - placement.fPitch;

				// interpolate position
				{
					const CVec3 vAnchor = cameraAnchors[i] + (cameraAnchors[i + 1] - cameraAnchors[i]) * fCoeff;
					const float fDist = interp( cameraDists[i], cameraDists[i + 1], fCoeff );
					const CVec3 vDir( sin(-(placement.fYaw))*cos(-(FP_PI2 * 3.0f - placement.fPitch)),
														cos(-(placement.fYaw))*cos(-(FP_PI2 * 3.0f - placement.fPitch)), 
														sin(-(FP_PI2 * 3.0f - placement.fPitch)) );
					placement.vPosition = vAnchor - vDir*fDist;
				}
				AI2Vis( &placement.vPosition );

				placement.fFOV = interp( startCamera.fFOV, finishCamera.fFOV, fCoeff );

				if ( i != nLastKeyID )
				{
					ScriptCallback( i );
					nLastKeyID = i;
				}

				break;
			}

			itKeyPosPrev = itKeyPos;
			++itKeyPos;
			++i;
		}
	}
}


const NCamera::CCameraPlacement CScriptMoviesMutatorHolder::GetValue() const
{
	return placement;
}


const CVec3 CScriptMoviesMutatorHolder::GetAnchor() const
{
	NCamera::CCameraPlacement placementTmp;

	placementTmp.fYaw = ToDegree( placement.fYaw );
	placementTmp.fPitch = 270.0f - ToDegree( placement.fPitch );
	placementTmp.vPosition = placement.vPosition;

	CVec3 vAnchor = placementTmp.GetAnchor();

	return vAnchor;
}


const CVec3 CScriptMoviesMutatorHolder::GetPos() const
{
  return placement.vPosition;
}


const float CScriptMoviesMutatorHolder::GetDistance() const
{
	return placement.GetDistance();
}


const float CScriptMoviesMutatorHolder::GetYaw() const
{
	return placement.fYaw;
}


const float CScriptMoviesMutatorHolder::GetPitch() const
{
	return placement.fPitch;
}


const float CScriptMoviesMutatorHolder::GetFOV() const
{
	return placement.fFOV;
}


void CScriptMoviesMutatorHolder::GetPlacement( float *pfDist, float *pfPitch, float *pfYaw ) const
{
	(*pfDist) = AI2Vis( placement.GetDistance() );
	(*pfPitch) = placement.fPitch;
	(*pfYaw) = placement.fYaw;
}


float CScriptMoviesMutatorHolder::GetLength() const
{
	NI_VERIFY( (moviesData.scriptMovieSequences.size() > 0) &&
						 (nMovieIndex >= 0) &&
						 (nMovieIndex < moviesData.scriptMovieSequences.size()), "Invalid Movie", return 0.0f )

	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKeyFirst = moviesData.scriptMovieSequences[nMovieIndex].posKeys.begin();
	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKeyLast = moviesData.scriptMovieSequences[nMovieIndex].posKeys.end();
	--itPosKeyLast;

	return ( itPosKeyLast->fStartTime - itPosKeyFirst->fStartTime );
}


void CScriptMoviesMutatorHolder::SetTime( float _fTime )
{
	fCurrTime = _fTime;
	pTimer.Refresh();
	movieStartTime = pTimer->GetValue() - fCurrTime*1000.0;
}


void CScriptMoviesMutatorHolder::Stop()
{
	ScriptCallback( -1 );
	//
	eMode = PM_STOPPED;
}


void CScriptMoviesMutatorHolder::JumpFirstKey()
{
	NI_VERIFY( (moviesData.scriptMovieSequences.size() > 0) &&
						 (nMovieIndex >= 0) &&
						 (nMovieIndex < moviesData.scriptMovieSequences.size()), "Invalid Movie", return )

	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKeyFirst = moviesData.scriptMovieSequences[nMovieIndex].posKeys.begin();

	SetTime( itPosKeyFirst->fStartTime );
}


void CScriptMoviesMutatorHolder::JumpLastKey()
{
	NI_VERIFY( (moviesData.scriptMovieSequences.size() > 0) &&
						 (nMovieIndex >= 0) &&
						 (nMovieIndex < moviesData.scriptMovieSequences.size()), "Invalid Movie", return )

	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKeyLast = moviesData.scriptMovieSequences[nMovieIndex].posKeys.end();
	--itPosKeyLast;

	SetTime( itPosKeyLast->fStartTime );
}


void CScriptMoviesMutatorHolder::StepNextKey()
{
	NI_VERIFY( (moviesData.scriptMovieSequences.size() > 0) &&
						 (nMovieIndex >= 0) &&
						 (nMovieIndex < moviesData.scriptMovieSequences.size()), "Invalid Movie", return )

	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKeyPrev = moviesData.scriptMovieSequences[nMovieIndex].posKeys.begin();
	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKey = itPosKeyPrev;
	++itPosKey;

	while ( itPosKey != moviesData.scriptMovieSequences[nMovieIndex].posKeys.end() )
	{
		if ( (fCurrTime >= itPosKeyPrev->fStartTime) && (fCurrTime < itPosKey->fStartTime) )
		{
			SetTime( itPosKey->fStartTime );
			return;
		}

		itPosKeyPrev = itPosKey;
		++itPosKey;
	}
}


void CScriptMoviesMutatorHolder::StepPrevKey()
{
	NI_VERIFY( (moviesData.scriptMovieSequences.size() > 0) &&
						 (nMovieIndex >= 0) &&
						 (nMovieIndex < moviesData.scriptMovieSequences.size()), "Invalid Movie", return )

	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKeyPrev = moviesData.scriptMovieSequences[nMovieIndex].posKeys.begin();
	vector<NDb::SScriptMovieKeyPos>::const_iterator itPosKey = itPosKeyPrev;
	++itPosKey;

	while ( itPosKey != moviesData.scriptMovieSequences[nMovieIndex].posKeys.end() )
	{
		if ( (fCurrTime > itPosKeyPrev->fStartTime) && (fCurrTime <= itPosKey->fStartTime) )
		{
			SetTime( itPosKeyPrev->fStartTime );
			return;
		}

		itPosKeyPrev = itPosKey;
		++itPosKey;
	}
}


void CScriptMoviesMutatorHolder::SetCallbackFuncName( const string &_szCallbackFuncName )
{
	szCallbackFuncName = _szCallbackFuncName;
}


void CScriptMoviesMutatorHolder::ScriptCallback( int nKeyID )
{
	if ( !szCallbackFuncName.empty() )
	{
		WriteToPipe( PIPE_SCRIPT_CMDS, StrFmt("%s(%d)", szCallbackFuncName.c_str(), nKeyID) );
	}
}

int CScriptMoviesMutatorHolder::operator&( IBinSaver &Saver )
{
	Saver.Add( 6, &moviesData );
	Saver.Add( 7, &nMovieIndex );
	Saver.Add( 8, &movieStartTime );
	Saver.Add( 9, &fCurrTime );
	Saver.Add( 10, &bLoopPlayback );
	Saver.Add( 11, &fSpeedCoeff );
	Saver.Add( 14, &pTimer );
	Saver.Add( 16, &placement );
	Saver.Add( 17, &cameraQuats );
	Saver.Add( 18, &eMode );
	Saver.Add( 19, &cameraAnchors );
	Saver.Add( 20, &cameraDists );
	Saver.Add( 21, &nLastKeyID );
	Saver.Add( 22, &szCallbackFuncName );

	return 0;
}


REGISTER_SAVELOAD_CLASS( 0x1B1C84C0, CSCamDMoveFlightMutator )
REGISTER_SAVELOAD_CLASS( 0x1B1C84C2, CSCamDFollowFlightMutator )
REGISTER_SAVELOAD_CLASS( 0x1B1C84C3, CSCamDRotateFlightMutator )
REGISTER_SAVELOAD_CLASS( 0x1B1C84C4, CSCamSplineMutator )
REGISTER_SAVELOAD_CLASS( 0x1B224400, CScriptMoviesMutatorHolder )


