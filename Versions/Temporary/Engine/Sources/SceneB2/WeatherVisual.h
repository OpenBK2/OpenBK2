#pragma once

#include "../3Dmotor/GView.h"
#include "../3Dmotor/GObjectInfo.h"
#include "Camera.h"

namespace NDb
{
	enum EWeatherState
	{
		EWS_CLEAR,
		EWS_BAD,
		EWS_FADE_IN,	// start
		EWS_FADE_OUT,	// finish
		EWS_UNKNOWN
	};
	struct SWeatherDesc;
}

namespace NGScene
{
	class CCVec3;
	class CAnimLightInfo;
	class CAnimLight;
};

interface IFullScreenFader;

class CWeatherVisual : public CFuncBase<bool>
{
	OBJECT_NOCOPY_METHODS( CWeatherVisual )

	class CWeatherPart : public CPtrFuncBase<NGScene::CObjectInfo>
	{
		OBJECT_NOCOPY_METHODS( CWeatherPart )

		CDGPtr<CWeatherVisual> pWeather;
		int nNumPart;

	public:
		CWeatherPart() : nNumPart(-1) {}
		CWeatherPart( CWeatherVisual *_pWeather, int _nNumPart ) : pWeather(_pWeather), nNumPart(_nNumPart) {}

		bool NeedUpdate() { return pWeather.Refresh(); }
		void Recalc();
	};

	NTimer::STime timeStart;
	NTimer::STime timeLength;
	CDGPtr< CFuncBase<STime> > pTimer;
	CDGPtr<CFuncBase<STime>, CPtr<CFuncBase<STime> > > pAbsTimer;
	CDBPtr<NDb::SWeatherDesc> pDesc;
	bool bActive;
	//bool bExistLightning;
	//bool bThunder;
	NDb::EWeatherState eState;

	CVec2 vBBMin, vBBMax;
	CVec2 vWindOffset;
	vector<CVec3> parts;
	vector<BYTE> partsPresent;
	CVec3 vCameraAnchor, vCameraEye, vViewNormal, vViewNormal3;
	NTimer::STime timeLightStop;
	NTimer::STime timeThunder;
	NTimer::STime timeLastRecalc;
	NTimer::STime timeNextAmbientSound;

	float fFadeCoeff;

	vector< CDGPtr<CPtrFuncBase<NGScene::CAnimLightInfo> > > lightningLoaders;
	CDGPtr<CPtrFuncBase<NGScene::CAnimLight> > pLightningAnimator;
	CObj<NGScene::CCVec3> pLightningColor;
	CPtr<IFullScreenFader> pScreenFader;

	CDBPtr<NDb::SAmbientLight> pNormalWeatherLight;
	CDBPtr<NDb::SAmbientLight> pBadWeatherLight;

	CObj<NDb::SAmbientLight> pCurrentWeatherLight;

	int nVisualParts;
	vector<NGScene::CObjectInfo::SData> visualData;

	void UpdateAmbientSound();

	void Recalc();
	bool NeedUpdate() { UpdateAmbientSound(); return (pTimer != 0 ? pTimer.Refresh() : false) || WasCameraMoved(); }

	void InitParts();
	void UpdateAreas();
	void UpdateLights( float fFadeCoeff );
	void PlayAmbientSound();

	bool WasCameraMoved() const
	{
		return ( (fabs(vCameraAnchor - Camera()->GetAnchor()) > FP_EPSILON) ||
						 (fabs(vCameraEye - Camera()->GetPos()) > FP_EPSILON) );
	}
	void UpdateViewNormal()
	{
		vCameraAnchor = Camera()->GetAnchor();
		vCameraEye = Camera()->GetPos();
		//
		vViewNormal = CVec3( -(vCameraEye.y - vCameraAnchor.y), vCameraEye.x - vCameraAnchor.x, 0 );
		vViewNormal3 = CVec3( (vCameraAnchor.z - vCameraEye.z), -(vCameraAnchor.z - vCameraEye.z), fabs(vViewNormal) );
		::Normalize( &vViewNormal );
		vViewNormal3.x *= vViewNormal.y;
		vViewNormal3.y *= vViewNormal.x;
		::Normalize( &vViewNormal3 );
	}

	CVec4 GetParticlesColor();

public:
	CWeatherVisual() {}
	CWeatherVisual( const NDb::SWeatherDesc *_pWeatherDesc, CFuncBase<STime> *_pTimer, IFullScreenFader *pScreenFader, 
		CDBPtr<NDb::SAmbientLight> pNormalWeatherLight, CDBPtr<NDb::SAmbientLight> pBadWeatherLight );

	NGScene::IGameView::SMeshInfo MakeMeshInfo();

	void SwitchOn( NTimer::STime _timeLength );
	void SwitchOff( NTimer::STime _timeLength );

	// Updating weather.
	void Update();

public:
	vector<NGScene::CObjectInfo::SData> *GetVisualData() { return &visualData; }

	int operator&( IBinSaver &saver );
};

