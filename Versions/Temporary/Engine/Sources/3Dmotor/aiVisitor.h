#pragma once
#include "GSkeleton.h"
#include "Misc/Sync.h"

namespace NDb
{
//	struct SRPGArmor;
	struct SAIGeometry;
	struct SSkeleton;
}
class CMemObject;
struct IAIVisitor : public IVisitorBase
{
	struct SPieceMap
	{
		int nPieceID, nUserID;
	};
	virtual CObjectBase *AddHull( const NDb::SAIGeometry *pAIGeom, 
		const SHMatrix &pos, 
		const NDb::CResource *pArmor, int nFloor, int nMask ) = 0;
	// if pArmor is 0 then adding terrain part
	virtual CObjectBase *AddHull( CMemObject *pAIGeom, 
		const SHMatrix &pos, 
		const NDb::CResource *pArmor, int nFloor, int nMask ) = 0;
	virtual CObjectBase *AddAnimatedHull( const NDb::SAIGeometry *pAIGeom, const NAnimation::SGrannySkeletonHandle &skeletonH,
		CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, 
		const NDb::CResource *pArmor, int nFloor, int nMask ) = 0;
	virtual void AssignUserID( CObjectBase *p, int _nUserID ) = 0;
/*	virtual void AddFlippingHull( const NDb::SAIGeometry *pAIGeom, const NDb::SSkeleton *pSkeleton, const SHMatrix &pos,
		CFuncBase<NAnimation::SSkeletonPose> *pAn1, CFuncBase<NAnimation::SSkeletonPose> *pAn2, 
		const NDb::CResource *pArmor, int nFloor, int nMask, bool bOpen, int nDoorID, int nDestroyStage, bool bTransparentIfOpen ) = 0;
	virtual void AddPieces( const NDb::SAIGeometry *pAIGeom, const vector<SPieceMap> &parts,
		const SHMatrix &pos, 
		const NDb::CResource *pArmor, int nFloor, int nMask ) = 0;
	virtual void AddTerrainPart( CPtrFuncBase<CTerrainPart> *pPart, 
		const NDb::CResource *pArmor, int nFloor, int nMask ) = 0;
	virtual void LoadGeometry( const NDb::SAIGeometry *pAIGeom ) {}
	virtual void LoadSkinGeometry( const NDb::SAIGeometry *pAIGeom, const NDb::SSkeleton *pSkeleton ) {}
	*/
};

