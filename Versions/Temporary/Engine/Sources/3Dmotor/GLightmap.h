#pragma once
#include "System/DG.h"
#include "Misc/2dArray.h"
#include "GPixelFormat.h"
#include "GLightmapLoader.h"

namespace NGfx
{
	struct SPixel8888;
	class CTexture;
}
namespace NCache
{
class CQuadTreeElement;
}
namespace NGScene
{
const int N_LM_TEXTURE_SIZE = 1024;

class CLightmapTextureCache;
class CSingleTexAlloc
{
	CObj<CLightmapTextureCache> pCache;
	vector<CObj<CObjectBase> > regions;
public:
	CSingleTexAlloc( int _nSize );
	bool AllocRegion( const CTPoint<int> &size, CTPoint<int> *pPos );
};

class CLMAlloc
{
	struct STex
	{
		CObj<CLightmapTextureCache> pCache;
		CArray2D<NGfx::SPixel8888> data;
	};
	vector<STex> textures;
	vector<CObj<CObjectBase> > regions;

	bool TryAlloc( const NCache::CQuadTreeElement &elem, int nTexture, const CArray2D<NGfx::SPixel8888> &_data, CTPoint<int> *pPos );
public:
	bool AllocRegion( const CArray2D<NGfx::SPixel8888> &_data, CTPoint<int> *pPos, int *pnTexture );
	int GetTexturesNum() const { return textures.size(); }
	const CArray2D<NGfx::SPixel8888> &GetTexture( int k ) const { return textures[k].data; }
};

class CLightmapTexture : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_NOCOPY_METHODS( CLightmapTexture );
	ZDATA
	CObj< NGScene::CLightmapsLoader> pLD;
	int nTex;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pLD);f.Add(3,&nTex); return 0; }
	void Recalc();
public:
	CLightmapTexture( CObj<CLightmapsLoader> _pLD, int _nTex): pLD(_pLD), nTex(_nTex){};
	CLightmapTexture() { }
	//CLightmapTexture( const CArray2D<NGfx::SPixel8888> &_tex ) : tex(_tex) {}
	//void SetData( const CArray2D<NGfx::SPixel8888> &_data ) { tex = _data; Updated(); }
};

class CObjectInfo;
class CLMGeometryGen : public CPtrFuncBase<CObjectInfo>
{
	OBJECT_NOCOPY_METHODS( CLMGeometryGen );
	ZDATA
	CDGPtr< CPtrFuncBase<CObjectInfo> > pSrc;
	bool bLMCalc;
	CTPoint<int> shift;
	float fLMResolution;
	bool bDump;

	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSrc); f.Add(3,&bLMCalc); f.Add(4,&shift); f.Add(5,&fLMResolution);f.Add(6,&bDump); return 0; }

	virtual bool NeedUpdate() { return pSrc.Refresh(); }
	virtual void Recalc();
public:
	CLMGeometryGen() : shift(0,0), bLMCalc(false) {}
	CLMGeometryGen( CPtrFuncBase<CObjectInfo> *p, const CTPoint<int> &_shift, float _fLMResolution, bool _bLMCalc, bool _bDump = false ) 
		: pSrc(p), shift(_shift), bLMCalc(_bLMCalc), fLMResolution(_fLMResolution), bDump(_bDump) {}
	CPtrFuncBase<CObjectInfo> *GetSrc() const { return pSrc; }
};
void EnableLMGeometryDump(bool flag);
bool GetDumpFlag();
void SetDumpDirectory(const string &szDirectoryName);
}

