#pragma once

#include "vendor/granny/include/granny.h"

#include "ED_Common_export.h"
#define INVALID_GRANNY_TYPEDEF_OFFSET (-1)

ED_COMMON_EXPORT int CalculateGrannyTypedefOffset( granny_data_type_definition *pType, const char *pName );
int CalculateGrannyMemberArraySize( granny_data_type_definition *pType, const char *pName );
ED_COMMON_EXPORT bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo ); 
ED_COMMON_EXPORT bool GetGrannyMeshBoundingBox( CVec3 *pvMin, CVec3 *pvMax, granny_file_info *pInfo, const std::string &szMeshName ); 
void GetVerticesFromGrannyMesh( granny_mesh *pMesh, std::vector<CVec3> *pVertexList );
void GetTrianglesFromGrannyMesh( granny_mesh *pMesh, std::vector<STriangle> *pTriangleList );
ED_COMMON_EXPORT int GetGrannyAnimationLength( granny_file_info *pInfo ); 

struct ED_COMMON_EXPORT SGrannyBoneAttributes
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
ED_COMMON_EXPORT bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, const std::string &rszFileName, const std::string &rszDesiredSkeletonName, bool bFromRoot );
ED_COMMON_EXPORT bool ReadAttributes( CGrannyBoneAttributesList *pBoneList, granny_file_info *pInfo, const std::string &rszDesiredSkeletonName, bool bFromRoot );

// ************************************************************************************************************************ //
// **
// ** granny file info guard. open granny file and retrieve file info. throw exception on error. automaticall close all
// **
// **
// **
// ************************************************************************************************************************ //

class ED_COMMON_EXPORT CGrannyFileInfoGuard
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


