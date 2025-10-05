#include "stdafx.h"

#include "3DLib/ggeometry.h"
#include "System/Commands.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "ShotTrace.h"

namespace NShotTrace
{
	//static float s_fShotTraceLength = 200.0f;
	//static float s_fShotTraceSpeedCoeff = 0.5f;
	//static float s_fShotTraceThikness = 0.1f;
	static bool s_bDisableTracers = false;
}

void CShotTraceObj::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;

	NGScene::CObjectInfo::SData data;

	if ( NShotTrace::s_bDisableTracers )
	{
		data.verts.resize( 0 );
		data.geometry.resize( 0 );
		pValue->AssignFast( &data );
		return;
	}

	data.verts.resize( 4 );
	data.geometry.resize( 2 );

	const float fTimeCurr = pTimer->GetValue(); // current time

	float fAlpha1 = ( fTimeCurr - timeStart ) / fTimeTotal;
	float fAlpha2 = ( fTimeCurr - timeStart - fTimeQuant ) / fTimeTotal;
	if ( fAlpha2 < 0 )
		fAlpha2 = 0;

	const float fHead = ClampFast( fAlpha1, 0.0f, 1.0f );
	const float fTail = ClampFast( fAlpha2, 0.0f, 1.0f );

	const CVec3 vTraceHead = vStart + ( vEnd - vStart ) * fHead; // head of trace
	const CVec3 vTraceTail = vStart + ( vEnd - vStart ) * fTail; // tail of trace

	if ( WasCameraMoved() )
	{
		UpdateViewNormal();
	}

	data.verts[0].pos = vTraceTail - vViewNormal * fHalfWidth;
	data.verts[1].pos = vTraceHead - vViewNormal * fHalfWidth;
	data.verts[2].pos = vTraceTail + vViewNormal * fHalfWidth;
	data.verts[3].pos = vTraceHead + vViewNormal * fHalfWidth;

	data.verts[0].tex = CVec2( 1, 0 );
	data.verts[1].tex = CVec2( 0, 0 );
	data.verts[2].tex = CVec2( 1, 1 );
	data.verts[3].tex = CVec2( 0, 1 );

	//
	for ( std::vector<NGScene::SVertex>::iterator itVert = data.verts.begin(); itVert != data.verts.end(); ++itVert )
	{
		CalcCompactVector( &(itVert->normal), V3_AXIS_Z );
		itVert->normal.w = 255;	// absolutely opaque
	}
	//
	data.geometry[0].i1 = 0;
	data.geometry[0].i2 = 1;
	data.geometry[0].i3 = 3;
	data.geometry[1].i1 = 3;
	data.geometry[1].i2 = 2;
	data.geometry[1].i3 = 0;
	//
	pValue->AssignFast( &data );
}

START_REGISTER( ShotTrace )
	//REGISTER_VAR_EX( "shot_trace_lenght", NGlobal::VarFloatHandler, &NShotTrace::s_fShotTraceLength, 5.0f, STORAGE_NONE )
	//REGISTER_VAR_EX( "shot_trace_speed_coeff", NGlobal::VarFloatHandler, &NShotTrace::s_fShotTraceSpeedCoeff, 1.0f, STORAGE_NONE )
	//REGISTER_VAR_EX( "shot_trace_thikness", NGlobal::VarFloatHandler, &NShotTrace::s_fShotTraceThikness, 0.1f, STORAGE_NONE )
	REGISTER_VAR_EX( "shot_trace_disable", NGlobal::VarBoolHandler, &NShotTrace::s_bDisableTracers, false, STORAGE_NONE )
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x1B173C00, CShotTraceObj )


