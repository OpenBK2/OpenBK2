#include "StdAfx.h"
#include "DBScene.h"
#include "MaterialReplacer.h"

namespace NGScene
{

#define N_MAX_DB_REPLACED_MATERIALS 9

struct SDefMaterials
{
	string szPrefix;
	int dbMaterials[N_MAX_DB_REPLACED_MATERIALS];
};

#define N_DEF_MATERIAL_GROUPS 3

const SDefMaterials defMaterials[N_DEF_MATERIAL_GROUPS] =
{
	{
		"Flag_Hero_Haven",
		{ 8863, 8861, 8856, 8857, 8858, 8859, 8860, 8862, 8864 }
	},
	{
		"Flag_Hero_Inferno",
		{ 13554, 13560, 13559, 13553, 13555, 13558, 13552, 13556, 13557 }
	},
	{
		"Flag",
		{ 7892, 7890, 7885, 7886, 7887, 7888, 7889, 7891, 7893 }
	}
};

const NDb::SMaterial *GetReplacedMaterial( const string &szName, int nInd )
{
	NI_ASSERT( nInd < N_MAX_DB_REPLACED_MATERIALS, "Index of replaced material is big" );
	if ( nInd < N_MAX_DB_REPLACED_MATERIALS )
	{
		for ( int k = 0; k < N_DEF_MATERIAL_GROUPS; ++k )
		{
			const SDefMaterials &defMat = defMaterials[k];
			if ( strncmp( szName.c_str(), defMat.szPrefix.c_str(), defMat.szPrefix.size() ) == 0 )
				return NDb::Get<NDb::SMaterial>( defMat.dbMaterials[nInd] );
		}
	}
	return 0;
}

}

