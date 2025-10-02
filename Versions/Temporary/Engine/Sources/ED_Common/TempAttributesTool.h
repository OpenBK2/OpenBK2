#pragma once

struct granny_file_info;
struct granny_bone;
interface IManipulator;
interface IExportTool;

namespace NMEGeomAttribs
{

IExportTool *GetOrCreateTempAttributesExportTool();
void DestroyTempAttributesExportTool();

granny_file_info *GetAttribsByVisObj( IManipulator *pMan );
granny_file_info *GetAttribsByModel( IManipulator *pMan );
granny_file_info *GetAttribsBySkeleton( IManipulator *pMan );
granny_file_info *GetAttribsByGeometry( IManipulator *pMan );
granny_file_info *GetAttribs( const string &szFileName, const string &szRootMesh, const string &szRootJoint );

// pDstData - pointer to destination structure with 'float' (can be just an array)
// pBone - source bone to extract attributes from
// ppszAttribNames - attribute names array
// nNumAttribs - number of attributes in array above
// NOTE: destination struct size must be at least nNumAttribs*4
void GetAttributesFromBone( void *pDstData, granny_bone *pBone, const char **ppszAttribNames, const int nNumAttribs );

};


