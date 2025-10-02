#pragma once

namespace NDb
{
	struct SModel;
}

namespace NGScene
{

class IGameView;

class ISkyDome : public CObjectBase
{
public:
	virtual void SetCameraPos( const CVec3 &vCamPos ) = 0;
};

ISkyDome *CreateSkyDome( NGScene::IGameView *pView, const NDb::SModel *pModel );

} // namespace NGScene

