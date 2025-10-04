#include "stdafx.h"
#include "GRenderUtils.h"
#include "GRenderModes.h"
#include "GfxRender.h"

namespace NGScene
{
	//void Clear( NGfx::CRenderContext *pRC, const CVec3 &vColor )
	//{
	//	NGfx::SPixel8888 clearColor( Float2Int( vColor.x * 255 ), Float2Int( vColor.y * 255 ), Float2Int( vColor.z * 255 ), 0xFF );
	//	if ( pRC->HasRegisters() )
	//	{
	//		pRC->SetRegister( 0 );
	//		pRC->ClearBuffers( clearColor.color );
	//		pRC->SetRegister( 1 );
	//		pRC->ClearTarget( 0 );
	//	}
	//	else
	//		pRC->ClearBuffers( clearColor.color );
	//}
void ClearRT( NGfx::CRenderContext *pRC, const SRTClearParams &rtClear )
{
	switch ( rtClear.ct )
	{
	case SRTClearParams::CT_NONE:
		break;
	case SRTClearParams::CT_ZBUFFER_ONLY:
		pRC->ClearZBuffer();
		break;
	case SRTClearParams::CT_FULL:
		pRC->ClearBuffers( NGfx::GetDWORDColor( rtClear.vColor ) );
		break;
	default:
		ASSERT(0);
		break;
	}
}
}

