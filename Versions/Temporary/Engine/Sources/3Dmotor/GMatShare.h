#pragma once
#include "GScene.h"
namespace NGfx
{
	class CCubeTexture;
}
namespace NDb
{
	struct SMaterial;
}
namespace NGScene
{
const int N_MIN_FLOOR = -3;
const int N_MAX_FLOOR = 4;
inline int GetFloorMask( int nFloor ) { return ( 1 << (nFloor - N_MIN_FLOOR + 1) ) - 1; }
inline int GetParticlesRequireFlag( bool bShowParticles ) { return bShowParticles ? 0 : N_MASK_TREECROWN; }
inline int GetFloorBit( int nFloor, bool bShadowCast, bool bParticles, int nLODMask = 0 ) 
{ 
	return ( 1 << Max( nFloor - N_MIN_FLOOR, 0 ) ) | ( bParticles ? 0 : N_MASK_TREECROWN ) | ( bShadowCast ? N_MASK_CAST_SHADOW : 0 ) | nLODMask; 
}
class IMaterial;
struct SMaterialCreateInfo;

struct SColorHash
{
	int operator()( const CVec3 &color ) const { int *p = (int*)&color; return p[0]^p[1]^p[2]; }
};

class CSkyAdapter: public CPtrFuncBase<NGfx::CCubeTexture>
{
	OBJECT_BASIC_METHODS( CSkyAdapter );
	ZDATA
	CDGPtr<CPtrFuncBase<NGfx::CCubeTexture> > pTex;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pTex); return 0; }
	virtual bool NeedUpdate();
	virtual void Recalc();
public:
	void SetSource( CPtrFuncBase<NGfx::CCubeTexture> *_pTex );
	CPtrFuncBase<NGfx::CCubeTexture>* GetSource() const { return pTex; }
};

class CMaterialShare: public CObjectBase
{
	OBJECT_BASIC_METHODS( CMaterialShare );
	typedef std::unordered_map<CDBPtr<NDb::SMaterial>, CPtr<IMaterial>, SDBPtrHash > CMatHashmap;
	ZDATA
	CMatHashmap materials;
	CObj<CSkyAdapter> pSky;
	CObj<IMaterial> pOccluder;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&materials); f.Add(3,&pSky); f.Add(4,&pOccluder); return 0; }

public:
	CMaterialShare(): pSky( new CSkyAdapter ) {}
	void SetSky( CPtrFuncBase<NGfx::CCubeTexture> *_pSky ) { pSky->SetSource( _pSky ); }
	CPtrFuncBase<NGfx::CCubeTexture>* GetSky() const { return pSky->GetSource(); }
	CPtrFuncBase<NGfx::CCubeTexture>* GetSkyBinder() const { return pSky; }
	IMaterial* CreateMaterial( const NDb::SMaterial *pMaterial );
	void FillCreateMaterialInfo( const NDb::SMaterial *pMaterial, SMaterialCreateInfo *pRes );
};

class CColorMaterialShare
{
	typedef std::unordered_map<CVec3, CPtr<IMaterial>, SColorHash> CMatHashmap;
	CMatHashmap materials;
public:
	IMaterial* CreateMaterial( const CVec3 &color );
	int operator&( CStructureSaver &f )
	{
		f.Add( 1, &materials );
		return 0;
	}
};

struct STransparentHash
{
	int operator()( const CVec4 &color ) const { int *p = (int*)&color; return p[0]^p[1]^p[2]^p[3]; }
};

class CTransparentMaterialShare
{
	typedef std::unordered_map<CVec4, CPtr<IMaterial>, STransparentHash> CMatHashmap;
	CMatHashmap materials;
	CMatHashmap noShadowMaterials;
public:
	IMaterial* CreateMaterial( const CVec4 &color, bool bDoesCastShadow );
	int operator&( CStructureSaver &f )
	{
		f.Add( 1, &materials );
		f.Add( 2, &noShadowMaterials );
		return 0;
	}
};

}


