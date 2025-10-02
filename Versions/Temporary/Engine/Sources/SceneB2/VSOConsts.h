#pragma once
//


////	от этих констант надо будет аккуратно избавляться
////	по мере переделки TerraGen'a,
////	заменяя их пользовательскими настройками данных


//

////	CRAG

//#define DEF_CRAG_HIGH_BORDER_RAND 1.0f
//#define DEF_CRAG_HEIGHT 0.2f
//#define DEF_CRAG_HEIGHT_ERROR (DEF_TERRA_GEOM_OFFSET) //0.025f

//#define DEF_EXACT_EPS 0.001f
//

////	RIVER

//#define DEF_RIVER_SAMPLES_PER_PATCH 4

//#define DEF_RIVER_HIGH_BORDER_RAND 0.5f
//#define DEF_RIVER_WATER_LEVEL 2.0f
//#define DEF_BOTTOM_TEX 5.0f
//#define DEF_BOTTOM_INV_TEX (1.0f / DEF_BOTTOM_TEX)
//#define DEF_BOTTOM_SAMPLES_NUM 4
//#define DEF_BOTTOM_SAMPLES_COEFF ( 1.0f / ( DEF_BOTTOM_SAMPLES_NUM - 1 ) )
//#define DEF_WATER_SAMPLES_NUM 6

//#define DEF_WATER2_SAMPLES_NUM 4

//#define DEF_WATER_TEX_WORLD_SCALING 0.2f
//#define DEF_WATER_LAYER_HEIGHT 2.0f

//#define DEF_RIVER_RIDGE_WIDTH ( DEF_TILE_SIZE * 4 )
//#define DEF_RIVER_RIDGE_NULL ( DEF_TILE_SIZE * FP_SQRT_2 / DEF_RIVER_RIDGE_WIDTH )
//#define DEF_RIVER_RIDGE_COEFF ( 1.0f / ( 1.0f - DEF_RIVER_RIDGE_NULL ) )

//#define DEF_DEPTH_INTERPOLATE_LEN 5
//#define DEF_DEPTH_INTERPOLATE_COEFF ( 1.0f / ( DEF_DEPTH_INTERPOLATE_LEN - 1 ) )

//#define DEF_WATER_TEXTURE_SCALE ( 1.0f / 4.0f )

//#define DEF_WATER_EXPAND 0.25f
//#define DEF_WATER_SAMPLES_CALC_COEFF /*0.25f*/0.1f
//#define DEF_WATER2_TO_WATER_COEFF ( 2.0f / 3.0f )
//#define DEF_WATER_ALPHA_INIT 1
//#define DEF_WATER_ALPHA_FROM_HEIGHT_COEFF /*2.5f*/0.0f

//#define DEF_MAX_REFL_ALPHA_LEN ( DEF_TILE_SIZE * 3 )
//#define DEF_INV_MAX_REFL_ALPHA_LEN ( 1.0f / DEF_MAX_REFL_ALPHA_LEN )
//#define DEF_MIN_REFL_ALPHA_VAL 128
//#define DEF_REFL_ALPHA_RANGE ( 255 - DEF_MIN_REFL_ALPHA_VAL )
//
//#define DIF_RIVER_SAMPLES_MAX_COUNT	256
//

////	PRECIPICES

//#define DEF_PRECIPICE_SAMPLES_PER_PATCH 4

//#define DEF_PRECIPICE_SAMP_HEIGHT 1.0f
//#define DEF_PRECIPICE_DEPTH 0.15f
//#define DEF_PRECIPICE_DEPTH_RAND 0.6f
//#define DEF_PRECIPICE_RANDX 0.25f
//#define DEF_PRECIPICE_RANDY 0.65f

//#define DEF_PRECIPICE_SMOOTH_RAD ( DEF_TILE_SIZE * 5.0f )
//#define DEF_PRECIPICE_SMOOTH_RAD2 ( DEF_PRECIPICE_SMOOTH_RAD * DEF_PRECIPICE_SMOOTH_RAD )
//#define DEF_PRECIPICE_SMOOTH_RAD2_INV ( 1.0f / DEF_PRECIPICE_SMOOTH_RAD2 )
//#define DEF_STAYED_ON_TERRAIN_ERROR DEF_RIVER_DEPTH/*DEF_TILE_SIZE*/

//#define DEF_MIN_PRECIPICE_HEIGHT 0.025f
//

////	FEET

//#define DEF_FOOT_OFFSET	(DEF_CRAG_HOLE_WIDTH * 4.0f)
//#define DEF_FOOT_WIDTH_BASE 1.5f
//#define DEF_FOOT_WIDTH 0.3f
//#define DEF_FOOT_PARAGRAPH 2
//#define DEF_FOOT_PARAGRAPH_INV ( 1.0f / ( DEF_FOOT_PARAGRAPH - 1 ) )
//#define DEF_FOOT_PATCH_SIZE 64
//

////	ROADS

//#define DEF_TERRA_GEOM_OFFSET 0.1f	// not just roads
//#define DEF_ROAD_SAMPS_PER_PATCH 4
//#define DEF_ROAD_PATCH_VERTS_RESERVE 256
//#define DEF_ROAD_PATCH_TRGS_RESERVE 256

//#define DEF_ROAD_VERT_EPS 0.001f


inline int GetVSOSeed( const NDb::SVSOInstance *pInstance )
{
	NI_VERIFY( pInstance, "GetVSOSeed( pInstance ) is called with invalid pointer", return 0.0f )
	//NI_VERIFY( !(pInstance->controlPoints.empty()), "GetVSOSeed: VSO has no control points", return 0.0f )
	if ( pInstance->controlPoints.empty() )
		return 0.0f;

	const CVec3 v1( pInstance->controlPoints[0] );
	const CVec3 v2( pInstance->controlPoints[pInstance->controlPoints.size() - 1] );

	const float fLength = fabs( v1 - v2 );
	return (int)( fLength * 1000 );
}

