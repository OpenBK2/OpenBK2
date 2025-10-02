#ifndef __GRenderFactor_H_
#define __GRenderFactor_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GRenderCore.h"
namespace NGfx
{
	struct SEffPointLight;
}
namespace NGScene
{
NGfx::CCubeTexture* GetNormalizeTexture();
NGfx::CTexture* GetSpecularResponse();
NGfx::CTexture* GetSpecularResponseR300();
NGfx::CTexture* GetUniformBump();
NGfx::CTexture* GetBlackTexture();
NGfx::CTexture* GetWhiteTexture();
NGfx::CTexture* GetDefaultLightmap();
NGfx::CTexture* GetLightFallLookup();
NGfx::CTexture* GetCheckerTexture();
NGfx::CTexture* Get16bitDepthLookup();

}
#endif
