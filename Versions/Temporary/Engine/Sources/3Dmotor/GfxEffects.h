#pragma once

namespace NGfx
{
class CGeometry;
class CTexture;
class CCubeTexture;
class CRenderContext;
//
const float F_FOG_HEIGHT = 25;
const float F_FOG_DISTANCE = 500;
const float F_POINT_FALLOFF = 6;

inline void InitRadius( CVec4 *p, float f )
{
	p->x = f;
	p->y = 1;
	p->z = 1 / f;
	p->w = sqr( F_POINT_FALLOFF ) * p->z * p->z;
}

inline void GetTexMapFromProjection( SHMatrix *p, int nResolution )
{
	p->SetX( p->x() * 0.5f + p->w() * ( 0.5f + 0.5f / nResolution ) );
	p->SetY( p->y() * ( -0.5f ) + p->w() * ( 0.5f + 0.5f / nResolution ) );
}

// EFFECTS

struct SEffect
{
	virtual void Use( CRenderContext *p ) = 0;
};

//! write color
struct SEffConstLight : public SEffect
{
	CVec4 color;
	//
	void Use( CRenderContext *p ) override;
};

struct SEffColoredTexture : public SEffect
{
	CTexture *pTex;
	CVec4 vColor;

	void Use( CRenderContext *p ) override;
};

struct SEffTnLParticles : public SEffect
{
	void Use( CRenderContext *p ) override;
};

struct SEffPureGeometry : public SEffect
{
	void Use( CRenderContext *p ) override;
};

struct SEffTransparentParticles : public SEffect
{
	CTexture *pLight;
	//
	void Use( CRenderContext *p ) override;
};

} // namespace NGfx
