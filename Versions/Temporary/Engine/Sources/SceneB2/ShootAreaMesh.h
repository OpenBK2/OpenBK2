#pragma once

#include "TerrainManager.h"

struct SLinearHeightFunctor
{
	virtual float GetHeight( float x ) = 0;
	virtual float GetGrid( float t ) = 0;
};

class CShootAreaMesh : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_BASIC_METHODS( CShootAreaMesh )
	
	CDGPtr<CFuncBase<SFBTransform> > pTransform;
	float fStartAngle;
	float fEndAngle;
	float fMinRadius;
	float fMaxRadius;
	CPtr<CTerrainManager> pTerraManager;
	float fWidth;
	
	CVec2 GetCenter();
	static void FillGrid( vector<float> &grid, int nNumPoints, SLinearHeightFunctor &func );
	static void LinearTransform( vector<float> &output, const vector<float> &input, float fMin, float fMax );
	void BuildCircle( vector<NGScene::SVertex> &verts, vector<STriangle> &tris, float fRadius );
	void BuildLine( vector<NGScene::SVertex> &verts, vector<STriangle> &tris, float fRadiant );
	void BuildSector( vector<NGScene::SVertex> &verts, vector<STriangle> &tris );
	static void FillVertexData( NGScene::SVertex &vertex );

protected:
	bool NeedUpdate() { return pTransform.Refresh(); }
	void Recalc();
	CShootAreaMesh() {}

public:
	CShootAreaMesh( const CVec2 &vCenter, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, CTerrainManager *_pTerraManager, float fWidth = 1.0f );
	CShootAreaMesh( CFuncBase<SFBTransform> *pTransform, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, CTerrainManager *_pTerraManager, float fWidth = 1.0f );
	
	int operator&( IBinSaver &saver )
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

