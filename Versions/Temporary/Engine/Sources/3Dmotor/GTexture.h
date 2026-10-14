#pragma once
#include "DBScene.h"
#include "System/GResource.h"

namespace NGfx
{
class CTexture;
class CCubeTexture;
} // namespace NGfx

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

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &pTexture );
		f.Add( 3, &nFlags );
		return 0;
	}

	STextureKey() {}

	STextureKey( const NDb::STexture *_pTexture, int _nFlags = 0 )
		: pTexture( _pTexture )
		, nFlags( _nFlags )
	{
	}

	bool operator==( const STextureKey &a ) const
	{
		return pTexture == a.pTexture && nFlags == a.nFlags;
	}
};

struct STextureKeyHash
{
	int operator()( const SResKey<STextureKey> &k ) const
	{
		return k.tKey.pTexture ? k.tKey.pTexture->GetDBID().GetHashKey() : 0;
	}
};

STextureKey GetKey( const NDb::STexture *pTex );

// texture loader from disk
class CFileTexture : public CResourceLoader<STextureKey, NGfx::CTexture>
{
	OBJECT_BASIC_METHODS( CFileTexture );
	typedef CResourceLoader<STextureKey, NGfx::CTexture> TParent;
	bool bIsFakeTexture;
	CObj<CFileRequest> pRequest;

protected:
	void Recalc() override;
	bool NeedUpdate() override;

public:
	CFileTexture()
		: bIsFakeTexture( false )
	{
	}

	void CreateChecker();

	void ReleaseTexture()
	{
		pValue = 0;
	}
};

class CFileCubeTexture : public CResourceLoader<CDBPtr<NDb::SCubeTexture>, NGfx::CCubeTexture>
{
	OBJECT_BASIC_METHODS( CFileCubeTexture );

protected:
	void Recalc() override;

public:
	void CreateChecker();
};

class CColorTexture : public CPtrFuncBase<NGfx::CTexture>
{
	OBJECT_BASIC_METHODS( CColorTexture );
	ZDATA
	CVec4 vColor;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &vColor );
		return 0;
	}

protected:
	void Recalc() override;

public:
	CColorTexture() {}

	CColorTexture( const CVec4 &_v )
		: vColor( _v )
	{
	}
};

} // namespace NGScene
