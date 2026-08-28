// Converting the animation side of a file: track groups, tracks and curves.
//
// The engine's gate on all of this is one line, GAnimation.cpp:432, which reads
// pFileInfo->Animations[n] and gives up if it is not there. Until this
// conversion existed every model in the game stood in its bind pose, not because
// anything was stubbed but because CSkeletonAnimator could never get far enough
// to call a stub.
//
// Three exporter vintages are in the corpus and they disagree about these
// structures, so the fixtures below are three files rather than one, each
// authored with the type tree the census found:
//
//   15,457 files  VectorTracks, TransformLODErrors, Oversampling, Dimension
//    5,948 files  ScalarTracks, no TransformLODErrors, no Oversampling
//      315 files  as above and no RootMotion either
//
// The one structure the corpus cannot check is the text track: no file in all
// 21,720 has a single one, so the only evidence for it is the type tree, which
// every file carries whether it uses it or not. That case is authored here
// precisely because nothing else covers it.
//
// Agreement with granny2.dll on real data is not here; it needs the DLL and the
// corpus, and lives in scripts/port/gr2diff.py. The commit that added this file
// records what that run reported.

#include "TypedGr2.h"

#include "Convert.h"
#include "File.h"
#include "Structures.h"

#include <gr2/granny.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2Test;

