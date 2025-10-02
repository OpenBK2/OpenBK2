#pragma once
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

