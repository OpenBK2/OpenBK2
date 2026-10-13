#pragma once

namespace NGfx
{
	class CRenderContext;
}
namespace NGScene
{
struct SRTClearParams;
void ClearRT( NGfx::CRenderContext *pRC, const SRTClearParams &rtClear );
}