namespace
{

const NGr2::SFileInfo *Info( granny_file *pFile )
{
	return reinterpret_cast<const NGr2::SFileInfo *>( GrannyGetFileInfo( pFile ) );
}

//! Sizes on disk, where a pointer is four bytes whatever the host is.
constexpr uint32_t DISK_CURVE = 4 + 8 + 8;                     // Degree, Knots, Controls
constexpr uint32_t DISK_TRANSFORM_TRACK = 4 + DISK_CURVE * 3;  // Name, three curves
constexpr uint32_t DISK_TRANSFORM = 68;

//! What the three vintages call a track group's scalar tracks.
enum EVintage
{
	//! ScalarTracks, no TransformLODErrors, no Oversampling, no Dimension.
	VINTAGE_OLD,
	//! VectorTracks, TransformLODErrors, Oversampling, Dimension.
	VINTAGE_NEW,
};

//! An animation file with one track group and one transform track.
//!
//! Returns the file; nTrackOut receives where the transform track's object
//! starts, so a test can write curve data into it.
struct SBuilt
{
	CTypedFile file;
	uint32_t nTrackGroup = 0;
	uint32_t nTransformTrack = 0;
	uint32_t nAnimation = 0;
};

//! The curve type, which is the same shape in all three vintages.
uint32_t AddCurveType( CTypedFile &file, uint32_t nReal32Type )
{
	return file.AddType( {
		{ "Degree", T_INT32, 1, 0 },
		{ "Knots", T_REFERENCE_TO_ARRAY, 1, nReal32Type },
		{ "Controls", T_REFERENCE_TO_ARRAY, 1, nReal32Type },
	} );
}

//! Write a curve in place at nAt: its degree, and pointers to knots and controls.
void PutCurve( CTypedFile &file, uint32_t nAt, int32_t nDegree,
               const std::vector<float> &knots, const std::vector<float> &controls )
{
	file.PutI32( nAt, nDegree );
	file.PutI32( nAt + 4, static_cast<int32_t>( knots.size() ) );
	file.PutI32( nAt + 12, static_cast<int32_t>( controls.size() ) );

	if ( !knots.empty() )
	{
		const uint32_t nKnots =
			file.AddObject( static_cast<uint32_t>( knots.size() ) * 4 );
		for ( size_t i = 0; i < knots.size(); ++i )
		{
			file.PutReal32( nKnots + static_cast<uint32_t>( i ) * 4, knots[i] );
		}
		file.Point( nAt + 8, CTypedFile::OBJECTS, nKnots );
	}
	if ( !controls.empty() )
	{
		const uint32_t nControls =
			file.AddObject( static_cast<uint32_t>( controls.size() ) * 4 );
		for ( size_t i = 0; i < controls.size(); ++i )
		{
			file.PutReal32( nControls + static_cast<uint32_t>( i ) * 4, controls[i] );
		}
		file.Point( nAt + 16, CTypedFile::OBJECTS, nControls );
	}
}

//! A whole file: root, one animation, one track group, one transform track.
//!
//! The animation's TrackGroups points at the same object the root's does, which
//! is what every file in the corpus does and what the engine's pointer
//! comparisons depend on.
SBuilt BuildAnimationFile( EVintage vintage )
{
	SBuilt built;
	CTypedFile &file = built.file;

	const uint32_t nReal32Type = file.AddType( { { "Real32", T_REAL32, 1, 0 } } );
	const uint32_t nCurveType = AddCurveType( file, nReal32Type );

	const uint32_t nTransformTrackType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "PositionCurve", T_INLINE, 1, nCurveType },
		{ "OrientationCurve", T_INLINE, 1, nCurveType },
		{ "ScaleShearCurve", T_INLINE, 1, nCurveType },
	} );

	std::vector<CTypedFile::SMemberSpec> group;
	group.push_back( { "Name", T_STRING, 1, 0 } );
	if ( vintage == VINTAGE_NEW )
	{
		group.push_back( { "VectorTracks", T_REFERENCE_TO_ARRAY, 1, 0 } );
		group.push_back(
			{ "TransformTracks", T_REFERENCE_TO_ARRAY, 1, nTransformTrackType } );
		group.push_back( { "TransformLODErrors", T_REFERENCE_TO_ARRAY, 1, nReal32Type } );
	}
	else
	{
		group.push_back( { "ScalarTracks", T_REFERENCE_TO_ARRAY, 1, 0 } );
		group.push_back(
			{ "TransformTracks", T_REFERENCE_TO_ARRAY, 1, nTransformTrackType } );
	}
	group.push_back( { "TextTracks", T_REFERENCE_TO_ARRAY, 1, 0 } );
	group.push_back( { "InitialPlacement", T_TRANSFORM, 1, 0 } );
	group.push_back( { "AccumulationFlags", T_INT32, 1, 0 } );
	group.push_back( { "LoopTranslation", T_REAL32, 3, 0 } );
	group.push_back( { "PeriodicLoop", T_REFERENCE, 1, 0 } );
	group.push_back( { "RootMotion", T_REFERENCE, 1, 0 } );
	const uint32_t nGroupType = file.AddType( group );

	std::vector<CTypedFile::SMemberSpec> animation;
	animation.push_back( { "Name", T_STRING, 1, 0 } );
	animation.push_back( { "Duration", T_REAL32, 1, 0 } );
	animation.push_back( { "TimeStep", T_REAL32, 1, 0 } );
	if ( vintage == VINTAGE_NEW )
	{
		animation.push_back( { "Oversampling", T_REAL32, 1, 0 } );
	}
	animation.push_back( { "TrackGroups", T_ARRAY_OF_REFERENCES, 1, nGroupType } );
	const uint32_t nAnimationType = file.AddType( animation );

	const uint32_t nRootType = file.AddType( {
		{ "TrackGroups", T_ARRAY_OF_REFERENCES, 1, nGroupType },
		{ "Animations", T_ARRAY_OF_REFERENCES, 1, nAnimationType },
	} );

	// One transform track, with a name and three empty curves. Tests fill the
	// curves in afterwards.
	built.nTransformTrack = file.AddObject( DISK_TRANSFORM_TRACK );
	file.PointAtString( built.nTransformTrack, "Basis" );

	// The track group.
	const uint32_t nGroupSize =
		4 + 8 + 8 + ( vintage == VINTAGE_NEW ? 8u : 0u ) + 8 + DISK_TRANSFORM + 4 + 12 + 4
		+ 4;
	built.nTrackGroup = file.AddObject( nGroupSize );
	file.PointAtString( built.nTrackGroup, "Basis" );

	// Whichever of the two names this vintage uses, the scalar track array is
	// first and the transform track array second.
	const uint32_t nTransformTracksAt = built.nTrackGroup + 4 + 8;
	file.PutI32( nTransformTracksAt, 1 );
	file.Point( nTransformTracksAt + 4, CTypedFile::OBJECTS, built.nTransformTrack );

	const uint32_t nAfterTracks =
		nTransformTracksAt + 8 + ( vintage == VINTAGE_NEW ? 8u : 0u );
	const uint32_t nInitialPlacementAt = nAfterTracks + 8;
	// An identity placement, since the DLL reports Flags 0 for all 11,360 groups.
	file.PutReal32( nInitialPlacementAt + 4 + 12 + 12, 1.0f );   // Orientation.w
	file.PutReal32( nInitialPlacementAt + 4 + 12 + 16, 1.0f );   // ScaleShear[0][0]
	file.PutReal32( nInitialPlacementAt + 4 + 12 + 32, 1.0f );   // ScaleShear[1][1]
	file.PutReal32( nInitialPlacementAt + 4 + 12 + 48, 1.0f );   // ScaleShear[2][2]
	// AccumulationFlags, which 2.11 calls Flags. 2 in 11,189 groups of the corpus.
	file.PutI32( nInitialPlacementAt + DISK_TRANSFORM, 2 );

	// The animation.
	const uint32_t nAnimationSize = 4 + 4 + 4 + ( vintage == VINTAGE_NEW ? 4u : 0u ) + 8;
	built.nAnimation = file.AddObject( nAnimationSize );
	file.PointAtString( built.nAnimation, "walk" );
	file.PutReal32( built.nAnimation + 4, 2.5f );
	file.PutReal32( built.nAnimation + 8, 0.03125f );
	if ( vintage == VINTAGE_NEW )
	{
		file.PutReal32( built.nAnimation + 12, 2.0f );
	}
	const uint32_t nAnimationGroupsAt =
		built.nAnimation + 12 + ( vintage == VINTAGE_NEW ? 4u : 0u );
	file.PutI32( nAnimationGroupsAt, 1 );
	const uint32_t nAnimationGroupSlots = file.AddObject( 4 );
	file.Point( nAnimationGroupsAt + 4, CTypedFile::OBJECTS, nAnimationGroupSlots );
	file.Point( nAnimationGroupSlots, CTypedFile::OBJECTS, built.nTrackGroup );

	// The root: one track group and one animation, the group shared between them.
	const uint32_t nRoot = file.AddObject( 8 + 8 );
	file.PutI32( nRoot, 1 );
	const uint32_t nRootGroupSlots = file.AddObject( 4 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nRootGroupSlots );
	file.Point( nRootGroupSlots, CTypedFile::OBJECTS, built.nTrackGroup );
	file.PutI32( nRoot + 8, 1 );
	const uint32_t nRootAnimationSlots = file.AddObject( 4 );
	file.Point( nRoot + 12, CTypedFile::OBJECTS, nRootAnimationSlots );
	file.Point( nRootAnimationSlots, CTypedFile::OBJECTS, built.nAnimation );

	file.SetRoot( nRootType, nRoot );
	return built;
}

}

