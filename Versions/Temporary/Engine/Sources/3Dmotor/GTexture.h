#pragma once
#include "DBScene.h"
#include "GPixelFormat.h"
#include "Misc/2Darray.h"
#include "Misc/Geom.h"
#include "System/GResource.h"

#include "3Dmotor_export.h"

namespace NGfx
{
class CTexture;
class CCubeTexture;
enum ETextureUsage : int;
enum EWrap : int;
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

// A source of pictures that replace textures on disk, such as unit icons
// rendered at the resolution the screen needs (GameX's unit icon cache). A
// replacement is nScale times the size the texture record states, and is drawn
// in the same place at the same size: the 2D view maps the record's texel
// coordinates onto it (C2DGameView), so layout, which goes by the record's
// Width and Height, does not change.
class ITextureOverride
{
public:
	enum EState
	{
		// not a texture this source replaces; it loads from disk as usual
		OVERRIDE_NONE,
		// it will be replaced, but the picture is not ready yet; the texture on
		// disk is shown meanwhile and the source is asked again next frame
		OVERRIDE_PENDING,
		// *pImage holds the picture
		OVERRIDE_READY
	};

	virtual ~ITextureOverride() {}

	// Called on the render thread whenever the texture is (re)loaded and every
	// frame while it is pending, so it must be cheap when nothing changed
	virtual EState GetOverride( const NDb::STexture *pTexture, CArray2D<NGfx::SPixel8888> *pImage, int *pnScale ) = 0;
};

// The one source in use; 0 for none. The caller keeps it alive while it is set.
_3DMOTOR_EXPORT void SetTextureOverride( ITextureOverride *pOverride );

// texture loader from disk
class CFileTexture : public CResourceLoader<STextureKey, NGfx::CTexture>
{
	OBJECT_BASIC_METHODS( CFileTexture );
	typedef CResourceLoader<STextureKey, NGfx::CTexture> TParent;
	bool bIsFakeTexture;
	// an ITextureOverride picture is expected; ask again each frame
	bool bOverridePending;
	// Actual image texels per logical texel of the database record.
	CVec2 vTexelScale;
	CObj<CFileRequest> pRequest;

	bool LoadOverride( NGfx::ETextureUsage eUsage, NGfx::EWrap eWrap );

protected:
	void Recalc() override;
	bool NeedUpdate() override;

public:
	CFileTexture()
		: bIsFakeTexture( false )
		, bOverridePending( false )
		, vTexelScale( 1, 1 )
	{
	}

	void CreateChecker();

	void ReleaseTexture()
	{
		pValue = 0;
	}

	// Includes both high-resolution DDS files and ITextureOverride pictures.
	const CVec2 &GetTexelScale() const
	{
		return vTexelScale;
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
