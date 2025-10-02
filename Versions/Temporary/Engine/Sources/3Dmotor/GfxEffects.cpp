#include "StdAfx.h"
#include <D3D9.h>
#include "GfxEffects.h"
#include "GfxRender.h"
#include "GfxShaders.h"
#include "GfxBuffers.h"
#include "..\System\Commands.h"

namespace NGfx
{

// SEffConstLight

void SEffConstLight::Use( CRenderContext *p )
{
	if ( IsTnLDevice() )
	{
		p->SetPixelShader( psTFactor );
		p->SetVertexShader( NGfx::TNLVS_NONE );
		p->SetPSConst( 0, color );
	}
	else
	{
		p->SetPixelShader( psDiffuse );
		p->SetVertexShader( vsConstLight );
		p->SetVSConst( 16, color );
	}
}

// SEffColoredTexture

void SEffColoredTexture::Use( CRenderContext *p )
{
	p->SetPixelShader( psTextureTFactor );
	p->SetVertexShader( vsTexture );
	p->SetTexture( 0, pTex, FILTER_BEST );
	p->SetPSConst( 0, vColor );
}

// SEffTnLParticles

void SEffTnLParticles::Use( CRenderContext *p )
{
	ASSERT( IsTnLDevice() );
	p->SetPixelShader( psTransparentDifTex4 );
	p->SetVertexShader( NGfx::TNLVS_NONE );
	p->SetTexture( 0, GetTransparentTextureCache(), FILTER_BEST );
}

// SEffPureGeometry

void SEffPureGeometry::Use( CRenderContext *p )
{
	p->SetPixelShader( psDiffuse );
	p->SetVertexShader( vsPureGeometry );
}

// SEffTransparentParticles

bool bSimpleParticles = true;

// SEffTransparentParticles

void SEffTransparentParticles::Use( CRenderContext *p )
{
	//p->SetPixelShader( psTransparentTexTex4AlphaFirst );
	//p->SetVertexShader( vsTransparentMap );
	p->SetTexture( 0, NGfx::GetTransparentTextureCache(), FILTER_BEST );
	p->SetVertexShader( vsG3Particles );

	if( !bSimpleParticles )
	{
		p->SetPixelShader( psG3Particles );
		p->SetTexture( 1, pLight, FILTER_LINEAR );
	}
	else 
		p->SetPixelShader( psG3SimpleParticles );
}

/*
START_REGISTER(GfxEffects)
REGISTER_VAR_EX( "gfx_simple_particles", NGlobal::VarBoolHandler, &bSimpleParticles, true, STORAGE_USER )
FINISH_REGISTER
*/
}

