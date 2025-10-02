#pragma once

#include "TerrainManager.h"

class CLineMesh : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_BASIC_METHODS( CLineMesh )

	CVec2 vStart;
	CVec2 vEnd;
	CPtr<CTerrainManager> pTerraManager;

	static void FillVertexData( NGScene::SVertex &vertex );
	void BuildLine( vector<NGScene::SVertex> &verts, vector<STriangle> &tris );
protected:
	void Recalc();
	CLineMesh() {}
public:
	CLineMesh( const CVec2 &vStart, const CVec2 &vEnd, CTerrainManager *_pTerraManager );

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &vStart );
		saver.Add( 3, &vEnd );
		saver.Add( 4, &pTerraManager );
		return 0;
	}
};