TEST( Animation, ConvertsTheNewestVintage )
{
	SBuilt built = BuildAnimationFile( VINTAGE_NEW );
	// A rotation curve: two knots, four controls each, which is what an
	// orientation curve looks like in every file that has one.
	PutCurve( built.file, built.nTransformTrack + 4 + DISK_CURVE, 2, { 0.0f, 1.0f },
	          { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.75f } );

	granny_file *pFile = built.file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );

	ASSERT_EQ( 1, pInfo->nAnimationCount );
	ASSERT_EQ( 1, pInfo->nTrackGroupCount );

	const NGr2::SAnimation *pAnimation = pInfo->ppAnimations[0];
	ASSERT_NE( nullptr, pAnimation );
	EXPECT_STREQ( "walk", pAnimation->pszName );
	EXPECT_FLOAT_EQ( 2.5f, pAnimation->fDuration );
	EXPECT_FLOAT_EQ( 0.03125f, pAnimation->fTimeStep );
	EXPECT_FLOAT_EQ( 2.0f, pAnimation->fOversampling );
	// Absent from every file in the corpus, and 0 in all 11,400 animations the
	// real DLL read.
	EXPECT_EQ( 0, pAnimation->nDefaultLoopCount );
	EXPECT_EQ( 0, pAnimation->nFlags );
	EXPECT_EQ( nullptr, pAnimation->ExtendedData.pType );

	ASSERT_EQ( 1, pAnimation->nTrackGroupCount );
	// One object in the file is one object in memory however many ways it is
	// reached, which is why this is EQ and not merely both non-null.
	EXPECT_EQ( pInfo->ppTrackGroups[0], pAnimation->ppTrackGroups[0] );

	const NGr2::STrackGroup *pGroup = pAnimation->ppTrackGroups[0];
	ASSERT_NE( nullptr, pGroup );
	EXPECT_STREQ( "Basis", pGroup->pszName );
	EXPECT_EQ( 2, pGroup->nFlags ) << "AccumulationFlags became Flags";
	EXPECT_EQ( 0, pGroup->nTransformLODErrorCount );
	EXPECT_EQ( nullptr, pGroup->pPeriodicLoop );
	EXPECT_EQ( 0, pGroup->nVectorTrackCount );
	EXPECT_EQ( 0, pGroup->nTextTrackCount );
	ASSERT_EQ( 1, pGroup->nTransformTrackCount );

	const NGr2::STransformTrack &track = pGroup->pTransformTracks[0];
	EXPECT_STREQ( "Basis", track.pszName );
	EXPECT_EQ( 0, track.nFlags );

	// The curve became a variant. Every one of the 772,743 curves in the corpus
	// comes back as DaK32fC32f, which is the 2.11 format with the same three
	// fields an old curve has.
	ASSERT_NE( nullptr, track.OrientationCurve.CurveData.pType );
	const NGr2::SCurveDataDaK32fC32f *pCurve =
		static_cast<const NGr2::SCurveDataDaK32fC32f *>(
			track.OrientationCurve.CurveData.pObject );
	ASSERT_NE( nullptr, pCurve );
	// 1 rather than the enum, which would compare a constant against itself.
	// This is the number the real DLL wrote into all 772,743 of them, and it is
	// DaK32fC32f's place in the order granny211.h declares the formats in.
	EXPECT_EQ( 1, pCurve->Header.nFormat );
	EXPECT_EQ( NGr2::CURVE_DA_K32F_C32F, pCurve->Header.nFormat );
	EXPECT_EQ( 2, pCurve->Header.nDegree );
	EXPECT_EQ( 0, pCurve->nPadding ) << "the DLL leaves this uninitialised; this "
	                                    "library does not reproduce that";
	ASSERT_EQ( 2, pCurve->nKnotCount );
	ASSERT_EQ( 8, pCurve->nControlCount );
	EXPECT_FLOAT_EQ( 0.0f, pCurve->pKnots[0] );
	EXPECT_FLOAT_EQ( 1.0f, pCurve->pKnots[1] );
	EXPECT_FLOAT_EQ( 1.0f, pCurve->pControls[3] );
	EXPECT_FLOAT_EQ( 0.75f, pCurve->pControls[7] );

	// An untouched curve is still a real object with a type, not a null variant:
	// 228,061 scale-shear curves in the corpus are exactly this.
	ASSERT_NE( nullptr, track.ScaleShearCurve.CurveData.pType );
	EXPECT_EQ( track.OrientationCurve.CurveData.pType,
	           track.ScaleShearCurve.CurveData.pType );
	const NGr2::SCurveDataDaK32fC32f *pEmpty =
		static_cast<const NGr2::SCurveDataDaK32fC32f *>(
			track.ScaleShearCurve.CurveData.pObject );
	ASSERT_NE( nullptr, pEmpty );
	EXPECT_EQ( 1, pEmpty->Header.nFormat );
	EXPECT_EQ( 0, pEmpty->nKnotCount );
	EXPECT_EQ( 0, pEmpty->nControlCount );
	EXPECT_EQ( nullptr, pEmpty->pKnots );
	EXPECT_EQ( nullptr, pEmpty->pControls );

	GrannyFreeFile( pFile );
}

