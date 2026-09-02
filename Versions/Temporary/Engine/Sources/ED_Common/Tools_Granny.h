#pragma once

#include "vendor/granny/include/granny.h"

#define INVALID_GRANNY_TYPEDEF_OFFSET (-1)

int CalculateGrannyTypedefOffset( granny_data_type_definition *pType, const char *pName );
int CalculateGrannyMemberArraySize( granny_data_type_definition *pType, const char *pName );
bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo ); 
bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo, const std::string &szMeshName ); 
void GetVerticesFromGrannyMesh( granny_mesh *pMesh, std::vector<CVec3> *pVertexList );
void GetTrianglesFromGrannyMesh( granny_mesh *pMesh, std::vector<STriangle> *pTriangleList );
int GetGrannyAnimationLength( granny_file_info *pInfo ); 

struct SGrannyBoneAttributes
{
	typedef std::unordered_map<std::string, float> CAttributeMap;
	//
	std::string szBoneName;
	std::string szRealName;					// Not forced to lowercase
	CAttributeMap attributeMap;
	//
	bool GetAttribute( const std::string &rszAttributeName, float *pfValue ) const;
	bool GetAttribute( const std::string &rszAttributeName, int *pnValue ) const;
	bool GetAttribute( const std::string &rszAttributeName, bool *pbValue ) const;
};
typedef std::vector<SGrannyBoneAttributes> CGrannyBoneAttributesList;
bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, const std::string &rszFileName, const std::string &rszDesiredSkeletonName, bool bFromRoot );
bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, granny_file_info *pInfo, const std::string &rszDesiredSkeletonName, bool bFromRoot );

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
	CGrannyFileInfoGuard( const std::string &szFileName );
	~CGrannyFileInfoGuard();
	//
	granny_file_info *operator->() const { return pInfo; }
	granny_file_info &operator*() const { return *pInfo; }
	operator granny_file_info *() const { return pInfo; }
};


