#pragma once

#include "TerrainManager.h"

class CShootAreaMesh : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_BASIC_METHODS( CShootAreaMesh )

	CDGPtr<CFuncBase<SFBTransform>> pTransform;
	float fStartAngle;
	float fEndAngle;
	float fMinRadius;
	float fMaxRadius;
	CPtr<CTerrainManager> pTerraManager;
	float fWidth;

	CVec2 GetCenter();
	void BuildCircle( std::vector<NGScene::SVertex> &verts, std::vector<STriangle> &tris, float fRadius );
	void BuildLine( std::vector<NGScene::SVertex> &verts, std::vector<STriangle> &tris, float fRadiant );
	static void FillVertexData( NGScene::SVertex &vertex );

protected:
	bool NeedUpdate() override
	{
		return pTransform.Refresh();
	}

	void Recalc() override;

	CShootAreaMesh() {}

public:
	CShootAreaMesh( const CVec2 &vCenter,
	                float fStartAngle,
	                float fEndAngle,
	                float fMinRadius,
	                float fMaxRadius,
	                CTerrainManager *_pTerraManager,
	                float fWidth = 1.0f );
	CShootAreaMesh( CFuncBase<SFBTransform> *pTransform,
	                float fStartAngle,
	                float fEndAngle,
	                float fMinRadius,
	                float fMaxRadius,
	                CTerrainManager *_pTerraManager,
	                float fWidth = 1.0f );

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 2, &pTerraManager );
		saver.Add( 3, &pTransform );
		saver.Add( 4, &fStartAngle );
		saver.Add( 6, &fEndAngle );
		saver.Add( 7, &fMinRadius );
		saver.Add( 8, &fMaxRadius );
		return 0;
	}
};
