#include "stdafx.h"
#include "GltfExporter.h"
#include "GltfImport.h"
#include "3Dmotor/GltfAnimation.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/MessageBoxes.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <simdjson.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace NEditorGltf
{
namespace
{
namespace fs = std::filesystem;
std::string Value( IManipulator *resource, const char *field )
{
	std::string value;
	if ( resource && resource->IsNameExists(field) )
		CManipulatorManager::GetValue( &value, resource, field );
	return value;
}
std::string Source( IManipulator *resource )
{
	const auto *owner = resource->GetObjMan()->GetObject();
	const std::string ref = Value(resource, "ModelFileRef");
	// A newly selected model can still live in the configured source folder.
	// Never fall back to SrcName when an explicit ModelFileRef was supplied.
	const std::string source = ref.empty() ? Value(resource, "SrcName") : ref;
	for ( const std::string &path : {
		std::string(NGltf::ResolveModelFilePath(owner, source)),
		NFile::JoinPath(Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder, source) } )
	{
		if ( NVFS::GetMainVFS()->DoesFileExist(path) )
			return path;
	}
	return source;
}
std::vector<std::byte> Read( const std::string &path )
{
	CFileStream stream( NVFS::GetMainVFS(), path );
	if ( !stream.IsOk() || !stream.CanRead() || stream.GetSize() <= 0 )
		throw std::runtime_error("Cannot read " + path);
	std::vector<std::byte> bytes(stream.GetSize());
	stream.Read(bytes.data(), bytes.size());
	if ( !stream.IsOk() )
		throw std::runtime_error("Incomplete read of " + path);
	return bytes;
}

using TExtras = std::unordered_map<size_t, SGrannyBoneAttributes::CAttributeMap>;
void ParseExtras( simdjson::dom::object *object, size_t index, fastgltf::Category category, void *user )
{
	if ( category != fastgltf::Category::Nodes )
		return;
	auto &values = (*static_cast<TExtras *>(user))[index];
	for ( auto field : *object )
	{
		std::string key(field.key);
		NStr::ToLowerASCII(&key);
		double number;
		bool flag;
		if ( !field.value.get_double().get(number) && std::isfinite(number) )
			values[key] = static_cast<float>(number);
		else if ( !field.value.get_bool().get(flag) )
			values[key] = flag ? 1.0f : 0.0f;
	}
}
fastgltf::Asset Parse( const std::vector<std::byte> &bytes, TExtras *extras = nullptr )
{
	auto data = fastgltf::GltfDataBuffer::FromBytes(bytes.data(), bytes.size());
	if ( !data )
		throw std::runtime_error("Invalid GLB/GLTF input");
	fastgltf::Parser parser;
	if ( extras )
	{
		parser.setUserPointer(extras);
		parser.setExtrasParseCallback(ParseExtras);
	}
	// Include image URIs when collecting the package, although the game uses XDB materials.
	auto asset = parser.loadGltf(data.get(), fs::path(), fastgltf::Options::None);
	if ( !asset || fastgltf::validate(asset.get()) != fastgltf::Error::None )
		throw std::runtime_error("Invalid or unsupported GLB/GLTF document");
	return std::move(asset.get());
}
bool Below( const fs::path &path, const fs::path &root )
{
	const auto relative = path.lexically_relative(root);
	return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
}
std::string CopyPackage( const std::string &objectName, const NGltf::TGltfFilePtr &file )
{
	const std::string source = file->sourcePath;
	const auto bytes = Read(source);
	auto asset = Parse(bytes);
	const fs::path dataRoot = fs::absolute(fs::u8path(
		Singleton<IMODContainer>()->GetDataFolder(SUserData::NPT_EXPORT_DESTINATION))).lexically_normal();
	const fs::path sourcePath = fs::u8path(source).lexically_normal();
	fs::path relative = sourcePath;
	if ( sourcePath.is_absolute() )
	{
		if ( Below(sourcePath, dataRoot) )
			relative = sourcePath.lexically_relative(dataRoot);
		else
			relative = fs::u8path(objectName).parent_path() / sourcePath.filename();
	}
	const fs::path destination = (dataRoot / relative).lexically_normal();
	if ( !Below(destination, dataRoot) )
		throw std::runtime_error("GLTF destination is outside the current game/mod data folder");

	// Read and validate the complete package before writing any of its files.
	std::vector<std::pair<fs::path, std::vector<std::byte>>> files;
	files.emplace_back(destination, bytes);
	auto addDependency = [&]( const auto &data )
	{
		if ( const auto *uri = std::get_if<fastgltf::sources::URI>(&data) )
		{
			if ( !uri->uri.isLocalPath() )
				throw std::runtime_error("GLTF dependencies must use local relative paths");
			const auto local = fs::u8path(std::string(uri->uri.path()));
			if ( local.is_absolute() || local.has_root_name() )
				throw std::runtime_error("GLTF dependencies must use relative paths");
			const auto target = (destination.parent_path() / local).lexically_normal();
			if ( !Below(target, dataRoot) )
				throw std::runtime_error("GLTF dependency escapes the game/mod data folder");
			files.emplace_back(target, Read((sourcePath.parent_path() / local).generic_u8string()));
		}
	};
	for ( const auto &buffer : asset.buffers ) addDependency(buffer.data);
	for ( const auto &image : asset.images ) addDependency(image.data);
	for ( const auto &entry : files )
	{
		fs::create_directories(entry.first.parent_path());
		// Avoid rewriting in-place exports (and invalidating the shared model cache).
		std::ifstream existing(entry.first, std::ios::binary | std::ios::ate);
		if ( existing && existing.tellg() == static_cast<std::streamoff>(entry.second.size()) )
		{
			std::vector<std::byte> old(entry.second.size());
			existing.seekg(0);
			existing.read(reinterpret_cast<char *>(old.data()), old.size());
			if ( existing && old == entry.second )
				continue;
		}
		existing.close();
		// Use an adjacent temporary file so replacement is atomic on this volume.
		// A sibling name rather than GetTempFileName's: the export is
		// single-threaded and the point of the temporary is where it sits, not
		// that the name was handed out by the system.
		fs::path temporary = entry.first;
		temporary += ".tmp";
		try
		{
			std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
			output.write(reinterpret_cast<const char *>(entry.second.data()), entry.second.size());
			output.close();
			std::error_code ec;
			// rename replaces an existing destination, which is what
			// MoveFileEx was asked for with MOVEFILE_REPLACE_EXISTING.
			if ( !output )
				throw std::runtime_error("Cannot write " + entry.first.u8string());
			fs::rename(temporary, entry.first, ec);
			if ( ec )
				throw std::runtime_error("Cannot write " + entry.first.u8string());
		}
		catch ( ... ) { std::error_code ec; fs::remove(temporary, ec); throw; }
	}
	return destination.lexically_relative(dataRoot).generic_u8string();
}
}

bool ImportModelFile( const std::string &objectName, const std::string &source,
	std::string *reference, std::string *error )
{
	try
	{
		if ( !IsGltfFileName(source) ) throw std::runtime_error("Select a .glb or .gltf model.");
		const auto file = NGltf::LoadFile(nullptr, source);
		if ( !file ) throw std::runtime_error("Cannot load GLB/GLTF model: " + source);
		std::string normalizedName = objectName;
		NFile::NormalizePath(&normalizedName);
		*reference = CopyPackage(normalizedName, file);
		return true;
	}
	catch ( const std::exception &failure )
	{
		*error = failure.what();
		NLog::Log(LT_ERROR, "Model import failed: %s\n", error->c_str());
		return false;
	}
}

bool IsGltfFileName( const std::string &path )
{
	std::string extension = NFile::GetFileExt(path);
	NStr::ToLowerASCII(&extension);
	return extension == ".glb" || extension == ".gltf";
}
bool IsGltf( IManipulator *resource )
{
	if ( !resource ) return false;
	const std::string ref = Value(resource, "ModelFileRef");
	return IsGltfFileName(ref.empty() ? Value(resource, "SrcName") : ref);
}
NGltf::TGltfFilePtr Load( IManipulator *resource )
{
	return resource ? NGltf::LoadFile(nullptr, Source(resource)) : NGltf::TGltfFilePtr();
}

bool Export( IManipulator *resource, const std::string &type, bool write )
{
	try
	{
		const auto file = Load(resource);
		if ( !file ) throw std::runtime_error("Cannot load GLB/GLTF model (see engine log)");
		CVec3 minimum = VNULL3, maximum = VNULL3;
		std::vector<size_t> meshes;
		float duration = 0;
		if ( type == "Geometry" || type == "AIGeometry" )
		{
			const std::string root = Value(resource, "RootMesh");
			if ( !NGltf::GetMeshNodes(file, root, &meshes) || meshes.empty() ||
				!NGltf::GetMeshBoundingBox(file, root, type == "AIGeometry", &minimum, &maximum) )
				throw std::runtime_error("No valid geometry under RootMesh: " + root);
		}
		else if ( type == "Skeleton" )
		{
			NGltf::SSkeletonDefinition skeleton;
			if ( !NGltf::BuildSkeleton(file, Value(resource, "RootJoint"), 0, &skeleton) )
				throw std::runtime_error("Invalid GLTF skeleton or RootJoint");
		}
		else if ( type == "AnimB2" )
		{
			int first = 0, last = 0;
			CManipulatorManager::GetValue(&first, resource, "FirstFrame");
			CManipulatorManager::GetValue(&last, resource, "LastFrame");
			if ( !NAnimation::CGltfSkeletonAnimator::GetSourceDuration(
				file, Value(resource, "ClipName"), first, last, &duration) )
				throw std::runtime_error("Invalid animation clip or frame range");
		}
		else throw std::runtime_error("Unsupported GLTF resource type: " + type);

		if ( !write ) return true;
		const std::string destination = CopyPackage(NDb::GetFileName(resource->GetDBID()), file);
		// Publish the portable reference only after all validation and file writes succeed.
		if ( !CManipulatorManager::SetValue(destination, resource, "ModelFileRef") )
			return false;
		if ( type == "Geometry" )
			return CManipulatorManager::SetValue(static_cast<int>(meshes.size()), resource, "NumMeshes") &&
				CManipulatorManager::SetVec3((minimum + maximum) * 0.5f, resource, "Center") &&
				CManipulatorManager::SetVec3(maximum - minimum, resource, "Size");
		if ( type == "AIGeometry" )
			return CManipulatorManager::SetVec3((minimum + maximum) * 0.5f, resource, "AABBCenter") &&
				CManipulatorManager::SetVec3((maximum - minimum) * 0.5f, resource, "AABBHalfSize");
		if ( type == "AnimB2" )
		{
			const int milliseconds = static_cast<int>(duration * 1000.0f + 0.5f);
			int first = 0, last = 0, action = 0;
			CManipulatorManager::GetValue(&first, resource, "FirstFrame");
			CManipulatorManager::GetValue(&last, resource, "LastFrame");
			CManipulatorManager::GetValue(&action, resource, "ActionFrame");
			// ActionFrame is an offset from FirstFrame in the existing XDB schema.
			if ( last > first )
				CManipulatorManager::SetValue(Clamp(int(double(action) * milliseconds / (last - first)), 0, milliseconds), resource, "Action");
			return CManipulatorManager::SetValue(milliseconds, resource, "Length");
		}
		return true;
	}
	catch ( const std::exception &error )
	{
		NLog::Log(LT_ERROR, "GLTF export of %s failed: %s\n", NDb::GetFileName(resource->GetDBID()).c_str(), error.what());
		return false;
	}
}

namespace
{
bool ValidateModelSources( IManipulator *resource, const std::string &type,
	std::unordered_set<CDBID> *visited, std::string *error )
{
	if ( !visited->insert(resource->GetDBID()).second ) return true;
	if ( type == "Geometry" || type == "AIGeometry" || type == "Skeleton" || type == "AnimB2" )
	{
		if ( !NEditorGltf::IsGltf(resource) )
		{
			*error = "Model export accepts GLB/GLTF sources only. Existing GR2 assets can still be loaded.\n"
				"Set ModelFileRef to a .glb or .gltf file before exporting.\n\n" + NDb::GetFileName(resource->GetDBID());
			return false;
		}
		if ( !NEditorGltf::Export(resource, type, false) )
		{
			*error = "Cannot export the GLB/GLTF resource below. Check its source file and "
				"RootMesh, RootJoint or ClipName; details are in the log.\n\n" + NDb::GetFileName(resource->GetDBID());
			return false;
		}
	}
	CPtr<IManipulatorIterator> it = resource->Iterate(true, ECT_CACHE_GLOBAL);
	if ( !it ) return true;
	for ( ; !it->IsEnd(); it->Next() )
	{
		std::string name;
		it->GetName(&name);
		const auto *desc = dynamic_cast<const SPropertyDesc *>(resource->GetDesc(name));
		if ( !desc || desc->refTypes.empty() ) continue;
		std::string refType, refName;
		if ( !CManipulatorManager::GetParamsFromReference(name, resource, &refType, &refName, nullptr) ||
			refName.empty() ) continue;
		// Only the model/animation graph needs source conversion; textures and
		// gameplay references retain their own exporters and validation.
		if ( refType != "VisObj" && refType != "Model" && refType != "Geometry" &&
			refType != "AIGeometry" && refType != "Skeleton" && refType != "AnimB2" ) continue;
		CPtr<IManipulator> child = CManipulatorManager::CreateManipulatorFromReference(name, resource, 0, 0, 0);
		if ( !child )
		{
			*error = "Cannot load model resource: " + refName;
			return false;
		}
		if ( !ValidateModelSources(child, refType, visited, error) ) return false;
	}
	return true;
}
}

bool ValidateForExport( IManipulator *resource, const std::string &type )
{
	std::unordered_set<CDBID> visited;
	std::string error;
	if ( ValidateModelSources(resource, type, &visited, &error) ) return true;
	NLog::Log(LT_ERROR, "%s\n", error.c_str());
	NMessage::Error(error, "Model export");
	return false;
}

bool LoadGeometry( IManipulator *resource, SMeshData *mesh )
{
	try
	{
		const auto file = Load(resource);
		std::vector<size_t> nodes;
		SMeshData result;
		if ( !NGltf::GetMeshNodes(file, Value(resource, "RootMesh"), &nodes) ||
			!NGltf::GetMeshBoundingBox(file, Value(resource, "RootMesh"), false, &result.minimum, &result.maximum) )
			return false;
		for ( size_t index : nodes )
		{
			const auto &node = file->asset.nodes[index];
			for ( const auto &primitive : file->asset.meshes[*node.meshIndex].primitives )
			{
				const auto position = primitive.findAttribute("POSITION");
				if ( position == primitive.attributes.end() ) return false;
				const auto &positions = file->Vec3Accessor(position->accessorIndex);
				const size_t offset = result.vertices.size();
				for ( const auto &value : positions )
				{
					CVec3 point = NGltf::ConvertPosition(value);
					if ( !node.skinIndex ) file->nodeWorldTransforms[index].RotateHVector(&point, point);
					result.vertices.push_back(point);
				}
				std::vector<uint32_t> indices;
				if ( primitive.indicesAccessor )
					fastgltf::iterateAccessor<uint32_t>(file->asset, file->asset.accessors[*primitive.indicesAccessor],
						[&](uint32_t vertex) { indices.push_back(vertex); });
				else
					for ( size_t i = 0; i < positions.size(); ++i ) indices.push_back(static_cast<uint32_t>(i));
				auto triangle = [&](uint32_t a, uint32_t b, uint32_t c)
				{
					if ( a >= positions.size() || b >= positions.size() || c >= positions.size() ) return false;
					// Match the renderer's winding after the Y/Z coordinate swap.
					if ( a != b && b != c && a != c ) result.triangles.emplace_back(offset + a, offset + c, offset + b);
					return true;
				};
				if ( primitive.type == fastgltf::PrimitiveType::Triangles )
				{
					if ( indices.size() % 3 ) return false;
					for ( size_t i = 0; i + 2 < indices.size(); i += 3 )
						if ( !triangle(indices[i], indices[i+1], indices[i+2]) ) return false;
				}
				else if ( primitive.type == fastgltf::PrimitiveType::TriangleStrip )
				{
					for ( size_t i = 2; i < indices.size(); ++i )
						if ( !triangle(indices[i-2+(i%2)], indices[i-1-(i%2)], indices[i]) ) return false;
				}
				else if ( primitive.type == fastgltf::PrimitiveType::TriangleFan )
				{
					for ( size_t i = 2; i < indices.size(); ++i )
						if ( !triangle(indices[0], indices[i-1], indices[i]) ) return false;
				}
				else return false;
			}
		}
		if ( result.triangles.empty() ) return false;
		*mesh = std::move(result);
		return true;
	}
	catch ( const std::exception &error )
	{
		NLog::Log(LT_ERROR, "Cannot read GLTF triangles: %s\n", error.what());
		return false;
	}
}

bool ReadAttributes( IManipulator *resource, CGrannyBoneAttributesList *attributes, const std::string &rootNode )
{
	try
	{
		const auto file = Load(resource);
		if ( !file ) return false;
		TExtras extras;
		Parse(Read(file->sourcePath), &extras);
		attributes->clear();
		int root = -1;
		if ( !rootNode.empty() )
		{
			for ( size_t i = 0; i < file->asset.nodes.size(); ++i )
				if ( std::string(file->asset.nodes[i].name) == rootNode ) { root = static_cast<int>(i); break; }
			if ( root < 0 ) return false;
		}
		for ( size_t i = 0; i < file->asset.nodes.size(); ++i )
		{
			// Section stage locators belong to this subtree, not every section in the file.
			if ( root >= 0 )
			{
				int ancestor = static_cast<int>(i);
				while ( ancestor >= 0 && ancestor != root ) ancestor = file->nodeParents[ancestor];
				if ( ancestor < 0 ) continue;
			}
			SGrannyBoneAttributes entry;
			entry.szRealName = std::string(file->asset.nodes[i].name);
			entry.szBoneName = entry.szRealName;
			NStr::ToLowerASCII(&entry.szBoneName);
			entry.attributeMap = std::move(extras[i]);
			const CVec3 position = file->nodeWorldTransforms[i].GetTranslation();
			entry.attributeMap.emplace("translatex", position.x);
			entry.attributeMap.emplace("translatey", position.y);
			entry.attributeMap.emplace("translatez", position.z);
			// Lights use Euler attributes in the old exporter. Supply the converted
			// node rotation unless the artist explicitly authored those attributes.
			const auto transform = NGltf::MakeBoneTransform(file->nodeWorldTransforms[i]);
			CQuat rotation(CVec4(transform.Orientation[0], transform.Orientation[1],
				transform.Orientation[2], transform.Orientation[3]));
			CVec3 angles;
			rotation.DecompEulerAngles(&angles.z, &angles.y, &angles.x);
			entry.attributeMap.emplace("rotatex", angles.x);
			entry.attributeMap.emplace("rotatey", angles.y);
			entry.attributeMap.emplace("rotatez", angles.z);
			attributes->push_back(std::move(entry));
		}
		return true;
	}
	catch ( const std::exception &error )
	{
		NLog::Log(LT_ERROR, "Cannot read GLTF node attributes: %s\n", error.what());
		return false;
	}
}
}