TEST( Animation, ConvertsTheOlderVintageThatNamesThingsDifferently )
{
	// The same conversion out of a file that calls the scalar tracks
	// ScalarTracks and has neither TransformLODErrors nor Oversampling. 6,263 of
	// the 21,720 files are like this, and reading members by name is the whole
	// reason one converter serves both.
	SBuilt built = BuildAnimationFile( VINTAGE_OLD );
	PutCurve( built.file, built.nTransformTrack + 4, 0, { 0.0f },
	          { 1.0f, 2.0f, 3.0f } );

	granny_file *pFile = built.file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );

	ASSERT_EQ( 1, pInfo->nAnimationCount );
	const NGr2::SAnimation *pAnimation = pInfo->ppAnimations[0];
	EXPECT_FLOAT_EQ( 2.5f, pAnimation->fDuration );
	// Not in this vintage's type tree, and 0.0 is what the DLL reports for the
	// 2,984 animated files that lack it.
	EXPECT_FLOAT_EQ( 0.0f, pAnimation->fOversampling );

	ASSERT_EQ( 1, pAnimation->nTrackGroupCount );
	const NGr2::STrackGroup *pGroup = pAnimation->ppTrackGroups[0];
	ASSERT_NE( nullptr, pGroup );
	EXPECT_EQ( 2, pGroup->nFlags );
	EXPECT_EQ( 0, pGroup->nTransformLODErrorCount );
	EXPECT_EQ( nullptr, pGroup->pTransformLODErrors );
	ASSERT_EQ( 1, pGroup->nTransformTrackCount );

	const NGr2::SCurveDataDaK32fC32f *pCurve =
		static_cast<const NGr2::SCurveDataDaK32fC32f *>(
			pGroup->pTransformTracks[0].PositionCurve.CurveData.pObject );
	ASSERT_NE( nullptr, pCurve );
	EXPECT_EQ( 0, pCurve->Header.nDegree );
	ASSERT_EQ( 1, pCurve->nKnotCount );
	ASSERT_EQ( 3, pCurve->nControlCount );
	EXPECT_FLOAT_EQ( 3.0f, pCurve->pControls[2] );

	GrannyFreeFile( pFile );
}

