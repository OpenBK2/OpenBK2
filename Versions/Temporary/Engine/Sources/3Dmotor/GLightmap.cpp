#include "StdAfx.h"
#include "GLightmap.h"
#include "GfxBuffers.h"
#include "../3dlib/GLMGeometry.h"
#include "Cache.h"
#include "../System/Commands.h"
#include "../System/BinaryResources.h"
#include "../System/GResource.h"
#include "../3dlib/GLMGeometry.h"
#include "GObjectInfo.h"
#include "GView.h"
#include "../System/VFSOperations.h"


namespace NGScene
{
hash_set<string>  objects;
static bool bChessLM = false;
static bool bDumpLM = false;
static string szIntDirectoryName;

void SetDumpDirectory(const string &szDirectoryName)
{
	objects.clear();
	szIntDirectoryName = szDirectoryName;
}

void EnableLMGeometryDump(bool flag)
{
	bDumpLM = flag;
}

bool GetDumpFlag()
{
	return  bDumpLM;
}

class CLightmapTextureCache;
class CLMRegion : 
	public NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CQuadTreeElement, CLMRegion>
{
	typedef NCache::CGatherElementBase<NCache::CShortPtrAllocator,NCache::CQuadTreeElement, CLMRegion> CParent;
	OBJECT_NOCOPY_METHODS(CLMRegion);
public:
	int operator&( CStructureSaver &f ) { ASSERT(0); return 0; }
};

class CLightmapTextureCache :
	public NCache::CGatheringCache<NCache::CShortPtrAllocator, NCache::CQuadTreeElement, CLMRegion>
{
	OBJECT_NOCOPY_METHODS( CLightmapTextureCache );
	typedef NCache::CGatheringCache<NCache::CShortPtrAllocator, NCache::CQuadTreeElement, CLMRegion> CParent;
	//CObj<NGfx::CTexture> pTexture;

	void InitCache( int nSize );
public:
	CLightmapTextureCache() {}
	CLightmapTextureCache( int nSize ) { InitCache( nSize ); }// RefreshTexture(); }
	int operator&( CStructureSaver &f ) { ASSERT(0); return 0; }
	friend class CLMRegion;
};

// CLightmapTextureCache

void CLightmapTextureCache::InitCache( int nSize )
{
	NCache::CQuadTreeElement root;
	root.nXSize = root.nYSize = GetMSB( nSize - 1 ) + 1;//N_LM_SIZE_LOG;
	root.nShiftX = root.nShiftY = 0;
	AddRoot( root );
}

// CSingleTexAlloc

CSingleTexAlloc::CSingleTexAlloc( int _nSize )
{
	pCache = new CLightmapTextureCache( _nSize );
}

bool CSingleTexAlloc::AllocRegion( const CTPoint<int> &size, CTPoint<int> *pPos )
{
	NCache::CQuadTreeElement elem;
	elem.nXSize = GetMSB( size.x - 1 ) + 1;
	elem.nYSize = GetMSB( size.y - 1 ) + 1;
	CLightmapTextureCache::SCachePlace place;
	if ( !pCache->GetPlace( elem, &place ) )
		return false;
	if ( place.nMRU != 0 )
		return false;
	CLMRegion *pRes = new CLMRegion;
	pCache->PerformAlloc( pRes, &place );
	regions.push_back( pRes );
	int nShiftX = place.resPlace.nShiftX;
	int nShiftY = place.resPlace.nShiftY;
	*pPos = CTPoint<int>( nShiftX, nShiftY );
	return true;
}

// CLMAlloc

bool CLMAlloc::TryAlloc( const NCache::CQuadTreeElement &elem, int nTexture, const CArray2D<NGfx::SPixel8888> &_data, CTPoint<int> *pPos )
{
	STex &tex = textures[nTexture];
	CLightmapTextureCache::SCachePlace place;
	if ( !tex.pCache->GetPlace( elem, &place ) )
		return false;
	if ( place.nMRU != 0 )
		return false;
	CLMRegion *pRes = new CLMRegion;
	tex.pCache->PerformAlloc( pRes, &place );
	regions.push_back( pRes );
	// copy data
	int nShiftX = place.resPlace.nShiftX;
	int nShiftY = place.resPlace.nShiftY;
	ASSERT( (1<<elem.nXSize) >= _data.GetSizeX() );
	ASSERT( (1<<elem.nYSize) >= _data.GetSizeY() );
	for ( int y = 0; y < _data.GetSizeY(); ++y )
	{
		for ( int x = 0; x < _data.GetSizeX(); ++x )
			tex.data[ y + nShiftY ][ x + nShiftX ] = _data[y][x];
	}
	// store result
	*pPos = CTPoint<int>( nShiftX, nShiftY );
	return true;
}

bool CLMAlloc::AllocRegion( const CArray2D<NGfx::SPixel8888> &_data, CTPoint<int> *pPos, int *pnTexture )
{
	NCache::CQuadTreeElement elem;
	elem.nXSize = GetMSB( _data.GetSizeX() - 1 ) + 1;
	elem.nYSize = GetMSB( _data.GetSizeY() - 1 ) + 1;
	for ( int k = 0; k < textures.size(); ++k )
	{
		if ( TryAlloc( elem, k, _data, pPos ) )
		{
			*pnTexture = k;
			return true;
		}
	}
	int nNew = textures.size();
	textures.resize( nNew + 1 );
	STex &tex = textures[ nNew ];
	tex.data.SetSizes( N_LM_TEXTURE_SIZE, N_LM_TEXTURE_SIZE );
	tex.data.FillEvery( NGfx::SPixel8888( 0, 0, 0, 0 ) );
	tex.pCache = new CLightmapTextureCache( N_LM_TEXTURE_SIZE );
	if ( TryAlloc( elem, nNew, _data, pPos ) )
	{
		*pnTexture = nNew;
		return true;
	}
	return false;
}

// CLightmapTexture

void CLightmapTexture::Recalc()
{
	const CArray2D< NGfx::SPixel8888>  & cTex = pLD.GetPtr()->GetTexture(nTex);

	pValue = NGfx::MakeTexture( cTex.GetSizeX(), cTex.GetSizeY(), 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> t( pValue, 0, NGfx::INPLACE );
	
	if ( bChessLM )
	{
		for ( int y = 0; y < t.GetSizeY(); ++y )
		{
			for ( int x = 0; x < t.GetSizeX(); ++x )
			{
				char c = ( ( x + y ) & 1 ) ? 255 : 0;
				//char c = tex[y][x].a;
				t[y][x] = NGfx::SPixel8888( c, c, c, 0xff );
			}
		}
	}
	else
	{
		for ( int y = 0; y < t.GetSizeY(); ++y )
		{
			for ( int x = 0; x < t.GetSizeX(); ++x )
			{
				//char c = ( ( x + y ) & 1 ) ? 255 : 0;
				//char c = tex[y][x].a;
				//t[y][x] = NGfx::SPixel8888( c, c, c, 0xff );
				/*NGfx::SPixel8888 c1 = cTex[y*2][x*2];
				NGfx::SPixel8888 c2 = cTex[y*2][x*2+1];
				NGfx::SPixel8888 c3 = cTex[y*2+1][x*2+1];
				NGfx::SPixel8888 c4 = cTex[y*2+1][x*2];
				*/

				NGfx::SPixel8888 c = cTex[y][x];

				c.a = 0xff;
				/*
				t[y][x].a=0xff;
				t[y][x].r=(c1.r+c2.r+c3.r+c4.r)/4;
				t[y][x].g=(c1.g+c2.g+c3.g+c4.g)/4;
				t[y][x].b=(c1.b+c2.b+c3.b+c4.b)/4;
				*/

				t[y][x] = c;

			}
		}
	}

	pLD.GetPtr()->ReleaseHint();
}

// CLMGeometryGen



extern CDGPtr<CGrannyMeshLoader> GetMeshLoader( const SPartAndSkeletonKey & key );

void CLMGeometryGen::Recalc()
{
	CObjectInfo *p = pSrc->GetValue();
	if ( !IsValid(p) )
	{
		pValue = 0;
		return;
	}
	
	pValue = new CObjectInfo;
	CObjectInfo::SData data, lmData;

	CTPoint<int> lmSize;
	CGrannyMeshLoader *pML=CDynamicCast<CGrannyMeshLoader>( pSrc.GetPtr() );
	
	if ( pML != 0 )
	{		
		SPartAndSkeletonKey key = pML->GetKey();
		key.nLightMapped = 0;
		CDGPtr<CGrannyMeshLoader> pML2 = GetMeshLoader( key );
		pML2.Refresh();		
		MakeSData( &data, *pML2->GetValue() );

		//ASSERT ( pML2->GetValue()->b

	}
	else
	{
		ASSERT ( 0 )
		MakeSData( &data, *p );
	}
	
	if ( bLMCalc )
	{

		MakeLMCalcGeometry( &lmData, &lmSize, data, fLMResolution, N_LM_TEXTURE_SIZE, shift, 0 );
		pValue->Assign( lmData, true );
	}
	else
	{
		MakeLMGeometry( &lmData, &lmSize, data, fLMResolution, N_LM_TEXTURE_SIZE, shift );
		string szDest = ( szIntDirectoryName + pML->GetString() );
		const char *str = szDest.c_str();
		pValue->Assign( lmData, true );
		if( bDumpLM )
		{
			if ( NVFS::GetMainFileCreator() )
			{
				OutputDebugString( str );
				OutputDebugString( "\n" );
				CFileStream stream( NVFS::GetMainFileCreator(), str );
				NGScene::CObjectInfo::SBinData sBinD;
				pValue->AssignTo( &sBinD );
				CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
				sBinD & *pSaver;
			}
		}
	}
}

START_REGISTER(GLightmap)
	REGISTER_VAR_EX( "gfx_lm_chess", NGlobal::VarBoolHandler, &bChessLM, 0, STORAGE_NONE )
FINISH_REGISTER
}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x201183C0, CLightmapTexture )
REGISTER_SAVELOAD_CLASS( 0x20119C00, CLMGeometryGen )
BASIC_REGISTER_CLASS( CLightmapTextureCache )

