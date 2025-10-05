#include "stdafx.h"

#include "Camera.h"
#include "SceneInternal.h"
#include "VisObjDesc.h"
#include "VisObjIconsManager.h"
#include "DBSceneConsts.h"
#include "VisObjIcons.h"

#define DEF_ICON_SIZE 1.5f
//#define DEF_ICON_ADD_HEIGHT_SCALE 0.1f
#define DEF_ICON_HPBAR_WIDTH ( 0.15f / 2 )
#define DEF_ICON_HPBAR_BORDER_WIDTH ( 0.0375f / 2 )
#define DEF_ICON_ELEM_WIDTH ( 0.3f / 2 )
#define DEF_ICON_ELEM_WIDTH_BETWEEN ( 0.2f / 2 )

void inline SetCompactVcetorAndSaveW( NGfx::SCompactVector &dst, const NGfx::SCompactVector &src )
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

void CVisObjIconInfo::OrientToViewer()
{
	//SHMatrix matrix;
	//Invert( &matrix, pCamera->GetViewMatrix() );
	//matrix = matrix * pCamera->GetProjectiveMatrix();
	const SHMatrix matrix = pCamera->GetViewMatrix();

	CVec3 s[4];
	NGfx::SCompactVector vNorm;
	CalcCompactVector( &vNorm, -pCamera->GetViewMatrix().GetZAxis3() );

	const int nIconsNum = data.vertices.size() >> 2;

	for ( int i = 0; i < nIconsNum; ++i )
	{
		const int nInd = i << 2;
		const CVec2 &vCurSizesMin = iconsSizesMin[i];
		const CVec2 &vCurSizesMax = iconsSizesMax[i];

		matrix.RotateVector( &s[0], CVec3( vCurSizesMin.x, 0, vCurSizesMin.y ) );
		matrix.RotateVector( &s[1], CVec3( vCurSizesMax.x, 0, vCurSizesMin.y ) ); 
		matrix.RotateVector( &s[2], CVec3( vCurSizesMax.x, 0, vCurSizesMax.y ) ); 
		matrix.RotateVector( &s[3], CVec3( vCurSizesMin.x, 0, vCurSizesMax.y ) );

		data.vertices[nInd].pos = vCenter + s[0];
		SetCompactVcetorAndSaveW( data.vertices[nInd].normal, vNorm );
		//data.vertices[nInd].normal = vNorm;
		data.vertices[nInd + 1].pos = vCenter + s[1];
		SetCompactVcetorAndSaveW( data.vertices[nInd + 1].normal, vNorm );
		//data.vertices[nInd + 1].normal = vNorm;
		data.vertices[nInd + 2].pos = vCenter + s[2];
		SetCompactVcetorAndSaveW( data.vertices[nInd + 2].normal, vNorm );
		//data.vertices[nInd + 2].normal = vNorm;
		data.vertices[nInd + 3].pos = vCenter + s[3];
		SetCompactVcetorAndSaveW( data.vertices[nInd + 3].normal, vNorm );
		//data.vertices[nInd + 3].normal = vNorm;
	}
}

void CVisObjIconInfo::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;

	OrientToViewer();

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	objData.geometry = data.triangles;

	pValue->AssignFast( &objData );
	bUpdate = false;
}

inline bool CVisObjIconInfo::NeedUpdate()
{
	 return bUpdate || pCamera->WasUpdated();
}

inline void AddSingleIcon( CVisObjIconInfo *pIconInfo, const float x1, const float y1, const float x2, const float y2,
													const CTRect<float> &rcRect, NGScene::SVertex &vert )
{
	const CVec3 &vPos = pIconInfo->vCenter;
	std::vector<NGScene::SVertex> &vertices = pIconInfo->data.vertices;
	std::vector<STriangle> &triangles = pIconInfo->data.triangles;
	const int nVertsOffs = vertices.size();
	vert.pos.Set( vPos.x + x1, vPos.y, vPos.z + y1 );
	vert.tex.Set( rcRect.GetLeftBottom().x, rcRect.GetLeftBottom().y );
	vertices.push_back( vert );
	vert.pos.Set( vPos.x + x2, vPos.y, vPos.z + y1 );
	vert.tex.Set( rcRect.GetRightBottom().x, rcRect.GetRightBottom().y );
	vertices.push_back( vert );
	vert.pos.Set( vPos.x + x2, vPos.y, vPos.z + y2 );
	vert.tex.Set( rcRect.GetRightTop().x, rcRect.GetRightTop().y );
	vertices.push_back( vert );
	vert.pos.Set( vPos.x + x1, vPos.y, vPos.z + y2 );
	vert.tex.Set( rcRect.GetLeftTop().x, rcRect.GetLeftTop().y );
	vertices.push_back( vert );
	triangles.push_back( STriangle( nVertsOffs, nVertsOffs + 1, nVertsOffs + 2 ) );
	triangles.push_back( STriangle( nVertsOffs + 2, nVertsOffs + 3, nVertsOffs ) );
	pIconInfo->iconsSizesMin.push_back( CVec2( x1, y1 ) );
	pIconInfo->iconsSizesMax.push_back( CVec2( x2, y2 ) );
}

