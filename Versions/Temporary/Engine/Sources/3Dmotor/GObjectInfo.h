#ifndef __OBJECTINFO_H_
#define __OBJECTINFO_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "3Dmotor_export.h"


#include "..\System\GResource.h"
#include "..\3DLib\GGeometry.h"

struct granny_mesh;
struct granny_model;
struct granny_file_info;
struct granny_skeleton;
struct granny_file;

namespace NAnimation
{
	class CGrannyFileInfo;
}

namespace NDb
{
	struct SGeometry;
	struct SSkeleton;
}

namespace NGScene
{

struct SPartAndSkeletonKey
{
	ZDATA
	CDBPtr<NDb::SGeometry> pGeometry; 
	int nGeometryPart;
	int nMaterialPart;
	CDBPtr<NDb::SSkeleton> pSkeleton;
	int nSkeletonPart;
	int nLightMapped;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGeometry); f.Add(3,&nGeometryPart); f.Add(4,&nMaterialPart); f.Add(5,&pSkeleton); f.Add(6,&nSkeletonPart); f.Add(7,&nLightMapped); return 0; }
	//
	SPartAndSkeletonKey() {}
	SPartAndSkeletonKey( const NDb::SGeometry *_pGeometry, int _nGeometryPart, int _nMaterialPart, const NDb::SSkeleton *_pSkeleton, int _nSkeletonPart, int _nLightMapped )
		: pGeometry(_pGeometry), nGeometryPart(_nGeometryPart), nMaterialPart(_nMaterialPart),
			pSkeleton(_pSkeleton), nSkeletonPart(_nSkeletonPart), nLightMapped(_nLightMapped)  {}
	bool operator==( const SPartAndSkeletonKey &a ) const 
	{ 
		return pGeometry == a.pGeometry && nGeometryPart == a.nGeometryPart && nMaterialPart == a.nMaterialPart
				&& pSkeleton == a.pSkeleton && nSkeletonPart == a.nSkeletonPart && nLightMapped == a.nLightMapped; 
	}
};

struct SPartAndSkeletonKeyHash
{
	int operator()( const SPartAndSkeletonKey &k ) const;
};

class CGrannyFile : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CGrannyFile)
public:
	granny_file *pFile;
	//
	CGrannyFile() : pFile(0) {}
	~CGrannyFile();
};

struct SGrannyFileLoaderInfo
{
	ZDATA
	string szResName;
	int nID;
	bool bAllowDelayedLoad;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szResName); f.Add(3,&nID); f.Add(4,&bAllowDelayedLoad); return 0; }
	//
	SGrannyFileLoaderInfo() : nID(-1), bAllowDelayedLoad(false) {}
	SGrannyFileLoaderInfo( const string &_szResName, int _nID, bool _bAllowDelayedLoad ) : szResName(_szResName), nID(_nID), bAllowDelayedLoad(_bAllowDelayedLoad) {}
	bool operator == ( const SGrannyFileLoaderInfo &s ) const { return s.nID == nID && s.szResName == szResName && s.bAllowDelayedLoad == bAllowDelayedLoad; }
};
/*
struct SGrannyFileLoaderInfo
{
ZDATA
string szResName;
SIntResKey key;
bool bAllowDelayedLoad;
ZEND ZEND int operator&( IBinSaver &f ) { f.Add(2,&szResName); f.Add(3,&key); f.Add(4,&bAllowDelayedLoad); return 0; }
//
SGrannyFileLoaderInfo() : key(-1), bAllowDelayedLoad(false) {}
SGrannyFileLoaderInfo( const string &_szResName, const GUID &uid, int _nID, bool _bAllowDelayedLoad ) : szResName(_szResName), key(uid, _nID), bAllowDelayedLoad(_bAllowDelayedLoad) {}
bool operator == ( const SGrannyFileLoaderInfo &s ) const { return s.key == key && s.szResName == szResName && s.bAllowDelayedLoad == bAllowDelayedLoad; }
};
*/

struct SGrannyFileLoaderInfoHash
{
	int operator() ( const SResKey<SGrannyFileLoaderInfo> &k ) const
	{
		return __stl_hash_string( k.tKey.szResName.c_str() ) ^ k.tKey.nID;
	}
};

class CGrannyMemFileLoader : public CLazyResourceLoader<SGrannyFileLoaderInfo, CGrannyFile>
{
	OBJECT_BASIC_METHODS(CGrannyMemFileLoader);
	virtual CFileRequest* CreateRequest()	
	{ 
		const SResKey<SGrannyFileLoaderInfo> &k = GetKey();
		static const char *str;

		return CreateFileRequiest( k.tKey.szResName.c_str(), SIntResKey( k.uidKey, k.tKey.nID ), k.tKey.bAllowDelayedLoad ); 
	}
	//
	virtual void RecalcValue( CFileRequest *p );
};

class CGrannyMeshLoader : public CHoldedPtrFuncBase<CObjectInfo>
{
	typedef CHoldedPtrFuncBase<CObjectInfo> TParent;
	OBJECT_BASIC_METHODS(CGrannyMeshLoader);
	ZDATA
	CDGPtr<CGrannyMemFileLoader> pGrannyFile;
	SPartAndSkeletonKey key;
	CDGPtr<CPtrFuncBase<NAnimation::CGrannyFileInfo> > pSkeletonFileInfo;
	string sLightMapped;
	bool   bLightMapped;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGrannyFile); f.Add(3,&key); f.Add(4,&pSkeletonFileInfo); f.Add(5,&sLightMapped); f.Add(6,&bLightMapped); return 0; }
protected:
	bool NeedUpdate() 
	{ 
		TParent::NeedUpdate();
		if ( pSkeletonFileInfo )
			return pGrannyFile.Refresh() | pSkeletonFileInfo.Refresh();
		return pGrannyFile.Refresh(); 
	}
	
public:
	void Recalc();
	

	bool  IsLightMapped()
	{
		return bLightMapped;
	}
	const string &GetString()const
	{
		return sLightMapped;
	}
	CGrannyMeshLoader(){}
	
	const SPartAndSkeletonKey &GetKey()
	{
		return key;
	}

	void SetKey( const SPartAndSkeletonKey &_key  );
};


void ConvertAIGeomVerticesFromGranny( granny_mesh *pMesh, vector<CVec3> *pRes );
void ConvertAIGeomTrisFromGranny( granny_mesh *pMesh, vector<STriangle> *pRes );
void ConvertWeightsFromGranny( const granny_skeleton *pSkeleton, granny_mesh *pMesh, int nMaterialIndex, vector<SVertexWeight> *pWeights, int nVertices );
granny_model *FindFirstAppropriateModel( granny_file_info *pData, granny_mesh *pMesh );


const char * ConvertWeightsFromGrannyEx(
							  const granny_skeleton *pSkeleton, granny_mesh *pMesh, int nMaterialIndex,
							  vector<SVertexWeight> *pWeights, int nVertices );


void ConvertVerticesFromGranny( granny_mesh *pMesh, int nMaterialIndex, vector<NGScene::SVertex> *pVerts );

void ConvertGeometryFromGranny( granny_mesh *pMesh, int nMaterialIndex, vector<STriangle> *pGeometry );

bool EndsWith( const char *pszA, const char *pszB );

enum ELoadMode
{
	E_CACHED_LIGHTMAPS,
	E_PURE_GRANNY,
};

_3DMOTOR_EXPORT void SetLoadMode( ELoadMode eMode );



}	// namespace NGScene

#endif // __OBJECTINFO_H_

