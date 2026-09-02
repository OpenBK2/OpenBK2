#pragma once

#include "Misc/2Darray.h"

#include <cstdint>

template<class TEditParameters>
inline bool GetEditParameters( TEditParameters *pEditParameters, unsigned nCommandHandlerType )
{
	return SetGetEditParameters( reinterpret_cast<uintptr_t>( pEditParameters ), nCommandHandlerType, ID_GET_EDIT_PARAMETERS );
}

template<class TEditParameters>
inline bool SetEditParameters( const TEditParameters &rEditParameters, unsigned nCommandHandlerType )
{
	return SetGetEditParameters( reinterpret_cast<uintptr_t>( &rEditParameters ), nCommandHandlerType, ID_SET_EDIT_PARAMETERS );
}


#define NO_AFFECTED_TILE 0x7f

namespace NDb
{
	struct STerrain;
};

struct ITerraManager;
namespace NEditor
{
	extern const std::string SZ_TERRA_BIN_FILE_NAME;
	//
	std::string MakeMapPath( const CDBID &dbid );
	std::string GetTerrainBinFileName( const NDb::STerrain *pDesc );
	void CreateTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc );
	bool LoadTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc );
	bool SaveTerrain( ITerraManager *pTerraManager );
	void LoadBgMap( const std::string &szDesiredSeason, const std::string &rszMapInfoNameLoaded, CVec2 *pBgMapSize );
};

inline float ADiff( const float a, const float b )
{
	float r = a - b;
	if ( fabs(r) > FP_PI )
	{
		r += (FP_2PI * (r < 0.f ? 1.f : -1.f));
	}
	return r;
}


inline float AngleDiff( const CVec3 &v0, const CVec3 &v1 )
{
	return ADiff( atan2( v0.y, v0.x ), atan2( v1.y, v1.x ) );
}


inline CVec3 GetVecDir( float fDir )
{
	CQuat q( fDir, V3_AXIS_Z );
	CVec3 dir = V3_AXIS_X;
	q.Rotate( &dir, dir );
	return dir;
}


//
//		UNIT COMMAND TYPE INFO
//
// Описания строкового и числового значения для стартовых команд лежит в  файле AiActions.xml
// Кроме имени там для каждой команды указано, нужно ли для выполнения этой команды
// указывать target-unit ( 1 - указывать юнит, 0 - использовать target-position(Vec2) )

struct SUnitCommandTypeInfo
{
	std::string szName;
	int nValue;
	int nNeedTargetUnit;
	///
	SUnitCommandTypeInfo() : szName( "" ), nValue( -1 ), nNeedTargetUnit(0) {}
	///
	int operator&( IXmlSaver &saver );
};

bool LoadUnitCommandTypesFromXML( std::vector<SUnitCommandTypeInfo> *pCmdTypes );


bool GetPolyBoundingRect( float *pXmin, float *pYmin, float *pXmax, float *pYmax, const std::vector<CVec3> &rPoly );
CVec3 GetNearestTileCenter( const CVec3 &p );
CVec3 GetNearestVisTileCorner( const CVec3 &p );


struct SDoubleVec3
{
	double x;
	double y;
	double z;
};


inline bool Invert3x3Matrix( CArray2D<double> *pResult, const CArray2D<double> &rSource )
{
	if ( pResult == 0 )
	{
		return false;
	}
	( *pResult )[0][0] =  rSource[1][1] * rSource[2][2] - rSource[2][1] * rSource[1][2];
	( *pResult )[1][0] = -rSource[1][0] * rSource[2][2] + rSource[2][0] * rSource[1][2];
	( *pResult )[2][0] =  rSource[1][0] * rSource[2][1] - rSource[2][0] * rSource[1][1];
	//
	( *pResult )[0][1] = -rSource[0][1] * rSource[2][2] + rSource[2][1] * rSource[0][2];
	( *pResult )[1][1] =  rSource[0][0] * rSource[2][2] - rSource[2][0] * rSource[0][2];
	( *pResult )[2][1] = -rSource[0][0] * rSource[2][1] + rSource[2][0] * rSource[0][1];
	//
	( *pResult )[0][2] =  rSource[0][1] * rSource[1][2] - rSource[1][1] * rSource[0][2];
	( *pResult )[1][2] = -rSource[0][0] * rSource[1][2] + rSource[1][0] * rSource[0][2];
	( *pResult )[2][2] =  rSource[0][0] * rSource[1][1] - rSource[1][0] * rSource[0][1];
	// Division by determinant
	double fDeterminant = rSource[0][0] * ( *pResult )[0][0] +
												rSource[1][0] * ( *pResult )[0][1] +
												rSource[2][0] * ( *pResult )[0][2];
	if ( fDeterminant == 0.0f )
	{
		return false;
	}
	fDeterminant = 1.0f / fDeterminant;
	( *pResult )[0][0] *= fDeterminant;	( *pResult )[1][0] *= fDeterminant;	( *pResult )[2][0] *= fDeterminant;
	( *pResult )[0][1] *= fDeterminant;	( *pResult )[1][1] *= fDeterminant;	( *pResult )[2][1] *= fDeterminant;
	( *pResult )[0][2] *= fDeterminant;	( *pResult )[1][2] *= fDeterminant;	( *pResult )[2][2] *= fDeterminant;
	return true;
}


template<class TVec3>
void Multiply3x3Matrix( TVec3 *pResult, const CArray2D<double> &rSource, const TVec3 &rC )
{
	if ( pResult == 0 )
	{
		return;
	}
	pResult->x = rSource[0][0] * rC.x + rSource[0][1] * rC.y + rSource[0][2] * rC.z;
	pResult->y = rSource[1][0] * rC.x + rSource[1][1] * rC.y + rSource[1][2] * rC.z;
	pResult->z = rSource[2][0] * rC.x + rSource[2][1] * rC.y + rSource[2][2] * rC.z;
}



