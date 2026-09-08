#pragma once
#include "ED_Common_export.h"
#include "3Dmotor/GltfFormat.h"
#include "Tools_Granny.h"

struct IManipulator;
namespace NEditorGltf
{
// ModelFileRef takes precedence over the old exporter-only SrcName field.
ED_COMMON_EXPORT bool IsGltf( IManipulator *resource );
ED_COMMON_EXPORT NGltf::TGltfFilePtr Load( IManipulator *resource );
ED_COMMON_EXPORT bool Export( IManipulator *resource, const std::string &type, bool write );
ED_COMMON_EXPORT bool ReadAttributes( IManipulator *resource, CGrannyBoneAttributesList *attributes );
}