void SVisObjIcons::CreateIcons( NGScene::IGameView *pGameView, const NDb::SIconsSet *pIconSet, const CVec3 &vPos )
{
	if ( iconHolder.pPatch == 0 )
		iconHolder.pPatch = new CVisObjIconInfo( Camera() );

	iconHolder.pPatch->data.vertices.reserve( 16 );
	iconHolder.pPatch->data.vertices.resize( 0 );
	iconHolder.pPatch->data.triangles.reserve( 8 );
	iconHolder.pPatch->data.triangles.resize( 0 );

	iconHolder.pPatch->iconsSizesMin.reserve( 4 );
	iconHolder.pPatch->iconsSizesMin.resize( 0 );
	iconHolder.pPatch->iconsSizesMax.reserve( 4 );
	iconHolder.pPatch->iconsSizesMax.resize( 0 );

	const float fWidth2 = fIconHalfWidth * 2.0f;
	const float fElemWidth = fIconHalfWidth * DEF_ICON_ELEM_WIDTH * 2.0f;
	float fElemOffset = -fIconHalfWidth;

	NGScene::SVertex vert;
	CalcCompactVector( &(vert.texU), CVec3( 1, 0, 0 ) );
	CalcCompactVector( &(vert.texV), CVec3( 0, 1, 0 ) );
	CalcCompactVector( &(vert.normal), CVec3( 0, 0, 1 ) );

	iconHolder.pPatch->vCenter.Set( vPos.x, vPos.y, vPos.z + fIconHalfWidth );

	int nInd;

	for ( std::vector<SPresentIcon>::const_iterator it = icons.begin(); it != icons.end(); ++it )
	{
		vert.normal.w = it->cAlpha;
		for ( int i = 0; i < pIconSet->icons.size(); ++i )
		{
			if ( pIconSet->icons[i].eType == it->eType )
			{
				switch ( it->eType )
				{
				case NDb::SIconsSet::SIconType::ICONTYPE_HPBAR:
				{
					// border
					AddSingleIcon( iconHolder.pPatch, -fIconHalfWidth, -fIconHalfWidth, -fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ),
						-fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ), pIconSet->icons[i].rcRect, vert );
					AddSingleIcon( iconHolder.pPatch, -fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ), -fIconHalfWidth,
						fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ), -fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ),
						pIconSet->icons[i + 1].rcRect, vert );
					AddSingleIcon( iconHolder.pPatch, fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ), -fIconHalfWidth,
						fIconHalfWidth, -fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_WIDTH * 2.0f ),
						pIconSet->icons[i + 2].rcRect, vert );
					// color
					nInd = 4;
					if ( it->fValue > 0.33333333f )
					{
						nInd = ( it->fValue > 0.66666666f ) ? 3 : 5;
					}
					const float fHPBarLength = fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_BORDER_WIDTH * 2.0f ) * it->fValue * 2.0f;
					AddSingleIcon( iconHolder.pPatch,
						-fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_BORDER_WIDTH * 2.0f ),
						-fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_BORDER_WIDTH * 2.0f ),
						-fIconHalfWidth * ( 1.0f - DEF_ICON_HPBAR_BORDER_WIDTH * 2.0f ) + fHPBarLength,
						-fIconHalfWidth * ( 1.0f - ( DEF_ICON_HPBAR_WIDTH - DEF_ICON_HPBAR_BORDER_WIDTH ) * 2.0f ),
						pIconSet->icons[i + nInd].rcRect, vert );
				 }
				 break;

