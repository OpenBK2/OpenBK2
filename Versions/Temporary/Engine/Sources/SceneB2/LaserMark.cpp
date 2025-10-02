#include "stdafx.h"

#include "LaserMark.h"
#include "System/Commands.h"

static float LASER_MARK_HALF_THIKNESS = 0.1f;
static float LASER_MARK_PULSAR_TIME = 0.2f;

void CLaserMarkTrace::Recalc()
{
	if ( HasZeroValue() )
		pValue = new NGScene::CObjectInfo;

	NGScene::CObjectInfo::SData data;
	data.verts.resize( 8 );
	data.geometry.resize( 4 );

	const float fTimeDiff = ( pTimer->GetValue() - timeStart )/1000.0f;
	const float fLaserSize = LASER_MARK_HALF_THIKNESS * ( 1.0f - fabs( 2.0f*fmod( fTimeDiff, LASER_MARK_PULSAR_TIME )/LASER_MARK_PULSAR_TIME - 1.0f ) );

	data.verts[0].pos = vStart - vNorm * fLaserSize;
	data.verts[1].pos = vEnd - vNorm * fLaserSize;
	data.verts[2].pos = vStart + vNorm * fLaserSize;
	data.verts[3].pos = vEnd + vNorm * fLaserSize;

	const CVec3 vZNorm( 0.0f, 0.0f, fLaserSize );
	data.verts[4].pos = vStart - vZNorm;
	data.verts[5].pos = vEnd - vZNorm;
	data.verts[6].pos = vStart + vZNorm;
	data.verts[7].pos = vEnd + vZNorm;

	data.verts[0].tex = CVec2( 0.0f, 0.0f );
	data.verts[1].tex = CVec2( 1.0f, 0.0f );
	data.verts[2].tex = CVec2( 0.0f, 1.0f );
	data.verts[3].tex = CVec2( 1.0f, 1.0f );

	data.verts[4].tex = CVec2( 0.0f, 0.0f );
	data.verts[5].tex = CVec2( 1.0f, 0.0f );
	data.verts[6].tex = CVec2( 0.0f, 1.0f );
	data.verts[7].tex = CVec2( 1.0f, 1.0f );

	NGfx::SCompactVector vCompactNormal;
	NGfx::CalcCompactVector( &vCompactNormal, CVec3(0, 0, 1) );
	for ( vector<NGScene::SVertex>::iterator it = data.verts.begin(); it != data.verts.end(); ++it )
		it->normal = vCompactNormal;


	data.geometry[0].i1 = 0;
	data.geometry[0].i2 = 1;
	data.geometry[0].i3 = 2;
	data.geometry[1].i1 = 1;
	data.geometry[1].i2 = 2;
	data.geometry[1].i3 = 3;

	data.geometry[2].i1 = 4;
	data.geometry[2].i2 = 5;
	data.geometry[2].i3 = 6;
	data.geometry[3].i1 = 5;
	data.geometry[3].i2 = 6;
	data.geometry[3].i3 = 7;

	pValue->AssignFast( &data );
}

void CLaserMarkTrace::UpdatePoints( const CVec3 &_vStart, const CVec3 &_vEnd )
{
	vStart = _vStart;
	vEnd = _vEnd;

	vNorm.x = vStart.y - vEnd.y;
	vNorm.y = vEnd.x - vStart.x;
	vNorm.z = 0.0f;
	::Normalize( &vNorm );

	bNeedUpdate = true;
}

START_REGISTER( LaserMark )
REGISTER_VAR_EX( "laser_mark_halt_thikness", NGlobal::VarFloatHandler, &LASER_MARK_HALF_THIKNESS, 0.1f, STORAGE_NONE )
REGISTER_VAR_EX( "laser_mark_pulsar_time", NGlobal::VarFloatHandler, &LASER_MARK_PULSAR_TIME, 0.2f, STORAGE_NONE )
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x311B8340, CLaserMarkTrace )


