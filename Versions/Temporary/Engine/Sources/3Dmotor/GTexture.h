#ifndef __GTEXTURE_H__
#define __GTEXTURE_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../System/GResource.h"
#include "dbscene.h"

namespace NGfx
{
	class CTexture;
	class CCubeTexture;
}

namespace NGScene
{

struct STextureKey
{
	enum EEFlags
	{
		TK_WRAP_X = 1,
		TK_WRAP_Y = 2,
		TK_WRAP = 3,
		TK_TRANSPARENT = 4
	};

	ZDATA
	CDBPtr<NDb::STexture> pTexture;
	int nFlags;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTexture); f.Add(3,&nFlags); return 0; }

	STextureKey() {}
	STextureKey( const NDb::STexture *_pTexture, int _nFlags = 0 ): pTexture(_pTexture), nFlags(_nFlags) {}

	bool operator==( const STextureKey &a ) const { return pTexture == a.pTexture && nFlags == a.nFlags; }
};
struct STextureKeyHash
{
	int operator()( const SResKey<STextureKey> &k ) const { return k.tKey.pTexture ? k.tKey.pTexture->GetDBID().GetHashKey() : 0; }
};
STextureKey GetKey( const NDb::STexture *pTex );

// texture loader from disk
class CFileTexture : public CResourceLoader<STextureKey, NGfx::CTexture>
{
	OBJECT_BASIC_METHODS(CFileTexture);
	typedef CResourceLoader<STextureKey, NGfx::CTexture> TParent;
	bool bIsFakeTexture;
	CObj<CFileRequest> pRequest;
protected:
	virtual void Recalc();
	virtual bool NeedUpdate();
public:
	CFileTexture() : bIsFakeTexture(false) {}
	void CreateChecker();
	void ReleaseTexture() { pValue = 0; }
};

class CFileCubeTexture : public CResourceLoader<CDBPtr<NDb::SCubeTexture>, NGfx::CCubeTexture>
{
	OBJECT_BASIC_METHODS(CFileCubeTexture);
protected:
	virtual void Recalc();
public:
	void CreateChecker();
};

class CColorTexture : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_BASIC_METHODS(CColorTexture);
	ZDATA
	CVec4 vColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vColor); return 0; }
protected:
	virtual void Recalc();
public:
	CColorTexture() {}
	CColorTexture( const CVec4 &_v ) : vColor(_v) {}
};

} // namespace

#endif // __GTEXTURE_H__

