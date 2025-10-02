#include "StdAfx.h"
#include "GView.h"
#include "SkyDome.h"

namespace NGScene
{

class CSkyDomeTransferer : public CFuncBase<SFBTransform>
{
	OBJECT_BASIC_METHODS( CSkyDomeTransferer )
	//
	bool bUpdate;
protected:
	bool NeedUpdate() { return bUpdate; }
	void Recalc() { bUpdate = false; }
public:
	CSkyDomeTransferer() : bUpdate(false)
	{
		Identity( &value.forward );
		Identity( &value.backward );
	}
	//
	void SetCameraPos( const CVec3 &vCamPos )
	{
		value.forward._14 = vCamPos.x;
		value.forward._24 = vCamPos.y;
		value.forward._34 = 0; // vCamPos.z; // - SkyDome should not change height.
		Invert( &value.backward, value.forward );
		bUpdate = true;
	}
};

class CSkyDome : public ISkyDome
{
	OBJECT_BASIC_METHODS( CSkyDome )
	//
	ZDATA
	CObj<CSkyDomeTransferer> pTransferer;
	CObj<CObjectBase> pSkyDomeMesh;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTransferer); f.Add(3,&pSkyDomeMesh); return 0; }
protected:
	CSkyDome() {}
public:
	CSkyDome( NGScene::IGameView *pView, const NDb::SModel *pModel )
	{
		NGScene::IGameView::SMeshInfo meshInfo;
		pView->CreateMeshInfo( pModel, &meshInfo, false );
		pTransferer = new CSkyDomeTransferer();
		pSkyDomeMesh = pView->CreateMesh( meshInfo, pTransferer.GetPtr(), 0, 0 );
	}
	//
	virtual void SetCameraPos( const CVec3 &vCamPos )
	{
		pTransferer->SetCameraPos( vCamPos );
	}
};

ISkyDome *CreateSkyDome( NGScene::IGameView *pView, const NDb::SModel *pModel )
{
	return new CSkyDome( pView, pModel );
}

} // namespace NGScene

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x131A6B40, CSkyDome );
REGISTER_SAVELOAD_CLASS( 0x751A9B81, CSkyDomeTransferer );