TEST( Animation, ConvertsVectorTracksAndTextTracks )
{
	// Neither is common. There are 24 vector tracks in the whole corpus and not
	// one text track, so this is the only evidence either path works; the shapes
	// are the ones the type trees of all 21,720 files agree on.
	CTypedFile file;

	const uint32_t nReal32Type = file.AddType( { { "Real32", T_REAL32, 1, 0 } } );
	const uint32_t nCurveType = AddCurveType( file, nReal32Type );
	const uint32_t nVectorTrackType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "Dimension", T_INT32, 1, 0 },
		{ "ValueCurve", T_INLINE, 1, nCurveType },
	} );
	const uint32_t nEntryType = file.AddType( {
		{ "TimeStamp", T_REAL32, 1, 0 },
		{ "Text", T_STRING, 1, 0 },
	} );
	const uint32_t nTextTrackType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "Entries", T_REFERENCE_TO_ARRAY, 1, nEntryType },
	} );
	const uint32_t nGroupType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "VectorTracks", T_REFERENCE_TO_ARRAY, 1, nVectorTrackType },
		{ "TextTracks", T_REFERENCE_TO_ARRAY, 1, nTextTrackType },
	} );
	const uint32_t nRootType =
		file.AddType( { { "TrackGroups", T_ARRAY_OF_REFERENCES, 1, nGroupType } } );

	// One vector track: a name, a dimension and a curve of one control per knot.
	const uint32_t nVectorTrack = file.AddObject( 4 + 4 + DISK_CURVE );
	file.PointAtString( nVectorTrack, "Blend" );
	file.PutI32( nVectorTrack + 4, 1 );
	PutCurve( file, nVectorTrack + 8, 2, { 0.0f, 0.5f, 1.0f }, { 0.0f, 0.25f, 1.0f } );

	// Two annotations. Unlike knots and controls these cannot be handed over in
	// place, because an entry holds a string and a string is a pointer.
	const uint32_t nEntries = file.AddObject( 8 * 2 );
	file.PutReal32( nEntries, 0.25f );
	file.PointAtString( nEntries + 4, "footstep_left" );
	file.PutReal32( nEntries + 8, 0.75f );
	file.PointAtString( nEntries + 12, "footstep_right" );

	const uint32_t nTextTrack = file.AddObject( 4 + 8 );
	file.PointAtString( nTextTrack, "annotations" );
	file.PutI32( nTextTrack + 4, 2 );
	file.Point( nTextTrack + 8, CTypedFile::OBJECTS, nEntries );

	const uint32_t nGroup = file.AddObject( 4 + 8 + 8 );
	file.PointAtString( nGroup, "Basis" );
	file.PutI32( nGroup + 4, 1 );
	file.Point( nGroup + 8, CTypedFile::OBJECTS, nVectorTrack );
	file.PutI32( nGroup + 12, 1 );
	file.Point( nGroup + 16, CTypedFile::OBJECTS, nTextTrack );

	const uint32_t nRoot = file.AddObject( 8 );
	file.PutI32( nRoot, 1 );
	const uint32_t nSlots = file.AddObject( 4 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nSlots );
	file.Point( nSlots, CTypedFile::OBJECTS, nGroup );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	ASSERT_EQ( 1, pInfo->nTrackGroupCount );

	const NGr2::STrackGroup *pGroup = pInfo->ppTrackGroups[0];
	ASSERT_EQ( 1, pGroup->nVectorTrackCount );
	EXPECT_STREQ( "Blend", pGroup->pVectorTracks[0].pszName );
	EXPECT_EQ( 1, pGroup->pVectorTracks[0].nDimension );
	// In no file's type tree at all, and 0 in all 24 vector tracks the DLL read.
	EXPECT_EQ( 0u, pGroup->pVectorTracks[0].nTrackKey );

	const NGr2::SCurveDataDaK32fC32f *pCurve =
		static_cast<const NGr2::SCurveDataDaK32fC32f *>(
			pGroup->pVectorTracks[0].ValueCurve.CurveData.pObject );
	ASSERT_NE( nullptr, pCurve );
	ASSERT_EQ( 3, pCurve->nKnotCount );
	ASSERT_EQ( 3, pCurve->nControlCount );
	// A vector track has one control per knot, so the counts alone cannot tell
	// the two arrays apart. The values can, and are different on purpose.
	EXPECT_FLOAT_EQ( 0.5f, pCurve->pKnots[1] );
	EXPECT_FLOAT_EQ( 0.25f, pCurve->pControls[1] );

	ASSERT_EQ( 1, pGroup->nTextTrackCount );
	EXPECT_STREQ( "annotations", pGroup->pTextTracks[0].pszName );
	ASSERT_EQ( 2, pGroup->pTextTracks[0].nEntryCount );
	EXPECT_FLOAT_EQ( 0.25f, pGroup->pTextTracks[0].pEntries[0].fTimeStamp );
	EXPECT_STREQ( "footstep_left", pGroup->pTextTracks[0].pEntries[0].pszText );
	EXPECT_FLOAT_EQ( 0.75f, pGroup->pTextTracks[0].pEntries[1].fTimeStamp );
	EXPECT_STREQ( "footstep_right", pGroup->pTextTracks[0].pEntries[1].pszText );

	GrannyFreeFile( pFile );
}

