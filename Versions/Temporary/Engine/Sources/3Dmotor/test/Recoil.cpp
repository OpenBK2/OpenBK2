// Drive the production recoil curve, animator and skinning matrices without a graphics device.
#include "3Dmotor/stdafx.h"

#include "3Dmotor/DBScene.h"
#include "3Dmotor/GAnimation.hpp"
#include "System/VFSOperations.h"

#include <cmath>
#include <tuple>

#include <fastgltf/core.hpp>
#include <gtest/gtest.h>

namespace
{
class CRecoilVFS : public NVFS::IVFS
{
	OBJECT_NOCOPY_METHODS( CRecoilVFS );

public:
	std::string path;
	std::string document;

	CDataStream *OpenFile( const std::string &name ) override
	{
		if ( name != path )
		{
			return nullptr;
		}
		auto *stream = new CMemoryStream;
		stream->Write( document.data(), document.size() );
		stream->Seek( 0 );
		return stream;
	}

	bool DoesFileExist( const std::string &name ) override
	{
		return name == path;
	}

	bool GetFileStats( NVFS::SFileStats *, const std::string & ) override
	{
		return false;
	}

	void GetAllFileNames( std::vector<std::string> *, const std::string & ) override {}
};

DEFINE_DG_CONSTANT_NODE( CRecoilTime, STime );

class Recoil : public testing::TestWithParam<std::tuple<bool, bool, float>>
{
	CObj<NVFS::IVFS> savedVFS = NVFS::GetMainVFS();
	CObj<CRecoilVFS> vfs = new CRecoilVFS;

protected:
	CObj<CRecoilTime> time = new CRecoilTime( 0 );
	CDGPtr<NAnimation::ISkeletonAnimator> animator;
	CDGPtr<NAnimation::ISkeletonAnimator> baseline;
	SHMatrix barrelRest, muzzleRest;
	CVec3 forward;
	CVec3 vertexRest;
	int barrelIndex = -1;

	void SetUp() override
	{
		const auto [skinned, nonuniform, scale] = GetParam();
		fastgltf::Asset asset;
		asset.nodes.resize( 4 );
		const char *names[] = { "Basis", "GunCarriage", "MainBarrel", "LMainGun" };
		for ( int i = 0; i < 4; ++i )
		{
			asset.nodes[i].name = names[i];
			if ( i < 3 )
			{
				asset.nodes[i].children.push_back( i + 1 );
			}
		}
		fastgltf::TRS root, carriage, barrel, muzzle;
		root.scale = { scale, scale, scale };
		carriage.translation = { 1, 2, 3 };
		carriage.rotation = fastgltf::math::fquat( 0.21603462f, 0.f, 0.f, 0.97638571f );
		carriage.scale = nonuniform ? fastgltf::math::fvec3( 2, 0.75f, 1.5f ) : fastgltf::math::fvec3( 1 );
		barrel.translation = { 0, 3, 4 };
		barrel.rotation = fastgltf::math::fquat( 0.f, 0.3f, 0.f, std::sqrt( 0.91f ) );
		barrel.scale = { 0.8f, 1.2f, 0.6f };
		muzzle.translation = { 0, 5, 6 };
		// As on the 3.7-inch gun, the muzzle's firing axis differs from the mesh's local Y.
		muzzle.rotation = fastgltf::math::fquat( 0.53764945f, 0.f, 0.f, 0.84316850f );
		muzzle.scale = { 11.797557f, 11.797557f, 11.797557f };
		asset.nodes[0].transform = root;
		asset.nodes[1].transform = carriage;
		asset.nodes[2].transform = barrel;
		asset.nodes[3].transform = muzzle;
		asset.scenes.emplace_back().nodeIndices.push_back( 0 );
		asset.defaultScene = 0;
		if ( skinned )
		{
			auto &skin = asset.skins.emplace_back();
			skin.joints = { 0, 1, 2, 3 };
			skin.skeleton = 0;
		}
		fastgltf::Exporter exporter;
		auto exported = exporter.writeGltfJson( asset );
		ASSERT_TRUE( exported );
		vfs->path = "recoil_" + std::to_string( skinned ) + "_" + std::to_string( nonuniform ) + "_" +
		            std::to_string( scale ) + ".gltf";
		vfs->document = exported.get().output;
		NVFS::SetMainVFS( vfs );
		CObj<NDb::SSkeleton> skeleton =
				dynamic_cast<NDb::SSkeleton *>( NObjectFactory::MakeObject( NDb::SSkeleton::typeID ) );
		ASSERT_TRUE( skeleton );
		skeleton->szModelFileRef = vfs->path;
		skeleton->szRootJoint = "Basis";
		animator = NAnimation::CreateSkeletonAnimator( NAnimation::SSkeletonHandle( skeleton, 0 ), time );
		ASSERT_TRUE( animator );
		baseline = NAnimation::CreateSkeletonAnimator( NAnimation::SSkeletonHandle( skeleton, 0 ), time );
		ASSERT_TRUE( baseline );
		SHMatrix placement( CVec3( 10, 20, 30 ), CQuat( 0.8f, V3_AXIS_Z ) );
		MultiplyScale( &placement, placement, 1.5f, 1.5f, 1.5f );
		animator->SetGlobalPosition( placement );
		baseline->SetGlobalPosition( placement );
		Advance( 0 );
		ASSERT_TRUE( animator->GetBonePosition( "MainBarrel", &barrelRest ) );
		ASSERT_TRUE( animator->GetBonePosition( "LMainGun", &muzzleRest ) );
		forward = CVec3( muzzleRest._13, muzzleRest._23, muzzleRest._33 );
		ASSERT_TRUE( Normalize( &forward ) );
		auto *bones = dynamic_cast<NAnimation::IGetBone *>( animator.GetPtr() );
		ASSERT_NE( bones, nullptr );
		barrelIndex = bones->GetBoneIndex( "MainBarrel" );
		ASSERT_GE( barrelIndex, 0 );
		// A point carried by the barrel must receive the same translation through skinning.
		animator->GetValue().compositePose[barrelIndex].RotateHVector( &vertexRest, CVec3( 2, 3, 4 ) );
	}

