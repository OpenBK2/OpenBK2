#include "stdafx.h"
#include "GltfExporter.h"
#include "3Dmotor/GltfAnimation.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/Interface_Logger.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include <fastgltf/core.hpp>
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
	if ( !ref.empty() )
		return NGltf::ResolveModelFilePath(owner, ref);
	const std::string source = Value(resource, "SrcName");
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
std::string CopyPackage( IManipulator *resource, const NGltf::TGltfFilePtr &file )
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
			relative = fs::u8path(NDb::GetFileName(resource->GetDBID())).parent_path() / sourcePath.filename();
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
		wchar_t temporary[MAX_PATH];
		if ( !GetTempFileNameW(entry.first.parent_path().c_str(), L"glb", 0, temporary) )
			throw std::runtime_error("Cannot create temporary GLTF export file");
		try
		{
			std::ofstream output(fs::path(temporary), std::ios::binary | std::ios::trunc);
			output.write(reinterpret_cast<const char *>(entry.second.data()), entry.second.size());
			output.close();
			if ( !output || !MoveFileExW(temporary, entry.first.c_str(), MOVEFILE_REPLACE_EXISTING) )
				throw std::runtime_error("Cannot write " + entry.first.u8string());
		}
		catch ( ... ) { DeleteFileW(temporary); throw; }
	}
	return destination.lexically_relative(dataRoot).generic_u8string();
}
}

bool IsGltf( IManipulator *resource )
{
	if ( !resource ) return false;
	const std::string ref = Value(resource, "ModelFileRef");
	std::string extension = fs::u8path(ref.empty() ? Value(resource, "SrcName") : ref).extension().string();
	NStr::ToLowerASCII(&extension);
	return extension == ".glb" || extension == ".gltf";
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
		const std::string destination = CopyPackage(resource, file);
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

bool ReadAttributes( IManipulator *resource, CGrannyBoneAttributesList *attributes )
{
	try
	{
		const auto file = Load(resource);
		if ( !file ) return false;
		TExtras extras;
		Parse(Read(file->sourcePath), &extras);
		attributes->clear();
		for ( size_t i = 0; i < file->asset.nodes.size(); ++i )
		{
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