TEST( Animation, ConvertsAPeriodicLoop )
{
	// Null in all 11,360 track groups of the corpus, so like the text track this
	// is authored rather than measured. It is converted rather than dropped
	// because a count and a pointer that disagree is the failure mode this
	// converter exists to avoid, and a reference is cheap to follow.
	CTypedFile file;
	const uint32_t nLoopType = file.AddType( {
		{ "Radius", T_REAL32, 1, 0 },
		{ "dAngle", T_REAL32, 1, 0 },
		{ "dZ", T_REAL32, 1, 0 },
		{ "BasisX", T_REAL32, 3, 0 },
		{ "BasisY", T_REAL32, 3, 0 },
		{ "Axis", T_REAL32, 3, 0 },
	} );
	const uint32_t nGroupType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "PeriodicLoop", T_REFERENCE, 1, nLoopType },
	} );
	const uint32_t nRootType =
		file.AddType( { { "TrackGroups", T_ARRAY_OF_REFERENCES, 1, nGroupType } } );

	const uint32_t nLoop = file.AddObject( 4 * 3 + 12 * 3 );
	file.PutReal32( nLoop, 2.5f );
	file.PutReal32( nLoop + 4, 0.125f );
	file.PutReal32( nLoop + 8, -1.0f );
	file.PutReal32( nLoop + 12, 1.0f );        // BasisX[0]
	file.PutReal32( nLoop + 28, 1.0f );        // BasisY[1]
	file.PutReal32( nLoop + 44, 1.0f );        // Axis[2]

	const uint32_t nGroup = file.AddObject( 4 + 4 );
	file.PointAtString( nGroup, "Basis" );
	file.Point( nGroup + 4, CTypedFile::OBJECTS, nLoop );

	const uint32_t nRoot = file.AddObject( 8 );
	file.PutI32( nRoot, 1 );
	const uint32_t nSlots = file.AddObject( 4 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nSlots );
	file.Point( nSlots, CTypedFile::OBJECTS, nGroup );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	ASSERT_EQ( 1, pInfo->nTrackGroupCount );

	const NGr2::SPeriodicLoop *pLoop = pInfo->ppTrackGroups[0]->pPeriodicLoop;
	ASSERT_NE( nullptr, pLoop );
	EXPECT_FLOAT_EQ( 2.5f, pLoop->fRadius );
	EXPECT_FLOAT_EQ( 0.125f, pLoop->fdAngle );
	EXPECT_FLOAT_EQ( -1.0f, pLoop->fdZ );
	EXPECT_FLOAT_EQ( 1.0f, pLoop->BasisX[0] );
	EXPECT_FLOAT_EQ( 1.0f, pLoop->BasisY[1] );
	EXPECT_FLOAT_EQ( 1.0f, pLoop->Axis[2] );

	GrannyFreeFile( pFile );
}

