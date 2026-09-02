#pragma once
#include "ED_Common_export.h"

struct granny_file_info;
struct granny_bone;
struct IManipulator;
struct IExportTool;

namespace NMEGeomAttribs
{

ED_COMMON_EXPORT IExportTool *GetOrCreateTempAttributesExportTool();
ED_COMMON_EXPORT void DestroyTempAttributesExportTool();

ED_COMMON_EXPORT granny_file_info *GetAttribsByVisObj( IManipulator *pMan );
ED_COMMON_EXPORT granny_file_info *GetAttribsByModel( IManipulator *pMan );
granny_file_info *GetAttribsBySkeleton( IManipulator *pMan );
ED_COMMON_EXPORT granny_file_info *GetAttribsByGeometry( IManipulator *pMan );
ED_COMMON_EXPORT granny_file_info *GetAttribs( const std::string &szFileName, const std::string &szRootMesh, const std::string &szRootJoint );

// pDstData - pointer to destination structure with 'float' (can be just an array)
// pBone - source bone to extract attributes from
// ppszAttribNames - attribute names array
// nNumAttribs - number of attributes in array above
// NOTE: destination struct size must be at least nNumAttribs*4
void GetAttributesFromBone( void *pDstData, granny_bone *pBone, const char **ppszAttribNames, const int nNumAttribs );

};


