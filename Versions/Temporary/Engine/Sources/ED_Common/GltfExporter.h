#pragma once
#include "ED_Common_export.h"
#include "3Dmotor/GltfFormat.h"
#include "Tools_Granny.h"
#include "Misc/ModelTriangle.h"

struct IManipulator;
namespace NEditorGltf
{
// ModelFileRef takes precedence over the old exporter-only SrcName field.
ED_COMMON_EXPORT bool IsGltfFileName( const std::string &path );
ED_COMMON_EXPORT bool IsGltf( IManipulator *resource );
// Validate the model graph before recursive exporters can change any resources.
ED_COMMON_EXPORT bool ValidateForExport( IManipulator *resource, const std::string &type );
ED_COMMON_EXPORT NGltf::TGltfFilePtr Load( IManipulator *resource );
ED_COMMON_EXPORT bool Export( IManipulator *resource, const std::string &type, bool write );
struct SMeshData
{
	std::vector<CVec3> vertices;
	std::vector<SModelTriangle> triangles;
	CVec3 minimum = VNULL3, maximum = VNULL3;
};
ED_COMMON_EXPORT bool LoadGeometry( IManipulator *resource, SMeshData *mesh );
ED_COMMON_EXPORT bool ReadAttributes( IManipulator *resource, CGrannyBoneAttributesList *attributes,
	const std::string &rootNode = std::string() );
}
