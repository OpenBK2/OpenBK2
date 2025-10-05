#include "stdafx.h"
#include "GRTShare.h"
//#include "GfxUtils.h"
#include "GfxBuffers.h"
#include "Gfx.h"
//#include "RectLayout.h"
//#include "GRects.h"
//#include "..\3Dlib\Transform.h"
//#include "..\3Dlib\Bound.h"
#include "GRTInfo.h"

namespace NGScene
{

// CTexShare

template<class T> void CreateTex( CObj<T> *pRes, int nSize, int nPixelFormatID ) { ASSERT(0); }
template<>
static void CreateTex<NGfx::CTexture>( CObj<NGfx::CTexture> *pRes, int nSize, int nPixelFormatID )
{
	*pRes = NGfx::MakeTexture( nSize, nSize, 1, nPixelFormatID, NGfx::TARGET, NGfx::CLAMP );
}
template<>
static void CreateTex<NGfx::CCubeTexture>( CObj<NGfx::CCubeTexture> *pRes, int nSize, int nPixelFormatID )
{
	*pRes = NGfx::MakeCubeTexture( nSize, 1, nPixelFormatID, NGfx::TARGET );
}

template<class T>
class CTexShare
{
	struct STexInfo
	{
		CObj<T> pTex;
		int nResolution;
		int nPixelFormatID;
	};
	typedef std::unordered_map<std::string, STexInfo> CTexHash;
	CTexHash textureHash;

	void Refresh()
	{
		for ( CTexHash::iterator i = textureHash.begin(); i != textureHash.end(); ++i )
		{
			int nRes = i->second.nResolution;
			int nPixelFormatID = i->second.nPixelFormatID;
			//i->second.pTex = NGfx::MakeTexture( nRes, nRes, 1, nPixelFormatID, NGfx::TARGET, NGfx::CLAMP );
			CreateTex( &i->second.pTex, nRes, nPixelFormatID );
		}
	}
public:
	T *GetTexture( const std::string &sz )
	{
		CTexHash::iterator i = textureHash.find( sz );
		if ( i == textureHash.end() )
		{
			ASSERT(0);
			return 0;
		}
		if ( !IsValid( i->second.pTex ) )
		{
			Refresh();
			return GetTexture( sz );
		}
		return i->second.pTex;
	}
	void Init( const std::vector<SUserRTInfo::STex> &rtInfo, NGfx::SRenderTargetsInfo::CRTHash *pRes )
	{
		textureHash.clear();
		for ( int k = 0; k < rtInfo.size(); ++k )
		{
			const SUserRTInfo::STex &tex = rtInfo[k];
			STexInfo &texInfo = textureHash[ tex.szName ];
			texInfo.nResolution = tex.nResolution;
			texInfo.nPixelFormatID = tex.nPixelFormatID;
			texInfo.pTex = 0;
			NGfx::SRenderTargetsInfo::Add( pRes, tex.nResolution, tex.nPixelFormatID, 1 );
		}
	}
};
static CTexShare<NGfx::CTexture> texShare;
static CTexShare<NGfx::CCubeTexture> texCubeShare;

NGfx::CTexture *CRTPtr::GetTexture()
{
	if ( IsValid(pRes) )
		return pRes;
	pRes = texShare.GetTexture( szName );
	return pRes;
}

NGfx::CCubeTexture *CCubeRTPtr::GetTexture()
{
	if ( IsValid(pRes) )
		return pRes;
	pRes = texCubeShare.GetTexture( szName );
	return pRes;
}

void InitRTShare( const SUserRTInfo &rtInfo, NGfx::SRenderTargetsInfo *pRes )
{
	texShare.Init( rtInfo.tex, &pRes->targets );
	texCubeShare.Init( rtInfo.cubeTex, &pRes->cubeTargets );
}
}