//	 			case NDb::SIconsSet::SIconType::ICONTYPE_BROKENTRUCK:
	 				default:
						AddSingleIcon( iconHolder.pPatch, fElemOffset, -fElemWidth * 2, fElemOffset + fElemWidth, -fElemWidth, pIconSet->icons[i].rcRect, vert );
						fElemOffset += fElemWidth * ( 1.0f + DEF_ICON_ELEM_WIDTH_BETWEEN );
					break;
				}
				break;
			}
		}
	}

	SBound bound;
	bound.BoxInit( CVec3( vPos.x - fIconHalfWidth, vPos.y - fIconHalfWidth, vPos.z ),
		CVec3( vPos.x + fIconHalfWidth, vPos.y + fIconHalfWidth, vPos.z + fWidth2 ) );

	if ( pBound == 0 )
		pBound = new CCSBound();
	pBound->Set( bound );

	NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
	iconHolder.pHolder = pGameView->CreateDynamicMesh( pGameView->MakeMeshInfo( iconHolder.pPatch, pIconSet->pMaterialZCheck ), 0, pBound, NGScene::MakeLargeHintBound(), room );

	iconHolder.pPatch->ForceUpdate();
}

void SVisObjIcons::MoveIcons( const CVec3 &vPos, const float fObjHeight )
{
	if ( iconHolder.pPatch == 0 )
		iconHolder.pPatch = new CVisObjIconInfo( Camera() );

	const CVec3 vIconPos( vPos.x, vPos.y, vPos.z + fObjHeight + fIconAddHeight + fIconHalfWidth/* * ( 1.0f + DEF_ICON_ADD_HEIGHT_SCALE )*/ );
	const CVec3 vCurDiffer = vIconPos - iconHolder.pPatch->vCenter;

	for ( std::vector<NGScene::SVertex>::iterator it = iconHolder.pPatch->data.vertices.begin(); it != iconHolder.pPatch->data.vertices.end(); ++it )
		it->pos += vCurDiffer;

	iconHolder.pPatch->vCenter = vIconPos;

	SBound bound;
	bound.BoxInit( CVec3( vIconPos.x - fIconHalfWidth, vIconPos.y - fIconHalfWidth, vIconPos.z ),
		CVec3( vIconPos.x + fIconHalfWidth, vIconPos.y + fIconHalfWidth, vIconPos.z + fIconHalfWidth * 2.0f ) );
	if ( pBound == 0 )
		pBound = new CCSBound();
	pBound->Set( bound );

	iconHolder.pPatch->ForceUpdate();
}

inline void GetVisObjIconsBase( CVec3 *pvPos, const SModelVisObjDesc *pVOD )
{
	NI_ASSERT( pVOD->pModel, StrFmt( "No model for VisObj %d", pVOD->GetID()) );
	if ( pVOD->pModel )
	{
		pVOD->GetPlacement().forward.RotateHVector( pvPos, pVOD->pModel->pGeometry->vCenter );
		pvPos->z += pVOD->pModel->pGeometry->vSize.z * 0.5f;
	}
	//*pvPos = pVOD->GetPlacement().forward.GetTrans3();
	//pvPos->z += pVOD->pModel->pGeometry->pAIGeometry->vAABBHalfSize.z;
}

void CScene::SetIcon( const SSceneObjIconInfo &iconInfo )
{
	if ( iconInfo.IsEmpty() )
	{
		RemoveIcon( iconInfo.nID );
		return;
	}
	
	if ( !( data[eScene]->pVisObjIconsManager ) && ( data[eScene]->pSceneConsts ) && ( data[eScene]->pSceneConsts->pVisObjIconsSet ) )
	{
		data[eScene]->pVisObjIconsManager = new CVisObjIconsManager;
		data[eScene]->pVisObjIconsManager->Init( data[eScene]->pSceneConsts->pVisObjIconsSet );
		data[eScene]->pVisObjIconsManager->Attach2DView( GetG2DView() );
	}

	if ( !( data[eScene]->pVisObjIconsManager ) )
		return;

	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( iconInfo.nID );
	if ( pos == data[eScene]->visObjects.end() ) // such object is not existed
		return;

	if ( const SModelVisObjDesc *pVOD = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second ) )
	{
		CVec3 vPos;
		GetVisObjIconsBase( &vPos, pVOD );
		data[eScene]->pVisObjIconsManager->SetIcon( iconInfo, vPos );
	}
}

void CScene::RemoveIcon( const int nID )
{
	if ( data[eScene]->pVisObjIconsManager )
		data[eScene]->pVisObjIconsManager->RemoveIcon( nID );
}

REGISTER_SAVELOAD_CLASS( 0x1311E302, CVisObjIconInfo );