	void TearDown() override
	{
		NVFS::SetMainVFS( savedVFS );
	}

	void Advance( STime milliseconds )
	{
		time->Set( milliseconds );
		MarkNewDGFrame();
		animator.Refresh();
		baseline.Refresh();
	}

	void ExpectDisplacement( float distance )
	{
		CVec3 barrel, muzzle, vertex;
		ASSERT_TRUE( animator->GetBonePosition( "MainBarrel", &barrel ) );
		ASSERT_TRUE( animator->GetBonePosition( "LMainGun", &muzzle ) );
		animator->GetValue().compositePose[barrelIndex].RotateHVector( &vertex, CVec3( 2, 3, 4 ) );
		for ( const CVec3 &delta :
		      { barrel - barrelRest.GetTranslation(), muzzle - muzzleRest.GetTranslation(), vertex - vertexRest } )
		{
			EXPECT_NEAR( fabs( delta ), distance, 0.0002f );
			EXPECT_NEAR( delta * forward, -distance, 0.0002f );
			EXPECT_NEAR( fabs( delta ^ forward ), 0, 0.0002f );
		}
	}
};
} // namespace

TEST_P( Recoil, RetractsAlongFiringAxisByWorldDistanceAndReturns )
{
	ASSERT_TRUE( NAnimation::SetBoneRecoil( animator, "MainBarrel", muzzleRest, 0, 1.2f, 50, 400 ) );
	for ( STime ms : { 0u, 25u, 50u, 100u, 225u, 350u, 400u, 500u } )
	{
		SCOPED_TRACE( ms );
		Advance( ms );
		const float fraction = ms <= 50 ? ms / 50.0f : ( ms < 400 ? ( 400 - ms ) / 350.0f : 0 );
		ExpectDisplacement( 1.2f * fraction );
	}
}

TEST_P( Recoil, RepeatedShotStartsAtCurrentRetractionWithoutDrift )
{
	ASSERT_TRUE( NAnimation::SetBoneRecoil( animator, "MainBarrel", muzzleRest, 0, 1.2f, 50, 400 ) );
	Advance( 225 );
	ExpectDisplacement( 0.6f );
	SHMatrix muzzle;
	ASSERT_TRUE( animator->GetBonePosition( "LMainGun", &muzzle ) );
	ASSERT_TRUE( NAnimation::SetBoneRecoil( animator, "MainBarrel", muzzle, 225, 1.2f, 50, 400 ) );
	Advance( 250 );
	ExpectDisplacement( 0.9f );
	Advance( 275 );
	ExpectDisplacement( 1.2f );
	Advance( 625 );
	ExpectDisplacement( 0 );
}

TEST_P( Recoil, RejectsMissingBoneAndCollapsedFiringAxis )
{
	EXPECT_FALSE( NAnimation::SetBoneRecoil( animator, "missing", muzzleRest, 0, 1.2f, 50, 400 ) );
	SHMatrix collapsed = muzzleRest;
	collapsed._13 = collapsed._23 = collapsed._33 = 0;
	EXPECT_FALSE( NAnimation::SetBoneRecoil( animator, "MainBarrel", collapsed, 0, 1.2f, 50, 400 ) );
	Advance( 50 );
	ExpectDisplacement( 0 );
}

TEST_P( Recoil, ContinuesAlongBarrelAsCarriageTurns )
{
	ASSERT_TRUE( NAnimation::SetBoneRecoil( animator, "MainBarrel", muzzleRest, 0, 1.2f, 50, 400 ) );
	std::vector<NAnimation::ISkeletonAnimator::SDesiredBoneMove> turn;
	turn.emplace_back( 400, CQuat( 0.7f, V3_AXIS_X ) );
	animator->SetBoneMutator( "GunCarriage", 0, turn );
	baseline->SetBoneMutator( "GunCarriage", 0, turn );
	for ( STime ms : { 25u, 50u, 225u, 350u, 400u } )
	{
		SCOPED_TRACE( ms );
		Advance( ms );
		ASSERT_TRUE( baseline->GetBonePosition( "MainBarrel", &barrelRest ) );
		ASSERT_TRUE( baseline->GetBonePosition( "LMainGun", &muzzleRest ) );
		forward = CVec3( muzzleRest._13, muzzleRest._23, muzzleRest._33 );
		ASSERT_TRUE( Normalize( &forward ) );
		baseline->GetValue().compositePose[barrelIndex].RotateHVector( &vertexRest, CVec3( 2, 3, 4 ) );
		const float fraction = ms <= 50 ? ms / 50.0f : ( 400 - ms ) / 350.0f;
		ExpectDisplacement( 1.2f * fraction );
	}
}

INSTANTIATE_TEST_SUITE_P( HierarchiesAndScales,
                          Recoil,
                          testing::Combine( testing::Bool(),
                                            testing::Bool(),
                                            testing::Values( 0.0767018f, 1.0f, 3.0f ) ) );