TEST( Animation, ACountAndItsPointerNeverDisagree )
{
	// A track group whose TransformTracks count is set but whose pointer has no
	// fixup, which is how a GR2 spells a null. The engine loops to the count and
	// dereferences as it goes, so a count without an array behind it is a crash;
	// zero and null together are merely an animation with no tracks.
	CTypedFile file;
	const uint32_t nReal32Type = file.AddType( { { "Real32", T_REAL32, 1, 0 } } );
	const uint32_t nCurveType = AddCurveType( file, nReal32Type );
	const uint32_t nTrackType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "PositionCurve", T_INLINE, 1, nCurveType },
	} );
	const uint32_t nGroupType = file.AddType( {
		{ "Name", T_STRING, 1, 0 },
		{ "TransformTracks", T_REFERENCE_TO_ARRAY, 1, nTrackType },
	} );
	const uint32_t nRootType =
		file.AddType( { { "TrackGroups", T_ARRAY_OF_REFERENCES, 1, nGroupType } } );

	const uint32_t nGroup = file.AddObject( 4 + 8 );
	file.PointAtString( nGroup, "Basis" );
	file.PutI32( nGroup + 4, 7 );
	// No Point() for nGroup + 8, so the slot has no fixup.

	const uint32_t nRoot = file.AddObject( 8 );
	file.PutI32( nRoot, 1 );
	const uint32_t nSlots = file.AddObject( 4 );
	file.Point( nRoot + 4, CTypedFile::OBJECTS, nSlots );
	file.Point( nSlots, CTypedFile::OBJECTS, nGroup );
	file.SetRoot( nRootType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	ASSERT_EQ( 1, pInfo->nTrackGroupCount );

	EXPECT_EQ( 0, pInfo->ppTrackGroups[0]->nTransformTrackCount );
	EXPECT_EQ( nullptr, pInfo->ppTrackGroups[0]->pTransformTracks );

	GrannyFreeFile( pFile );
}

TEST( Animation, AFileWithNoAnimationHasNoneRatherThanAnEmptyOne )
{
	// What every geometry resource in the game is: 10,320 of the 21,720 files
	// have neither an animation nor a track group. The engine reads the count
	// before the pointer, so this is the case that has to stay boring.
	CTypedFile file;
	const uint32_t nType = file.AddType( { { "FromFileName", T_STRING, 1, 0 } } );
	const uint32_t nRoot = file.AddObject( 4 );
	file.PointAtString( nRoot, "static.mb" );
	file.SetRoot( nType, nRoot );

	granny_file *pFile = file.Load();
	ASSERT_NE( nullptr, pFile );
	const NGr2::SFileInfo *pInfo = Info( pFile );
	ASSERT_NE( nullptr, pInfo );
	EXPECT_EQ( 0, pInfo->nAnimationCount );
	EXPECT_EQ( nullptr, pInfo->ppAnimations );
	EXPECT_EQ( 0, pInfo->nTrackGroupCount );
	EXPECT_EQ( nullptr, pInfo->ppTrackGroups );
	GrannyFreeFile( pFile );
}
