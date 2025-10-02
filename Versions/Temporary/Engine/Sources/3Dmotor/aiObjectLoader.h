#ifndef __aiObjectLoader_H_
#define __aiObjectLoader_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GFileSkin.h"
#include "..\System\GResource.h"
#include "aiObject.h"
#include "GSkeleton.h"

class CMemObject;

namespace NDb
{
	struct SAIGeometry;
}

namespace NAI
{
class CGeometryInfo;

class CLoadAIGeometryFromA5Exporter : public NGScene::CResourceLoader<int, CGeometryInfo>
{
	OBJECT_BASIC_METHODS(CLoadAIGeometryFromA5Exporter);
protected:
	virtual void Recalc();
};

class CLoadAIGeometryFromGranny : public CHoldedPtrFuncBase<CGeometryInfo>
{
	typedef CHoldedPtrFuncBase<CGeometryInfo> TParent;
	OBJECT_BASIC_METHODS(CLoadAIGeometryFromGranny);

	ZDATA
	CDGPtr<CPtrFuncBase<NAnimation::CGrannyFileInfo> > pData;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pData); return 0; }
	bool NeedUpdate() { return TParent::NeedUpdate() | pData.Refresh(); }
	void Recalc();
public:
	void SetKey( const NDb::SAIGeometry *pGeometry );
};

/*class CBSPTree;
class CPrecalcFlipper : public CObjectBase
{
	OBJECT_BASIC_METHODS(CPrecalcFlipper);
public:
	vector<NAI::CPrecalcPieces> treesOpen;
	vector<NAI::CPrecalcPieces> treesClosed;
};*/

class CMemGeometryInfo : public CPtrFuncBase<CGeometryInfo>
{
	OBJECT_BASIC_METHODS(CMemGeometryInfo);
protected:
	ZDATA
	CPtr<CMemObject> pMemObject;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMemObject); return 0; }
	virtual void Recalc();
public:
	CMemGeometryInfo( CMemObject *p = 0 ): pMemObject(p) {}
};

class CFileSkinPoints : public CObjectBase
{
	OBJECT_BASIC_METHODS(CFileSkinPoints);
public:
	vector<SMassSphere> spheres;
	CVec3 massCenter;
	struct SBodypart
	{
		vector<CVec3> points;
		CEdgesInfo edges;
		vector<NGScene::SVertexWeight> weights;
	};
	typedef hash_map<int, SBodypart> CBodypartsHash;
	CBodypartsHash parts;
};

class CFileSkinPointsLoadFromA5Exporter : public NGScene::CResourceLoader<int, CFileSkinPoints>
{
	OBJECT_BASIC_METHODS(CFileSkinPointsLoadFromA5Exporter);
protected:
	virtual void Recalc();
};

class CFileSkinPointsLoadFromGranny : public CHoldedPtrFuncBase<CFileSkinPoints>
{
	typedef CHoldedPtrFuncBase<CFileSkinPoints> TParent;
	OBJECT_BASIC_METHODS(CFileSkinPointsLoadFromGranny);
	
	ZDATA
	CDGPtr<CPtrFuncBase<NAnimation::CGrannyFileInfo> > pData;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pData); return 0; }
	bool NeedUpdate() { return TParent::NeedUpdate() | pData.Refresh(); }
	void Recalc();
public:
	void SetKey( const NDb::SAIGeometry *pGeometry );
};

class CSkinner: public CPtrFuncBase<CGeometryInfo>
{
	OBJECT_BASIC_METHODS(CSkinner);
	ZDATA
	CDGPtr< CFuncBase<NGScene::SSkeletonMatrices> > pAnimation;
	CDGPtr< CPtrFuncBase<CFileSkinPoints> > pSkin;
	//bool bDoPrecalc;
	//CPrecalcPieces setTrees;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimation); f.Add(3,&pSkin); return 0; }
protected:
	virtual bool NeedUpdate() { return pAnimation.Refresh() | pSkin.Refresh(); }
	virtual void Recalc();
public:
	CSkinner() {}
	CSkinner( CPtrFuncBase<CFileSkinPoints> *_pSkin, CFuncBase<NGScene::SSkeletonMatrices> *_pAnimation )
		:pSkin(_pSkin), pAnimation(_pAnimation) {}//, bDoPrecalc(false) {}
	//void CreatePrecalcInfo() { bDoPrecalc = true; }
	//void SetPrecalcInfo( const CPrecalcPieces &trees ) { bDoPrecalc = false; setTrees = trees; }
};

// create cube hull
//void MakeCube( CConvexHull *pRes, const CVec3 &base, const CVec3 &size );
}
#endif
