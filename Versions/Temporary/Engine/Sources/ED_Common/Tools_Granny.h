#pragma once

#include "vendor/Granny/include/granny.h"

#define INVALID_GRANNY_TYPEDEF_OFFSET (-1)

int CalculateGrannyTypedefOffset( granny_data_type_definition *pType, const char *pName );
int CalculateGrannyMemberArraySize( granny_data_type_definition *pType, const char *pName );
bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo ); 
bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo, const string &szMeshName ); 
void GetVerticesFromGrannyMesh( granny_mesh *pMesh, vector<CVec3> *pVertexList );
void GetTrianglesFromGrannyMesh( granny_mesh *pMesh, vector<STriangle> *pTriangleList );
int GetGrannyAnimationLength( granny_file_info *pInfo ); 

struct SGrannyBoneAttributes
{
	typedef hash_map<string, float> CAttributeMap;
	//
	string szBoneName;
	string szRealName;					// Not forced to lowercase
	CAttributeMap attributeMap;
	//
	bool GetAttribute( const string &rszAttributeName, float *pfValue ) const;
	bool GetAttribute( const string &rszAttributeName, int *pnValue ) const;
	bool GetAttribute( const string &rszAttributeName, bool *pbValue ) const;
};
typedef vector<SGrannyBoneAttributes> CGrannyBoneAttributesList;
bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, const string &rszFileName, const string &rszDesiredSkeletonName, bool bFromRoot );
bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, granny_file_info *pInfo, const string &rszDesiredSkeletonName, bool bFromRoot );

// ************************************************************************************************************************ //
// **
// ** granny file info guard. open granny file and retrieve file info. throw exception on error. automaticall close all
// **
// **
// **
// ************************************************************************************************************************ //

class CGrannyFileInfoGuard
{
	granny_file *pFile;
	granny_file_info *pInfo;
public:
	CGrannyFileInfoGuard( const string &szFileName );
	~CGrannyFileInfoGuard();
	//
	granny_file_info *operator->() const { return pInfo; }
	granny_file_info &operator*() const { return *pInfo; }
	operator granny_file_info *() const { return pInfo; }
};


