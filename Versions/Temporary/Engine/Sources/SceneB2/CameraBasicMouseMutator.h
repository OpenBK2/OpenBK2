#pragma once

#include "Camera.h"
#include "Input/GameMessage.h"

#include <cstdint>

namespace NCamera
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SLimit
	{
		float fMin;
		float fMax;
		float fAve;
		float fAutoSpeed;
		float fManualSpeed;
		//
		bool bCyclic;
		//
		void SetLimit( float _fMin, float _fMax, float _fAve, float _fAuto, float _fMan, bool _bCyclic )
		{ 
			fMin = _fMin; 
			fMax = _fMax; 
			fAve = _fAve;
			//
			if ( fMin < fMax )
			{
				fMin = (std::min)( fAve, fMin );
				fMax = (std::max)( fAve, fMax );
			}
			//
			fAutoSpeed = _fAuto; 
			fManualSpeed = _fMan; 
			bCyclic = _bCyclic; 
		}

		bool IsEmpty() const { return fMin >= fMax; }
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CCameraBasicMouseMutator : public CCameraPlacement, protected NInput::CGMORegContainer
	{
		// placement
		CVec3 vAnchor;									// camera's anchor point
		float fDistance;								// distance from camera to anchor point
		// limits. NOTE: all limits (except anchor limit) are taken from consts
		CTRect<float> rcAnchorLimit;		// anchor movement limit (rect)
		SLimit distanceLimit;						// distance limit (min/max)
		SLimit pitchLimit;							// pitch limit (min/max)
		SLimit yawLimit;								// yaw (rotation) limit (min/max)
		uint32_t timeLastUpdate;
		//
		// input sliders. NOTE: don't store it - acquire in Init()
		NInput::CBind sliderFwd;				// forward/backward moving slider (parrallel ground)
		NInput::CBind sliderStrafe;			// strafe (side moving) slider (parrallel ground)
		NInput::CBind sliderZoom;				// zoom/unzoom slider (moving toward/outward the anchor point)
		NInput::CBind sliderPitch;			// camera pitch
		NInput::CBind sliderYaw;				// camera yaw
		NInput::CBind sliderMousePitch;	// camera pitch from mouse
		NInput::CBind sliderMouseYaw;		// camera yaw from mouse
		NInput::CBind sliderMouseZoom;	// camera yaw from mouse
		NInput::CBind sliderMouseForward;
		NInput::CBind sliderMouseStrafe;
		float fScrollSpeedX;						// border scrolling
		float fScrollSpeedY;
		bool bAutoPositioning;								// allow camera automatically return to bounds
		std::unordered_map<std::string, bool> manualLockers;	// allow reaction on manual scrolling
		bool bWasUpdated;								// camera was updated from input
		bool bWasUpdatedExternally;			// camera was updated from external
		float fYawSpeed;

	protected:
		float GetPitchMouseDelta() { return sliderMousePitch.GetDelta(); }
		float GetPitchKeyboardDelta() { return sliderPitch.GetDelta(); }
	//	float GetPitchDelta() { return sliderPitch.GetDelta() + sliderMousePitch.GetDelta(); }
		float GetYawMouseDelta() { return sliderMouseYaw.GetDelta(); }
		float GetYawKeyboardDelta() { return sliderYaw.GetDelta(); }
	//	float GetYawDelta() { return sliderYaw.GetDelta() + sliderMouseYaw.GetDelta(); }
		float GetForwardMouseDelta() { return sliderMouseForward.GetDelta(); }
		float GetForwardKeyboardDelta() { return sliderFwd.GetDelta(); }
	//	float GetForwardDelta() { return sliderFwd.GetDelta() + sliderMouseForward.GetDelta(); }
		float GetStrafeMouseDelta() { return sliderMouseStrafe.GetDelta(); }
		float GetStrafeKeyboardDelta() { return sliderStrafe.GetDelta(); }
	//	float GetStrafeDelta() { return sliderStrafe.GetDelta() + sliderMouseStrafe.GetDelta(); }
		float GetZoomMouseDelta() { return sliderMouseZoom.GetDelta(); }
		float GetZoomKeyboardDelta() { return sliderZoom.GetDelta(); }
	//	float GetZoomDelta() { return sliderZoom.GetDelta() + sliderMouseZoom.GetDelta(); }
		void ClearSliders();
		//
		const SLimit &GetPitchLimit() const { return pitchLimit; }
		const SLimit &GetYawLimit() const { return yawLimit; }
		const SLimit &GetDistanceLimit() const { return distanceLimit; }
		const CTRect<float> &GetAnchorLimit() const { return rcAnchorLimit; }
		//
		float GetScrollSpeedX() const { return fScrollSpeedX; }
		float GetScrollSpeedY() const { return fScrollSpeedY; }
		//
		float GetYawSpeed() const { return fYawSpeed; }
		//
		bool HasAutopositioning() const { return bAutoPositioning; }
		void SetWasUpdated( bool _bWasUpdated ) { bWasUpdated = _bWasUpdated; }
		//
		uint32_t GetTimeLastUpdate() const { return timeLastUpdate; }
		void SetTimeLastUpdate( const uint32_t &_timeLastUpdate ) { timeLastUpdate = _timeLastUpdate; }
		bool IsCameraLocked() const { return !manualLockers.empty(); }

	public:
		CCameraBasicMouseMutator();

		bool ProcessEvent( const SGameMessage &msg );
		//
		void ResetToDefault();
		void SetLimits( const NCamera::ELimitsType eLimitsType, const NCamera::SCameraLimits &limits );
		void GetLimits( const NCamera::ELimitsType eLimitsType, NCamera::SCameraLimits *pLimits );
		void SetAnchorLimits( const CTRect<float> &vLimits );
		void SwitchAutoPositioning( const bool bAllowAutoPositioning );
		void SwitchManualScrolling( const std::string &szLocker, const bool bManualOn );
		const CVec3 GetListener() const;
		void SetAnchor( const CVec3 &_vAnchor ) { vAnchor = _vAnchor; }
		const CVec3& GetAnchor() const { return vAnchor; }
		const CVec3& GetPos() const { return vPosition; }
		void ResetScrolling() { fScrollSpeedX = 0; fScrollSpeedY = 0; }
		void SetScrollSpeedX( const float _fSpeed ) { fScrollSpeedX = _fSpeed; }
		void SetScrollSpeedY( const float _fSpeed ) { fScrollSpeedY = _fSpeed; }
		void SetYawSpeed( const float _fSpeed ) { fYawSpeed = _fSpeed; }
		float GetYaw() const { return fYaw; }
		float GetPitch() const { return fPitch; }
		float GetFOV() const { return fFOV; }
		void SetYaw( const float _fYaw ) { fYaw = _fYaw; }
		void SetPitch( const float _fPitch ) { fPitch = _fPitch; }
		void SetFOV( float _fFOV ) { fFOV = _fFOV; }
		void SetPlacement( const float _fDist, const float _fPitch, const float _fYaw ) { fDistance = _fDist; fPitch = _fPitch; fYaw = _fYaw; }
		void GetPlacement( float *pfDist, float *pfPitch, float *pfYaw ) { *pfDist = fDistance; *pfPitch = fPitch; *pfYaw = fYaw; }
		void SetDistance( const float _fDist ) { fDistance = _fDist; }
		float GetDistance() const { return fDistance; }
		//
		bool WasUpdated() const { return bWasUpdated; }
		//
		int operator&( IBinSaver &saver );
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	float IncrementValue( float fCurrValue, float fAdd, const SLimit &limit, float fTimeDiff, const bool bAutoPositioning, bool bFreeCamera );
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


