#include "GrannyGltf.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace NGrannyGltf
{
namespace
{
	using V2 = std::array<float, 2>;
	using V3 = std::array<float, 3>;
	using V4 = std::array<float, 4>;
	using M4 = std::array<float, 16>;
	constexpr double FramesPerSecond = 30.0;

	void Require( bool condition, const std::string &message )
	{
		if ( !condition )
		{
			throw std::runtime_error( message );
		}
	}

	float Finite( float value )
	{
		Require( std::isfinite( value ), "GR2 contains a non-finite transform or vertex" );
		return value;
	}

	std::string Number( double value )
	{
		// Metadata is JSON even when the editor runs with a decimal-comma locale.
		std::ostringstream text;
		text.imbue( std::locale::classic() );
		text << std::setprecision( std::numeric_limits<double>::max_digits10 ) << value;
		return text.str();
	}

	// Swap engine Z-up to glTF Y-up, with optional X mirroring. With mirrorX enabled
	// this is the inverse of the runtime's NGltf::ConvertPosition/ConvertRotation.
	V3 Vector( const float *value, bool mirrorX )
	{
		return { Finite( value[0] ) * ( mirrorX ? -1.f : 1.f ), Finite( value[2] ), Finite( value[1] ) };
	}

	fastgltf::TRS Transform( const granny_transform &source, bool mirrorX )
	{
		fastgltf::TRS result;
		if ( source.Flags & GrannyHasPosition )
		{
			const auto position = Vector( source.Position, mirrorX );
			result.translation = { position[0], position[1], position[2] };
		}
		if ( source.Flags & GrannyHasOrientation )
		{
			float length = 0;
			for ( float value : source.Orientation )
			{
				length += Finite( value ) * value;
			}
			Require( length > 1e-12f, "GR2 contains a zero rotation quaternion" );
			const float scale = 1 / std::sqrt( length );
			// Conjugate every local rotation by the same reflection as the mesh.
			// Applied to bind poses AND sampled keys, this preserves skin deformation.
			const float yzSign = mirrorX ? 1.f : -1.f;
			result.rotation = fastgltf::math::fquat( -source.Orientation[0] * scale,
			                                         yzSign * source.Orientation[2] * scale,
			                                         yzSign * source.Orientation[1] * scale,
			                                         source.Orientation[3] * scale );
		}
		if ( source.Flags & GrannyHasScaleShear )
		{
			// glTF animated nodes cannot represent shear. Refuse it explicitly instead
			// of producing a plausible but incorrectly deformed model.
			for ( int row = 0; row < 3; ++row )
			{
				for ( int col = 0; col < 3; ++col )
				{
					Require( row == col || std::abs( Finite( source.ScaleShear[row][col] ) ) < 1e-5f,
					         "GR2 contains bone shear, which glTF TRS animation cannot represent" );
				}
			}
			result.scale = {
				Finite( source.ScaleShear[0][0] ), Finite( source.ScaleShear[2][2] ), Finite( source.ScaleShear[1][1] ) };
		}
		return result;
	}

	template <class T>
	size_t Accessor( SDocument *doc,
	                 const std::vector<T> &values,
	                 fastgltf::AccessorType type,
	                 fastgltf::ComponentType component )
	{
		Require( !values.empty(), "Cannot export an empty GR2 attribute" );
		fastgltf::Accessor accessor;
		accessor.bufferViewIndex = doc->AddBytes( values.data(), values.size() * sizeof( T ) );
		accessor.count = values.size();
		accessor.type = type;
		accessor.componentType = component;
		doc->asset.accessors.push_back( std::move( accessor ) );
		return doc->asset.accessors.size() - 1;
	}

	template <size_t N>
	size_t Floats( SDocument *doc, const std::vector<std::array<float, N>> &values, fastgltf::AccessorType type )
	{
		return Accessor( doc, values, type, fastgltf::ComponentType::Float );
	}

	void Bounds( fastgltf::Accessor *accessor, const std::vector<V3> &values )
	{
		auto minimum = values.front(), maximum = minimum;
		for ( const auto &value : values )
		{
			for ( size_t i = 0; i < 3; ++i )
			{
				minimum[i] = std::min( minimum[i], value[i] );
				maximum[i] = std::max( maximum[i], value[i] );
			}
		}
		accessor->min = fastgltf::AccessorBoundsArray::ForType<double>( 3 );
		accessor->max = fastgltf::AccessorBoundsArray::ForType<double>( 3 );
		for ( size_t i = 0; i < 3; ++i )
		{
			accessor->min->set<double>( i, minimum[i] );
			accessor->max->set<double>( i, maximum[i] );
		}
	}

	struct SMember
	{
		int offset = -1;
		int width = 0;
		granny_member_type type = GrannyEndMember;
	};

	SMember Member( const granny_vertex_data &vertices, const char *name )
	{
		int offset = 0;
		for ( const auto *member = vertices.VertexType; member && member->Type != GrannyEndMember; ++member )
		{
			if ( member->Name && std::strcmp( member->Name, name ) == 0 )
			{
				return { offset, std::max( 1, member->ArrayWidth ), member->Type };
			}
			const int size = GrannyGetMemberTypeSize( member );
			Require( size > 0 && offset <= std::numeric_limits<int>::max() - size, "Invalid GR2 vertex type" );
			offset += size;
		}
		return {};
	}

	template <size_t N>
	std::array<float, N> ReadFloat( const unsigned char *vertex, SMember member )
	{
		Require( member.offset >= 0 && member.type == GrannyReal32Member && member.width >= N,
		         "Unsupported GR2 vertex attribute type" );
		std::array<float, N> result;
		std::memcpy( result.data(), vertex + member.offset, sizeof( result ) );
		for ( float &value : result )
		{
			value = Finite( value );
		}
		return result;
	}

	size_t Node( SDocument *doc, const std::string &name )
	{
		fastgltf::Node node;
		node.name = name;
		doc->asset.nodes.push_back( std::move( node ) );
		return doc->asset.nodes.size() - 1;
	}

	std::vector<size_t> Skeleton( const granny_skeleton *skeleton, SDocument *doc, bool mirrorX )
	{
		std::vector<size_t> roots;
		if ( !skeleton )
		{
			return roots;
		}
		Require( skeleton->BoneCount > 0 && skeleton->BoneCount <= 65535 && skeleton->Bones,
		         "GR2 skeleton has an invalid bone count" );
		fastgltf::Skin skin;
		skin.name = "Skeleton";
		std::vector<M4> inverseBind;
		std::map<std::string, int> names;
		for ( int i = 0; i < skeleton->BoneCount; ++i )
		{
			const auto &bone = skeleton->Bones[i];
			Require( bone.Name && names.emplace( bone.Name, i ).second,
			         "GR2 skeleton contains missing or duplicate bone names" );
			const size_t index = Node( doc, bone.Name );
			skin.joints.push_back( index );
			doc->asset.nodes[index].transform = Transform( bone.LocalTransform, mirrorX );
			M4 matrix;
			const int axes[] = { 0, 2, 1, 3 };
			// Granny stores the transpose of its row-major affine transform: the
			// flat array is already glTF column-major. Conjugate by the axis swap
			// and optional reflection, just like the local and animated transforms.
			for ( int col = 0; col < 4; ++col )
			{
				for ( int row = 0; row < 4; ++row )
				{
					matrix[col * 4 + row] = Finite( bone.InverseWorld4x4[axes[col]][axes[row]] ) *
					                        ( mirrorX && ( ( col == 0 ) != ( row == 0 ) ) ? -1.f : 1.f );
				}
			}
			inverseBind.push_back( matrix );
		}
		for ( int i = 0; i < skeleton->BoneCount; ++i )
		{
			const int parent = skeleton->Bones[i].ParentIndex;
			Require( parent >= -1 && parent < skeleton->BoneCount && parent != i, "Invalid GR2 bone parent" );
			// Detect cycles even in an out-of-order hierarchy.
			int ancestor = parent;
			for ( int depth = 0; ancestor != -1; ++depth )
			{
				Require( ancestor >= 0 && ancestor < skeleton->BoneCount && depth < skeleton->BoneCount,
				         "Cyclic or invalid GR2 skeleton hierarchy" );
				ancestor = skeleton->Bones[ancestor].ParentIndex;
			}
			if ( parent < 0 )
			{
				roots.push_back( skin.joints[i] );
			}
			else
			{
				doc->asset.nodes[skin.joints[parent]].children.push_back( skin.joints[i] );
			}
		}
		if ( roots.size() == 1 )
		{
			skin.skeleton = roots[0];
		}
		skin.inverseBindMatrices = Floats( doc, inverseBind, fastgltf::AccessorType::Mat4 );
		doc->asset.skins.push_back( std::move( skin ) );
		return roots;
	}

	size_t Mesh( const granny_mesh *source,
	             const granny_skeleton *skeleton,
	             bool skinned,
	             const std::vector<int> &materials,
	             SDocument *doc,
	             bool mirrorX,
	             size_t skinIndex = 0 )
	{
		Require( source && source->PrimaryVertexData && source->PrimaryTopology, "Missing GR2 mesh data" );
		const auto &vertices = *source->PrimaryVertexData;
		const auto &topology = *source->PrimaryTopology;
		Require( vertices.VertexCount > 0 && vertices.Vertices && vertices.VertexType, "Empty GR2 mesh" );
		const int stride = GrannyGetTotalObjectSize( vertices.VertexType );
		Require( stride > 0, "Invalid GR2 vertex stride" );
		const auto position = Member( vertices, GrannyVertexPositionName );
		const auto normal = Member( vertices, GrannyVertexNormalName );
		auto uv = Member( vertices, GrannyVertexTextureCoordinatesName );
		if ( uv.offset < 0 )
		{
			uv = Member( vertices, "TextureCoordinates0" );
		}
		const auto weightsMember = Member( vertices, GrannyVertexBoneWeightsName );
		const auto jointsMember = Member( vertices, GrannyVertexBoneIndicesName );
		const bool rigid = GrannyMeshIsRigid( source );
		std::vector<uint16_t> boneMap;
		if ( skinned )
		{
			Require( skeleton && source->BoneBindingCount > 0 && source->BoneBindings,
			         "Skinned GR2 mesh has no bone bindings" );
			for ( int i = 0; i < source->BoneBindingCount; ++i )
			{
				int bone = -1;
				Require( source->BoneBindings[i].BoneName &&
				             GrannyFindBoneByName( skeleton, source->BoneBindings[i].BoneName, &bone ),
				         std::string( "Mesh bone missing from model skeleton: " ) +
				             ( source->BoneBindings[i].BoneName ? source->BoneBindings[i].BoneName : "<unnamed>" ) );
				boneMap.push_back( static_cast<uint16_t>( bone ) );
			}
			if ( !rigid )
			{
				Require( weightsMember.offset >= 0 && jointsMember.offset >= 0 && weightsMember.width == jointsMember.width &&
				             weightsMember.width <= 4 &&
				             ( weightsMember.type == GrannyNormalUInt8Member || weightsMember.type == GrannyUInt8Member ) &&
				             jointsMember.type == GrannyUInt8Member,
				         "Unsupported GR2 skin weights" );
			}
		}
		std::vector<V3> positions, normals;
		std::vector<V2> texcoords;
		std::vector<V4> weights;
		std::vector<std::array<uint16_t, 4>> joints;
		for ( int i = 0; i < vertices.VertexCount; ++i )
		{
			const auto *vertex = vertices.Vertices + size_t( i ) * stride;
			positions.push_back( Vector( ReadFloat<3>( vertex, position ).data(), mirrorX ) );
			if ( normal.offset >= 0 )
			{
				auto n = Vector( ReadFloat<3>( vertex, normal ).data(), mirrorX );
				const float length = std::sqrt( n[0] * n[0] + n[1] * n[1] + n[2] * n[2] );
				Require( length > 1e-12f, "GR2 contains a zero vertex normal" );
				for ( float &value : n )
				{
					value /= length;
				}
				normals.push_back( n );
			}
			if ( uv.offset >= 0 )
			{
				texcoords.push_back( ReadFloat<2>( vertex, uv ) );
			}
			if ( skinned )
			{
				V4 weight = { 1, 0, 0, 0 };
				std::array<uint16_t, 4> joint = { boneMap[0], 0, 0, 0 };
				if ( !rigid )
				{
					weight = {};
					joint = {};
					float total = 0;
					for ( int k = 0; k < weightsMember.width; ++k )
					{
						const unsigned w = vertex[weightsMember.offset + k];
						if ( !w )
						{
							continue; // Unused slots may carry garbage bone indices.
						}
						const unsigned index = vertex[jointsMember.offset + k];
						Require( index < boneMap.size(), "GR2 skin index is outside its bone bindings" );
						joint[k] = boneMap[index];
						weight[k] = static_cast<float>( w );
						total += w;
					}
					Require( total > 0, "GR2 vertex has no skin weight" );
					for ( float &w : weight )
					{
						w /= total;
					}
				}
				weights.push_back( weight );
				joints.push_back( joint );
			}
		}
		fastgltf::Primitive base;
		const size_t positionAccessor = Floats( doc, positions, fastgltf::AccessorType::Vec3 );
		Bounds( &doc->asset.accessors[positionAccessor], positions );
		base.attributes.emplace_back( fastgltf::Attribute{ "POSITION", positionAccessor } );
		if ( !normals.empty() )
		{
			base.attributes.emplace_back(
					fastgltf::Attribute{ "NORMAL", Floats( doc, normals, fastgltf::AccessorType::Vec3 ) } );
		}
		if ( !texcoords.empty() )
		{
			base.attributes.emplace_back(
					fastgltf::Attribute{ "TEXCOORD_0", Floats( doc, texcoords, fastgltf::AccessorType::Vec2 ) } );
		}
		if ( skinned )
		{
			base.attributes.emplace_back(
					fastgltf::Attribute{ "WEIGHTS_0", Floats( doc, weights, fastgltf::AccessorType::Vec4 ) } );
			base.attributes.emplace_back( fastgltf::Attribute{
				"JOINTS_0", Accessor( doc, joints, fastgltf::AccessorType::Vec4, fastgltf::ComponentType::UnsignedShort ) } );
		}
		const int indexCount = GrannyGetMeshIndexCount( source );
		const int indexBytes = GrannyGetMeshBytesPerIndex( source );
		const auto *indices = static_cast<const unsigned char *>( GrannyGetMeshIndices( source ) );
		Require( indexCount > 0 && indexCount % 3 == 0 && indices && ( indexBytes == 2 || indexBytes == 4 ),
		         "Invalid GR2 triangle indices" );
		auto getIndex = [&]( size_t i )
		{
			uint32_t index = 0;
			std::memcpy( &index, indices + i * indexBytes, indexBytes );
			Require( index < positions.size(), "GR2 triangle refers outside the vertex array" );
			return index;
		};
		fastgltf::Mesh mesh;
		mesh.name = source->Name ? source->Name : "Mesh";
		Require( topology.GroupCount >= 0 && ( !topology.GroupCount || topology.Groups ), "Invalid GR2 material groups" );
		for ( int group = 0; group < std::max( 1, topology.GroupCount ); ++group )
		{
			const int first = topology.GroupCount ? topology.Groups[group].TriFirst : 0;
			const int count = topology.GroupCount ? topology.Groups[group].TriCount : indexCount / 3;
			Require( first >= 0 && count >= 0 && first <= indexCount / 3 && count <= indexCount / 3 - first,
			         "GR2 triangle group is outside the index array" );
			if ( count == 0 )
			{
				continue;
			}
			std::vector<uint32_t> triangles;
			for ( size_t i = size_t( first ) * 3; i < size_t( first + count ) * 3; i += 3 )
			{
				triangles.push_back( getIndex( i ) );
				// The extra reflection reverses winding a second time.
				triangles.push_back( getIndex( i + ( mirrorX ? 1 : 2 ) ) );
				triangles.push_back( getIndex( i + ( mirrorX ? 2 : 1 ) ) );
			}
			fastgltf::Primitive primitive;
			primitive.attributes = base.attributes;
			primitive.type = fastgltf::PrimitiveType::Triangles;
			primitive.indicesAccessor =
					Accessor( doc, triangles, fastgltf::AccessorType::Scalar, fastgltf::ComponentType::UnsignedInt );
			const int material = materials.empty() ? -1 : materials[std::min( size_t( group ), materials.size() - 1 )];
			if ( material >= 0 && size_t( material ) < doc->asset.materials.size() )
			{
				primitive.materialIndex = size_t( material );
			}
			mesh.primitives.push_back( std::move( primitive ) );
		}
		Require( !mesh.primitives.empty(), "GR2 mesh has no triangles" );
		const size_t node = Node( doc, std::string( mesh.name ) );
		doc->asset.nodes[node].meshIndex = doc->asset.meshes.size();
		if ( skinned )
		{
			doc->asset.nodes[node].skinIndex = skinIndex;
		}
		doc->asset.meshes.push_back( std::move( mesh ) );
		return node;
	}

	bool IsStaticCollisionBox( const SModel &source, const std::string &root )
	{
		if ( source.aiMeshes.size() != 1 )
		{
			return false;
		}
		const auto &part = source.aiMeshes.front();
		if ( !part.skeleton )
		{
			return true;
		}
		// Legacy AABBShape commonly has a separate one-bone rig. The box is already
		// in model space. Its node name retains the rigid binding used by the AI loader.
		if ( part.skeleton->BoneCount != 1 || !part.skeleton->Bones || !part.skeleton->Bones[0].Name ||
		     root != part.skeleton->Bones[0].Name || !part.mesh || part.mesh->BoneBindingCount != 1 ||
		     !part.mesh->BoneBindings || !part.mesh->BoneBindings[0].BoneName ||
		     root != part.mesh->BoneBindings[0].BoneName )
		{
			return false;
		}
		for ( const auto &clip : source.clips )
		{
			const auto &group = *clip.animation->TrackGroups[0];
			for ( int i = 0; i < group.TransformTrackCount; ++i )
			{
				if ( group.TransformTracks[i].Name && root == group.TransformTracks[i].Name )
				{
					return false;
				}
			}
		}
		return true;
	}

	void Animations( const SModel &source, const granny_skeleton *skeleton, SDocument *doc, size_t skinIndex = 0 )
	{
		if ( source.clips.empty() )
		{
			return;
		}
		Require( skeleton, "Cannot export animations without a skeleton" );
		granny_model model = {};
		model.Name = source.geometry->Name;
		model.Skeleton = const_cast<granny_skeleton *>( skeleton );
		GrannyMakeIdentity( &model.InitialPlacement );
		std::unique_ptr<granny_model_instance, decltype( &GrannyFreeModelInstance )> instance(
				GrannyInstantiateModel( &model ), GrannyFreeModelInstance );
		std::unique_ptr<granny_local_pose, decltype( &GrannyFreeLocalPose )> pose(
				GrannyNewLocalPose( skeleton->BoneCount ), GrannyFreeLocalPose );
		Require( instance && pose, "Cannot allocate GR2 animation sampler" );
		std::vector<std::vector<V3>> translations( skeleton->BoneCount ), scales( skeleton->BoneCount );
		std::vector<std::vector<V4>> rotations( skeleton->BoneCount );
		std::vector<float> times;
		std::string ranges = "{\"framesPerSecond\":30,\"clips\":[";
		int startFrame = 0;
		std::optional<size_t> markers;
		for ( size_t clipIndex = 0; clipIndex < source.clips.size(); ++clipIndex )
		{
			const auto &clip = source.clips[clipIndex];
			const auto *animation = clip.animation;
			Require( animation && std::isfinite( animation->Duration ) && animation->Duration >= 0 &&
			             animation->Duration <= 3600 && animation->TrackGroupCount > 0 && animation->TrackGroups &&
			             animation->TrackGroups[0],
			         "Invalid GR2 animation: " + clip.name );
			// Match CSkeletonAnimator: group zero, name-based binding, no accumulated
			// root movement. Untracked bones are reset by the sampler for every clip.
			auto *builder = GrannyBeginControlledAnimation( 0, animation );
			Require( builder != nullptr, "Cannot bind GR2 animation: " + clip.name );
			GrannySetTrackGroupTarget( builder, 0, instance.get() );
			GrannySetTrackGroupAccumulation( builder, 0, GrannyNoAccumulation );
			std::unique_ptr<granny_control, decltype( &GrannyFreeControl )> control( GrannyEndControlledAnimation( builder ),
			                                                                         GrannyFreeControl );
			Require( control != nullptr, "Cannot sample GR2 animation: " + clip.name );
			GrannySetControlLoopCount( control.get(), 1 );
			GrannySetControlForceClampedLooping( control.get(), true );
			GrannySetControlEaseIn( control.get(), false );
			GrannySetControlEaseOut( control.get(), false );
			// Marker ranges address a uniform frame clock. Sample the exact GR2 endpoints
			// onto the nearest whole-frame duration, including sub-frame and still clips.
			const int frames = std::max( 1, int( std::round( double( animation->Duration ) * FramesPerSecond ) ) );
			Require( startFrame < std::numeric_limits<int>::max() - frames - 1, "Animation timeline is too long" );
			const int endFrame = startFrame + frames;
			const double start = startFrame / FramesPerSecond;
			// The frame-slice importer needs three timestamps to identify a baked clock.
			const int lastSample = source.clips.size() == 1 ? std::max( frames, 2 ) : frames;
			for ( int frame = 0; frame <= lastSample; ++frame )
			{
				const float local = static_cast<float>( double( animation->Duration ) * std::min( frame, frames ) / frames );
				const float time = static_cast<float>( ( startFrame + frame ) / FramesPerSecond );
				Require( times.empty() || time > times.back(), "Animation timeline exceeds float time precision" );
				times.push_back( time );
				GrannySetControlRawLocalClock( control.get(), local );
				GrannySampleModelAnimations( instance.get(), 0, skeleton->BoneCount, pose.get() );
				for ( int bone = 0; bone < skeleton->BoneCount; ++bone )
				{
					const auto transform = Transform( *GrannyGetLocalPoseTransform( pose.get(), bone ), source.mirrorX );
					translations[bone].push_back(
							{ transform.translation[0], transform.translation[1], transform.translation[2] } );
					scales[bone].push_back( { transform.scale[0], transform.scale[1], transform.scale[2] } );
					V4 rotation = { transform.rotation[0], transform.rotation[1], transform.rotation[2], transform.rotation[3] };
					if ( !rotations[bone].empty() )
					{
						float dot = 0;
						for ( int k = 0; k < 4; ++k )
						{
							dot += rotation[k] * rotations[bone].back()[k];
						}
						if ( dot < 0 )
						{
							for ( float &value : rotation )
							{
								value = -value;
							}
						}
					}
					rotations[bone].push_back( rotation );
				}
			}
			if ( clipIndex )
			{
				ranges += ',';
			}
			ranges += "{\"name\":" + Quote( clip.name ) + ",\"firstFrame\":" + Number( startFrame ) +
			          ",\"lastFrame\":" + Number( endFrame ) + ",\"startSeconds\":" + Number( start ) +
			          ",\"endSeconds\":" + Number( endFrame / FramesPerSecond ) + "}";
			if ( skinIndex == 0 && source.includeAnimationsObject && !clip.markerName.empty() )
			{
				if ( !markers )
				{
					markers = Node( doc, "Animations" );
					doc->asset.scenes[0].nodeIndices.push_back( *markers );
				}
				const size_t marker = Node( doc, clip.markerName );
				doc->asset.nodes[*markers].children.push_back( marker );
				const int action =
						std::clamp( int( std::round( clip.actionMilliseconds * FramesPerSecond / 1000 ) ), 0, frames );
				std::string properties = "{\"StartTime\":" + std::to_string( startFrame ) +
				                         ",\"EndTime\":" + std::to_string( endFrame ) +
				                         ",\"ActionTime\":" + std::to_string( startFrame + action ) +
				                         ",\"Looped\":" + ( clip.looped ? "true" : "false" );
				if ( std::isfinite( clip.speed ) && clip.speed > 0 )
				{
					properties += ",\"Speed\":" + Number( clip.speed );
				}
				doc->extras[{ fastgltf::Category::Nodes, marker }] = properties + '}';
			}
			// Keep both endpoints, with a one-frame boundary before the next clip.
			startFrame += frames + 1;
		}
		ranges += "]}";
		const size_t input = Accessor( doc, times, fastgltf::AccessorType::Scalar, fastgltf::ComponentType::Float );
		doc->asset.accessors[input].updateBoundsToInclude( double( times.front() ) );
		doc->asset.accessors[input].updateBoundsToInclude( double( times.back() ) );
		fastgltf::Animation animation;
		animation.name = "AllAnimations";
		for ( int bone = 0; bone < skeleton->BoneCount; ++bone )
		{
			auto channel = [&]( size_t output, fastgltf::AnimationPath path )
			{
				animation.channels.push_back( { animation.samplers.size(), doc->asset.skins[skinIndex].joints[bone], path } );
				animation.samplers.push_back( { input, output, fastgltf::AnimationInterpolation::Linear } );
			};
			channel( Floats( doc, translations[bone], fastgltf::AccessorType::Vec3 ), fastgltf::AnimationPath::Translation );
			channel( Floats( doc, rotations[bone], fastgltf::AccessorType::Vec4 ), fastgltf::AnimationPath::Rotation );
			channel( Floats( doc, scales[bone], fastgltf::AccessorType::Vec3 ), fastgltf::AnimationPath::Scale );
		}
		doc->extras[{ fastgltf::Category::Animations, 0 }] = ranges;
		// Scene extras also survive importers which do not expose animation extras.
		doc->extras[{ fastgltf::Category::Scenes, 0 }] = ranges;
		if ( doc->asset.animations.empty() )
		{
			doc->asset.animations.push_back( std::move( animation ) );
		}
		else
		{
			// An AI mesh can carry a different skeleton. Sample it independently,
			// but put its channels on the same global clip and timeline.
			auto &target = doc->asset.animations[0];
			for ( auto &channel : animation.channels )
			{
				channel.samplerIndex += target.samplers.size();
				target.channels.push_back( channel );
			}
			for ( auto &sampler : animation.samplers )
			{
				target.samplers.push_back( std::move( sampler ) );
			}
		}
	}
} // namespace

std::string Quote( const std::string &text )
{
	std::string result = "\"";
	const char digits[] = "0123456789abcdef";
	for ( unsigned char c : text )
	{
		if ( c == '"' || c == '\\' )
		{
			result += '\\';
			result += char( c );
		}
		else if ( c < 32 )
		{
			result += "\\u00";
			result += digits[c >> 4];
			result += digits[c & 15];
		}
		else
		{
			result += char( c );
		}
	}
	return result + '"';
}

size_t SDocument::AddBytes( const void *data, size_t size )
{
	Require( size > 0 && data, "Cannot export empty binary data" );
	Require( size <= uint64_t( UINT32_MAX ) - 4 && binary.size() <= uint64_t( UINT32_MAX ) - 4 - size,
	         "Model exceeds the GLB 4 GB size limit" );
	while ( binary.size() % 4 )
	{
		binary.push_back( std::byte{ 0 } );
	}
	fastgltf::BufferView view;
	view.bufferIndex = 0;
	view.byteOffset = binary.size();
	view.byteLength = size;
	const auto *first = static_cast<const std::byte *>( data );
	binary.insert( binary.end(), first, first + size );
	asset.bufferViews.push_back( std::move( view ) );
	return asset.bufferViews.size() - 1;
}

size_t SDocument::AddImage( const std::string &name, const std::vector<std::byte> &bytes, fastgltf::MimeType mime )
{
	fastgltf::Image image;
	image.name = name;
	image.data = fastgltf::sources::BufferView{ AddBytes( bytes.data(), bytes.size() ), mime };
	asset.images.push_back( std::move( image ) );
	return asset.images.size() - 1;
}

std::vector<std::byte> SDocument::Finish()
{
	fastgltf::Buffer buffer;
	buffer.byteLength = binary.size();
	buffer.data = fastgltf::sources::Vector{ std::move( binary ), fastgltf::MimeType::None };
	asset.buffers.push_back( std::move( buffer ) );
	Require( fastgltf::validate( asset ) == fastgltf::Error::None, "Generated glTF failed validation" );
	fastgltf::Exporter exporter;
	exporter.setUserPointer( this );
	exporter.setExtrasWriteCallback(
			[]( size_t index, fastgltf::Category category, void *user ) -> std::optional<std::string>
			{
				const auto &values = static_cast<SDocument *>( user )->extras;
				const auto found = values.find( { category, index } );
				return found == values.end() ? std::nullopt : std::optional<std::string>( found->second );
			} );
	auto output = exporter.writeGltfBinary( asset );
	Require( output.error() == fastgltf::Error::None, "Cannot encode GLB" );
	return std::move( output.get().output );
}

void Convert( const SModel &source, SDocument *doc )
{
	Require( source.geometry && source.geometry->MeshBindingCount > 0 && source.geometry->MeshBindings,
	         "Geometry GR2 contains no model meshes" );
	doc->asset.assetInfo = fastgltf::AssetInfo{ "2.0", "", "OpenBK2 Granny3D export" };
	doc->asset.defaultScene = 0;
	const auto *skeleton = source.skeleton ? source.skeleton : source.geometry->Skeleton;
	const auto roots = Skeleton( skeleton, doc, source.mirrorX );
	const size_t geometry = Node( doc, source.rootMesh.empty() ? "Basis" : source.rootMesh );
	fastgltf::Scene scene;
	scene.name = source.name;
	// Blender parents skinned meshes to the armature containing their joints.
	// Make that armature the XDB mesh selector, with an identity transform.
	doc->asset.nodes[geometry].children.assign( roots.begin(), roots.end() );
	scene.nodeIndices.push_back( geometry );
	doc->asset.scenes.push_back( std::move( scene ) );
	size_t nextMaterial = 0;
	for ( int i = 0; i < source.geometry->MeshBindingCount; ++i )
	{
		std::vector<int> materials;
		if ( source.materialQuantities.empty() )
		{
			materials.push_back(
					doc->asset.materials.empty() ? -1 : int( std::min( size_t( i ), doc->asset.materials.size() - 1 ) ) );
		}
		else if ( size_t( i ) < source.materialQuantities.size() )
		{
			for ( int k = 0; k < source.materialQuantities[i]; ++k )
			{
				materials.push_back( int( nextMaterial++ ) );
			}
		}
		const auto *mesh = source.geometry->MeshBindings[i].Mesh;
		const bool animate =
				skeleton && mesh && mesh->BoneBindingCount > 0 &&
				( source.materialQuantities.empty() || size_t( i ) >= source.meshAnimated.size() || source.meshAnimated[i] );
		const size_t node = Mesh( mesh, skeleton, animate, materials, doc, source.mirrorX );
		doc->asset.nodes[geometry].children.push_back( node );
	}
	Animations( source, skeleton, doc );
	if ( !source.aiMeshes.empty() )
	{
		std::string root = source.aiRootMesh.empty() ? "AABB" : source.aiRootMesh;
		// Some GR2 pairs used the same selector in two different files. Give the
		// collision branch a distinct name now that both live in one scene.
		while ( std::any_of( doc->asset.nodes.begin(),
		                     doc->asset.nodes.end(),
		                     [&]( const fastgltf::Node &node ) { return std::string_view( node.name ) == root; } ) )
		{
			root += "_AI";
		}
		// Editors can show the hull in solid view, while material previews show the model.
		fastgltf::Material collision;
		collision.name = "Collision";
		collision.alphaMode = fastgltf::AlphaMode::Blend;
		collision.pbrData.baseColorFactor[3] = 0;
		const std::vector<int> material{ int( doc->asset.materials.size() ) };
		doc->asset.materials.push_back( std::move( collision ) );
		const bool staticBox = IsStaticCollisionBox( source, root );
		const size_t ai =
				staticBox ? Mesh( source.aiMeshes[0].mesh, nullptr, false, material, doc, source.mirrorX ) : Node( doc, root );
		doc->asset.nodes[ai].name = root;
		doc->asset.scenes[0].nodeIndices.push_back( ai );
		doc->extras[{ fastgltf::Category::Nodes, ai }] = "{\"role\":\"AIGeometry\"}";
		if ( staticBox )
		{
			return;
		}
		std::map<const granny_skeleton *, size_t> aiSkins;
		const auto *firstRig = source.aiMeshes.front().skeleton;
		const bool separateRigs =
				std::any_of( source.aiMeshes.begin(),
		                 source.aiMeshes.end(),
		                 [firstRig]( const SModel::SAIMesh &part ) { return part.skeleton != firstRig; } );
		for ( const auto &part : source.aiMeshes )
		{
			size_t skin = 0;
			if ( part.skeleton )
			{
				auto found = aiSkins.find( part.skeleton );
				if ( found == aiSkins.end() )
				{
					skin = doc->asset.skins.size();
					// Blender makes the parent of the root joints into an armature.
					// Give different GR2 models independent armatures in the AI branch.
					size_t armature = ai;
					if ( separateRigs )
					{
						armature = Node( doc, root + "_Rig" + std::to_string( aiSkins.size() + 1 ) );
						doc->asset.nodes[ai].children.push_back( armature );
					}
					for ( size_t joint : Skeleton( part.skeleton, doc, source.mirrorX ) )
					{
						doc->asset.nodes[armature].children.push_back( joint );
					}
					aiSkins.emplace( part.skeleton, skin );
					Animations( source, part.skeleton, doc, skin );
				}
				else
				{
					skin = found->second;
				}
			}
			const size_t node = Mesh( part.mesh,
			                          part.skeleton,
			                          part.skeleton && part.mesh && part.mesh->BoneBindingCount > 0,
			                          material,
			                          doc,
			                          source.mirrorX,
			                          skin );
			doc->asset.nodes[ai].children.push_back( node );
		}
	}
}
} // namespace NGrannyGltf
