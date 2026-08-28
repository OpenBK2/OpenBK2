// Turning what the file contains into what the engine reads.
//
// Two transformations happen here and they are worth keeping apart.
//
// MARSHALLING applies to everything. The file stores 32-bit pointers and the
// host may have 64-bit ones, and an array on disk is a count followed by a
// pointer in one member where granny211.h has two. That is mechanical, and it is
// why granny_file_info is 92 bytes on disk and 148 in memory with exactly the
// same fields.
//
// VERSION CONVERSION applies to nine structures. These files were written by a
// 2.5-era exporter and their type trees say so: a bone has seven members ending
// LightInfo, CameraInfo and ExtendedData, where granny211.h has six ending
// LODError and ExtendedData, and the two in between were renamed. There is no
// mechanical rule for that, so each is written out below, and the values Granny
// invents for members the file does not have were measured from the real DLL
// rather than guessed. See docs/GrannyReplacement.md.
//
// Everything is read through the file's own type tree by member name, never by a
// fixed offset. A member the file does not have simply is not found, and the
// default stands, which is what makes one converter serve every struct tag.
//
// Reading by name is not a nicety here. Three exporter vintages are in this
// corpus and they disagree about the animation structures: 15,457 files call a
// track group's scalar tracks VectorTracks and carry TransformLODErrors and an
// animation's Oversampling, 5,948 call them ScalarTracks and have neither, and
// the oldest 315 also drop the track group's RootMotion. One converter reads all
// three because it asks the file what it has.
//
// Scope. Everything the root object reaches: file info, models, meshes, vertex
// data, topology, materials, textures, skeletons, bones, track groups,
// animations and their curves. A texture's pixel bytes are the exception:
// nothing reads them, the engine takes its textures from the database, and they
// are the one part that would cost real memory to carry.
//
// Identity is preserved, not just content. FindFirstAppropriateModel in
// 3Dmotor/GObjectInfo.cpp finds a mesh's model by comparing
// MeshBindings[i].Mesh against a mesh pointer it already holds, so one object in
// the file has to become exactly one object in memory however many ways it is
// reached. That is what m_Converted is for.

#include "Convert.h"

#include "Structures.h"
#include "Trace.h"
#include "TypeTree.h"

#include <cstring>

namespace NGr2
{

namespace
{

//! An object in the file, and the type definition that describes it.
struct SObject
{
	SReference Type;
	SReference At;
};

//! The type definition every converted curve's variant points at.
//!
//! In the real DLL this is the exported global GrannyCurveDataDaK32fC32fType,
//! one address shared by all 772,743 curves in the corpus, and the four members
//! and their reference types below were read out of it. Nothing in the engine
//! walks it; it is here because a variant with a null type is not the same
//! object as one with a type, and reproducing it costs a static array.
SDataTypeDefinition *CurveDataDaK32fC32fType()
{
	static SDataTypeDefinition Header[] = {
		{ MEMBER_UINT8, "Format", nullptr, 0, {}, 0 },
		{ MEMBER_UINT8, "Degree", nullptr, 0, {}, 0 },
		{ MEMBER_END, nullptr, nullptr, 0, {}, 0 },
	};
	static SDataTypeDefinition Real32[] = {
		{ MEMBER_REAL32, "Real32", nullptr, 0, {}, 0 },
		{ MEMBER_END, nullptr, nullptr, 0, {}, 0 },
	};
	static SDataTypeDefinition Curve[] = {
		// Named for the format, not for the member, which is how the DLL has it.
		{ MEMBER_INLINE, "CurveDataHeader_DaK32fC32f", Header, 0, {}, 0 },
		{ MEMBER_INT16, "Padding", nullptr, 0, {}, 0 },
		{ MEMBER_REFERENCE_TO_ARRAY, "Knots", Real32, 0, {}, 0 },
		{ MEMBER_REFERENCE_TO_ARRAY, "Controls", Real32, 0, {}, 0 },
		{ MEMBER_END, nullptr, nullptr, 0, {}, 0 },
	};
	return Curve;
}

//! No shipped file comes near this. It exists so a corrupt count cannot ask for
//! a terabyte before anything notices.
constexpr int32_t MAX_ARRAY = 4 * 1024 * 1024;

class CConverter
{
public:
	explicit CConverter( granny_file &file )
		: m_File( file )
	{
	}

	SFileInfo *ConvertRoot();

private:
	// Reading one member of an object, by name, through the file's type tree.
	const SMember *Member( const SObject &object, const char *pszName ) const;
	const char *String( const SObject &object, const char *pszName ) const;
	int32_t Int32( const SObject &object, const char *pszName, int32_t nDefault = 0 ) const;
	float Real32( const SObject &object, const char *pszName, float fDefault = 0.0f ) const;
	void Real32Array( const SObject &object, const char *pszName, float *pOut,
	                  uint32_t nCount ) const;
	void Transform( const SObject &object, const char *pszName, STransform *pOut ) const;

	//! One curve member, which on disk is an inline granny_old_curve.
	void Curve( const SObject &object, const char *pszName, SCurve2 *pOut );

	void Int32Array( const SObject &object, const char *pszName, int32_t *pOut,
	                 uint32_t nCount ) const;

	//! An Inline member, which is a sub-object stored in place rather than
	//! pointed at, so it needs no fixup and its type is the member's.
	bool InlineAt( const SObject &object, const char *pszName, SObject *pOut ) const;

	//! The object a Reference member points at, with its element type.
	bool Follow( const SObject &object, const char *pszName, SObject *pOut ) const;

	//! An array member: how many, where, and what one element looks like.
	//!
	//! Covers both array forms. ArrayOfReferences stores pointers to elements and
	//! ReferenceToArray stores the elements themselves, and the difference is in
	//! pbOfReferences because it changes how the caller steps through them.
	bool Array( const SObject &object, const char *pszName, int32_t *pnCount, SObject *pFirst,
	            bool *pbOfReferences ) const;

	//! Element i of an array described by Array().
	bool Element( const SObject &first, bool bOfReferences, int32_t i, SObject *pOut ) const;

	//! A plain array of integers or floats, handed over in place.
	//!
	//! These need no conversion, so the engine reads the file's own bytes. Null
	//! when the count is zero or the array does not fit, and the count is then
	//! zeroed too so the two never disagree.
	template <typename T>
	T *RawArray( const SObject &object, const char *pszName, int32_t *pnCount ) const;

	//! Convert once, and hand back the same pointer every time after.
	template <typename T>
	T *Intern( const SObject &object, T *( CConverter::*pMake )( const SObject & ) );

	SArtToolInfo *MakeArtToolInfo( const SObject &object );
	SExporterInfo *MakeExporterInfo( const SObject &object );
	STexture *MakeTexture( const SObject &object );
	SMaterial *MakeMaterial( const SObject &object );
	SSkeleton *MakeSkeleton( const SObject &object );
	SVertexData *MakeVertexData( const SObject &object );
	STriTopology *MakeTriTopology( const SObject &object );
	SMesh *MakeMesh( const SObject &object );
	SModel *MakeModel( const SObject &object );
	SPeriodicLoop *MakePeriodicLoop( const SObject &object );
	STrackGroup *MakeTrackGroup( const SObject &object );
	SAnimation *MakeAnimation( const SObject &object );

	//! The vertex type array, which the engine walks itself.
	SDataTypeDefinition *MakeTypeArray( const SReference &typeRef );

	template <typename T>
	T *Alloc( size_t nCount = 1 )
	{
		return static_cast<T *>( m_File.m_Arena.Alloc( sizeof( T ) * nCount ) );
	}

	granny_file &m_File;
	bool m_bWarnedPointerVertex = false;
};

const SMember *CConverter::Member( const SObject &object, const char *pszName ) const
{
	const std::vector<SMember> *pMembers = ReadType( m_File, object.Type );
	return pMembers == nullptr ? nullptr : FindMember( *pMembers, pszName );
}

const char *CConverter::String( const SObject &object, const char *pszName ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nType != MEMBER_STRING )
	{
		return nullptr;
	}
	return m_File.ReadString( object.At.nSection, object.At.nOffset + pMember->nOffset );
}

int32_t CConverter::Int32( const SObject &object, const char *pszName, int32_t nDefault ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr )
	{
		return nDefault;
	}
	int32_t nValue = nDefault;
	m_File.ReadI32( object.At.nSection, object.At.nOffset + pMember->nOffset, &nValue );
	return nValue;
}

float CConverter::Real32( const SObject &object, const char *pszName, float fDefault ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr )
	{
		return fDefault;
	}
	float fValue = fDefault;
	m_File.ReadReal32( object.At.nSection, object.At.nOffset + pMember->nOffset, &fValue );
	return fValue;
}

void CConverter::Real32Array( const SObject &object, const char *pszName, float *pOut,
                              uint32_t nCount ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nSize < 4 * nCount )
	{
		return;
	}
	m_File.ReadBytes( SReference{ object.At.nSection, object.At.nOffset + pMember->nOffset },
	                  pOut, 4 * nCount );
}

void CConverter::Transform( const SObject &object, const char *pszName, STransform *pOut ) const
{
	// An identity transform, so a file without one still produces something the
	// renderer can multiply by.
	memset( pOut, 0, sizeof( *pOut ) );
	pOut->Orientation[3] = 1.0f;
	pOut->ScaleShear[0][0] = 1.0f;
	pOut->ScaleShear[1][1] = 1.0f;
	pOut->ScaleShear[2][2] = 1.0f;

	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nType != MEMBER_TRANSFORM )
	{
		return;
	}
	// Identical on disk and in memory: no pointers, and every field four bytes.
	m_File.ReadBytes( SReference{ object.At.nSection, object.At.nOffset + pMember->nOffset },
	                  pOut, sizeof( *pOut ) );
}

void CConverter::Curve( const SObject &object, const char *pszName, SCurve2 *pOut )
{
	pOut->CurveData.pType = nullptr;
	pOut->CurveData.pObject = nullptr;

	// A 2.5 curve is a granny_old_curve stored in place: a degree and two float
	// arrays. A 2.11 curve is a variant, one of eighteen formats, and the one
	// that holds exactly a degree and two float arrays is DaK32fC32f. So the
	// conversion is a change of container and not of representation, which is
	// what the real DLL was measured doing: Format is 1 for every curve in the
	// corpus, and Degree is whatever the file said.
	SObject curve;
	if ( !InlineAt( object, pszName, &curve ) && !Follow( object, pszName, &curve ) )
	{
		return;
	}

	SCurveDataDaK32fC32f *pData = Alloc<SCurveDataDaK32fC32f>();
	if ( pData == nullptr )
	{
		return;
	}

	const int32_t nDegree = Int32( curve, "Degree" );
	pData->Header.nFormat = CURVE_DA_K32F_C32F;
	pData->Header.nDegree = static_cast<uint8_t>( nDegree );
	// The DLL leaves this uninitialised, and three curves in one file came back
	// 16414, -17102 and 0. Zero rather than whatever the arena happens to hold.
	pData->nPadding = 0;
	// In place, like indices and vertices: knots and controls are floats and
	// need no conversion, and the file outlives everything this returns. An
	// empty curve keeps a real object with both counts zero, which is what the
	// DLL produces for the 228,061 scale-shear curves that have no data.
	pData->pKnots = RawArray<float>( curve, "Knots", &pData->nKnotCount );
	pData->pControls = RawArray<float>( curve, "Controls", &pData->nControlCount );

	pOut->CurveData.pType = CurveDataDaK32fC32fType();
	pOut->CurveData.pObject = pData;
}

void CConverter::Int32Array( const SObject &object, const char *pszName, int32_t *pOut,
                             uint32_t nCount ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nSize < 4 * nCount )
	{
		return;
	}
	m_File.ReadBytes( SReference{ object.At.nSection, object.At.nOffset + pMember->nOffset },
	                  pOut, 4 * nCount );
}

bool CConverter::InlineAt( const SObject &object, const char *pszName, SObject *pOut ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nType != MEMBER_INLINE
	     || !pMember->bHasReferenceType )
	{
		return false;
	}
	pOut->Type = pMember->ReferenceType;
	pOut->At.nSection = object.At.nSection;
	pOut->At.nOffset = object.At.nOffset + pMember->nOffset;
	return true;
}

bool CConverter::Follow( const SObject &object, const char *pszName, SObject *pOut ) const
{
	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nType != MEMBER_REFERENCE
	     || !pMember->bHasReferenceType )
	{
		return false;
	}
	if ( !m_File.ResolvePointer( object.At.nSection, object.At.nOffset + pMember->nOffset,
	                             &pOut->At ) )
	{
		// No fixup covers the slot, which is how a GR2 spells a null pointer.
		return false;
	}
	pOut->Type = pMember->ReferenceType;
	return true;
}

bool CConverter::Array( const SObject &object, const char *pszName, int32_t *pnCount,
                        SObject *pFirst, bool *pbOfReferences ) const
{
	*pnCount = 0;

	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || !pMember->bHasReferenceType )
	{
		return false;
	}
	if ( pMember->nType != MEMBER_ARRAY_OF_REFERENCES
	     && pMember->nType != MEMBER_REFERENCE_TO_ARRAY )
	{
		return false;
	}

	const uint32_t nAt = object.At.nOffset + pMember->nOffset;
	int32_t nCount = 0;
	if ( !m_File.ReadI32( object.At.nSection, nAt, &nCount ) || nCount <= 0
	     || nCount > MAX_ARRAY )
	{
		return false;
	}
	if ( !m_File.ResolvePointer( object.At.nSection, nAt + 4, &pFirst->At ) )
	{
		return false;
	}

	*pnCount = nCount;
	pFirst->Type = pMember->ReferenceType;
	*pbOfReferences = pMember->nType == MEMBER_ARRAY_OF_REFERENCES;
	return true;
}

bool CConverter::Element( const SObject &first, bool bOfReferences, int32_t i,
                          SObject *pOut ) const
{
	pOut->Type = first.Type;

	if ( bOfReferences )
	{
		// A pointer per element, each needing its own fixup.
		const uint32_t nSlot = first.At.nOffset + static_cast<uint32_t>( i ) * 4;
		return m_File.ResolvePointer( first.At.nSection, nSlot, &pOut->At );
	}

	const uint32_t nStride = DiskObjectSize( m_File, first.Type );
	if ( nStride == 0 )
	{
		return false;
	}
	pOut->At.nSection = first.At.nSection;
	pOut->At.nOffset = first.At.nOffset + static_cast<uint32_t>( i ) * nStride;
	return true;
}

template <typename T>
T *CConverter::RawArray( const SObject &object, const char *pszName, int32_t *pnCount ) const
{
	*pnCount = 0;

	const SMember *pMember = Member( object, pszName );
	if ( pMember == nullptr || pMember->nType != MEMBER_REFERENCE_TO_ARRAY )
	{
		return nullptr;
	}

	const uint32_t nAt = object.At.nOffset + pMember->nOffset;
	int32_t nCount = 0;
	SReference at;
	if ( !m_File.ReadI32( object.At.nSection, nAt, &nCount ) || nCount <= 0
	     || nCount > MAX_ARRAY )
	{
		return nullptr;
	}
	if ( !m_File.ResolvePointer( object.At.nSection, nAt + 4, &at ) )
	{
		return nullptr;
	}

	const uint8_t *pBytes =
		m_File.Raw( at, static_cast<uint32_t>( nCount ) * sizeof( T ) );
	if ( pBytes == nullptr )
	{
		return nullptr;
	}

	*pnCount = nCount;
	// Handed over in place. The engine only reads it, the file owns it, and the
	// file outlives everything this returns.
	return const_cast<T *>( reinterpret_cast<const T *>( pBytes ) );
}

template <typename T>
T *CConverter::Intern( const SObject &object, T *( CConverter::*pMake )( const SObject & ) )
{
	const auto found = m_File.m_Converted.find( object.At );
	if ( found != m_File.m_Converted.end() )
	{
		return static_cast<T *>( found->second );
	}

	T *pResult = Alloc<T>();
	if ( pResult == nullptr )
	{
		return nullptr;
	}
	// Recorded before the body runs, so a cycle finds the object under
	// construction rather than making a second one.
	m_File.m_Converted[object.At] = pResult;

	T *pFilled = ( this->*pMake )( object );
	if ( pFilled != pResult )
	{
		m_File.m_Converted[object.At] = pFilled;
	}
	return pFilled;
}

SDataTypeDefinition *CConverter::MakeTypeArray( const SReference &typeRef )
{
	const std::vector<SMember> *pMembers = ReadType( m_File, typeRef );
	if ( pMembers == nullptr )
	{
		return nullptr;
	}

	// One more than the members, for the End marker the engine loops until.
	SDataTypeDefinition *pOut = Alloc<SDataTypeDefinition>( pMembers->size() + 1 );
	if ( pOut == nullptr )
	{
		return nullptr;
	}

	for ( size_t i = 0; i < pMembers->size(); ++i )
	{
		const SMember &member = ( *pMembers )[i];
		pOut[i].nType = static_cast<int32_t>( member.nType );
		pOut[i].pszName = member.pszName;
		pOut[i].nArrayWidth = member.nArrayWidth;
		pOut[i].pReferenceType =
			member.bHasReferenceType ? MakeTypeArray( member.ReferenceType ) : nullptr;

		if ( !m_bWarnedPointerVertex
		     && ( member.nType == MEMBER_STRING || member.nType == MEMBER_REFERENCE
		          || member.nType == MEMBER_ARRAY_OF_REFERENCES
		          || member.nType == MEMBER_REFERENCE_TO_ARRAY ) )
		{
			// The engine takes GrannyGetTotalObjectSize of a vertex type as the
			// stride of the file's own vertex bytes. That holds only while vertex
			// types are made of numbers, which they are in every shipped file, and
			// would quietly stop holding here.
			m_bWarnedPointerVertex = true;
			Logger().warn( "type at {}:{} has a pointer member ({}), so its in-memory "
			               "size no longer matches the file's",
			               typeRef.nSection, typeRef.nOffset,
			               member.pszName ? member.pszName : "unnamed" );
		}
	}
	// The trailing entry is already zeroed, and MEMBER_END is 0.
	return pOut;
}

SArtToolInfo *CConverter::MakeArtToolInfo( const SObject &object )
{
	SArtToolInfo *p = static_cast<SArtToolInfo *>( m_File.m_Converted[object.At] );
	p->pszFromArtToolName = String( object, "FromArtToolName" );
	p->nArtToolMajorRevision = Int32( object, "ArtToolMajorRevision" );
	p->nArtToolMinorRevision = Int32( object, "ArtToolMinorRevision" );
	// Absent from these files: 2.11 added it. 0 is what the DLL reports.
	p->nArtToolPointerSize = Int32( object, "ArtToolPointerSize" );
	p->fUnitsPerMeter = Real32( object, "UnitsPerMeter" );
	Real32Array( object, "Origin", p->Origin, 3 );
	Real32Array( object, "RightVector", p->RightVector, 3 );
	Real32Array( object, "UpVector", p->UpVector, 3 );
	Real32Array( object, "BackVector", p->BackVector, 3 );
	return p;
}

SExporterInfo *CConverter::MakeExporterInfo( const SObject &object )
{
	SExporterInfo *p = static_cast<SExporterInfo *>( m_File.m_Converted[object.At] );
	p->pszExporterName = String( object, "ExporterName" );
	p->nExporterMajorRevision = Int32( object, "ExporterMajorRevision" );
	p->nExporterMinorRevision = Int32( object, "ExporterMinorRevision" );
	p->nExporterCustomization = Int32( object, "ExporterCustomization" );
	p->nExporterBuildNumber = Int32( object, "ExporterBuildNumber" );
	return p;
}

STexture *CConverter::MakeTexture( const SObject &object )
{
	STexture *p = static_cast<STexture *>( m_File.m_Converted[object.At] );
	p->pszFromFileName = String( object, "FromFileName" );
	p->nTextureType = Int32( object, "TextureType" );
	p->nWidth = Int32( object, "Width" );
	p->nHeight = Int32( object, "Height" );
	p->nEncoding = Int32( object, "Encoding" );
	p->nSubFormat = Int32( object, "SubFormat" );

	SObject layout;
	if ( InlineAt( object, "Layout", &layout ) )
	{
		p->Layout.nBytesPerPixel = Int32( layout, "BytesPerPixel" );
		Int32Array( layout, "ShiftForComponent", p->Layout.ShiftForComponent, 4 );
		Int32Array( layout, "BitsForComponent", p->Layout.BitsForComponent, 4 );
	}

	// The pixels themselves are not converted. Nothing reads them: the engine
	// takes its textures from the database, not from the GR2, and the images are
	// the one part of a texture that would cost real memory to carry. ImageCount
	// stays 0 rather than being set with a null array beside it, so that a count
	// and its pointer never disagree.
	return p;
}

SMaterial *CConverter::MakeMaterial( const SObject &object )
{
	SMaterial *p = static_cast<SMaterial *>( m_File.m_Converted[object.At] );
	p->pszName = String( object, "Name" );

	SObject texture;
	if ( Follow( object, "Texture", &texture ) )
	{
		p->pTexture = Intern( texture, &CConverter::MakeTexture );
	}

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;
	// Renamed in 2.11: the file calls this member Map and granny211.h calls the
	// thing it points at Material.
	if ( !Array( object, "Maps", &nCount, &first, &bOfReferences ) )
	{
		return p;
	}

	SMaterialMap *pMaps = Alloc<SMaterialMap>( static_cast<size_t>( nCount ) );
	if ( pMaps == nullptr )
	{
		return p;
	}
	for ( int32_t i = 0; i < nCount; ++i )
	{
		SObject map;
		SObject material;
		if ( !Element( first, bOfReferences, i, &map ) )
		{
			continue;
		}
		pMaps[i].pszUsage = String( map, "Usage" );
		if ( Follow( map, "Map", &material ) || Follow( map, "Material", &material ) )
		{
			pMaps[i].pMaterial = Intern( material, &CConverter::MakeMaterial );
		}
	}
	p->nMapCount = nCount;
	p->pMaps = pMaps;
	return p;
}

SSkeleton *CConverter::MakeSkeleton( const SObject &object )
{
	SSkeleton *p = static_cast<SSkeleton *>( m_File.m_Converted[object.At] );
	p->pszName = String( object, "Name" );
	// Absent from these files: 2.11 added it. The DLL reports 0.
	p->nLODType = Int32( object, "LODType" );

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;
	if ( !Array( object, "Bones", &nCount, &first, &bOfReferences ) )
	{
		return p;
	}

	SBone *pBones = Alloc<SBone>( static_cast<size_t>( nCount ) );
	if ( pBones == nullptr )
	{
		return p;
	}

	for ( int32_t i = 0; i < nCount; ++i )
	{
		SObject bone;
		if ( !Element( first, bOfReferences, i, &bone ) )
		{
			continue;
		}

		pBones[i].pszName = String( bone, "Name" );
		pBones[i].nParentIndex = Int32( bone, "ParentIndex", -1 );
		// Renamed in 2.11. The file's name is the one to look for, with the new
		// one tried too so a file written by a later exporter still reads.
		Transform( bone, "Transform", &pBones[i].LocalTransform );
		if ( Member( bone, "Transform" ) == nullptr )
		{
			Transform( bone, "LocalTransform", &pBones[i].LocalTransform );
		}
		Real32Array( bone, "InverseWorldTransform", pBones[i].InverseWorld4x4, 16 );
		if ( Member( bone, "InverseWorldTransform" ) == nullptr )
		{
			Real32Array( bone, "InverseWorld4x4", pBones[i].InverseWorld4x4, 16 );
		}
		// Absent from these files, and measured as 0 out of the real DLL. Not 1:
		// a first reading of it at offset 140 caught InverseWorld4x4[15] instead.
		pBones[i].fLODError = Real32( bone, "LODError", 0.0f );
		// LightInfo and CameraInfo have nowhere to go in 2.11 and are null in all
		// 2,472 bones of a 250-file sample, so nothing is lost dropping them.
	}

	p->nBoneCount = nCount;
	p->pBones = pBones;
	return p;
}

SVertexData *CConverter::MakeVertexData( const SObject &object )
{
	SVertexData *p = static_cast<SVertexData *>( m_File.m_Converted[object.At] );

	const SMember *pVertices = Member( object, "Vertices" );
	if ( pVertices == nullptr
	     || pVertices->nType != MEMBER_REFERENCE_TO_VARIANT_ARRAY )
	{
		return p;
	}

	// A variant array: the type of an element, then how many, then where.
	const uint32_t nAt = object.At.nOffset + pVertices->nOffset;
	SReference typeRef;
	int32_t nCount = 0;
	SReference at;
	if ( !m_File.ResolvePointer( object.At.nSection, nAt, &typeRef )
	     || !m_File.ReadI32( object.At.nSection, nAt + 4, &nCount )
	     || !m_File.ResolvePointer( object.At.nSection, nAt + 8, &at ) )
	{
		return p;
	}
	if ( nCount <= 0 || nCount > MAX_ARRAY )
	{
		return p;
	}

	const uint32_t nStride = DiskObjectSize( m_File, typeRef );
	const uint8_t *pBytes =
		nStride == 0 ? nullptr : m_File.Raw( at, static_cast<uint32_t>( nCount ) * nStride );
	if ( pBytes == nullptr )
	{
		Logger().warn( "vertex data at {}:{} claims {} vertices of {} bytes",
		               object.At.nSection, object.At.nOffset, nCount, nStride );
		return p;
	}

	p->pVertexType = MakeTypeArray( typeRef );
	p->nVertexCount = nCount;
	// In place: vertices are numbers and need no conversion, so the engine reads
	// the file's own bytes and the stride it computes is the file's stride.
	p->pVertices = const_cast<uint8_t *>( pBytes );
	return p;
}

STriTopology *CConverter::MakeTriTopology( const SObject &object )
{
	STriTopology *p = static_cast<STriTopology *>( m_File.m_Converted[object.At] );

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;
	if ( Array( object, "Groups", &nCount, &first, &bOfReferences ) )
	{
		STriMaterialGroup *pGroups = Alloc<STriMaterialGroup>( static_cast<size_t>( nCount ) );
		if ( pGroups != nullptr )
		{
			for ( int32_t i = 0; i < nCount; ++i )
			{
				SObject group;
				if ( !Element( first, bOfReferences, i, &group ) )
				{
					continue;
				}
				pGroups[i].nMaterialIndex = Int32( group, "MaterialIndex" );
				pGroups[i].nTriFirst = Int32( group, "TriFirst" );
				pGroups[i].nTriCount = Int32( group, "TriCount" );
			}
			p->nGroupCount = nCount;
			p->pGroups = pGroups;
		}
	}

	p->pIndices = RawArray<int32_t>( object, "Indices", &p->nIndexCount );
	p->pIndices16 = RawArray<uint16_t>( object, "Indices16", &p->nIndex16Count );
	p->pVertexToVertexMap =
		RawArray<int32_t>( object, "VertexToVertexMap", &p->nVertexToVertexCount );
	p->pVertexToTriangleMap =
		RawArray<int32_t>( object, "VertexToTriangleMap", &p->nVertexToTriangleCount );
	p->pSideToNeighborMap =
		RawArray<uint32_t>( object, "SideToNeighborMap", &p->nSideToNeighborCount );
	p->pBonesForTriangle =
		RawArray<int32_t>( object, "BonesForTriangle", &p->nBonesForTriangleCount );
	p->pTriangleToBoneIndices =
		RawArray<int32_t>( object, "TriangleToBoneIndices", &p->nTriangleToBoneCount );
	return p;
}

SMesh *CConverter::MakeMesh( const SObject &object )
{
	SMesh *p = static_cast<SMesh *>( m_File.m_Converted[object.At] );
	p->pszName = String( object, "Name" );

	SObject child;
	if ( Follow( object, "PrimaryVertexData", &child ) )
	{
		p->pPrimaryVertexData = Intern( child, &CConverter::MakeVertexData );
	}
	if ( Follow( object, "PrimaryTopology", &child ) )
	{
		p->pPrimaryTopology = Intern( child, &CConverter::MakeTriTopology );
	}

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;

	if ( Array( object, "MaterialBindings", &nCount, &first, &bOfReferences ) )
	{
		SMaterialBinding *pBindings =
			Alloc<SMaterialBinding>( static_cast<size_t>( nCount ) );
		if ( pBindings != nullptr )
		{
			for ( int32_t i = 0; i < nCount; ++i )
			{
				SObject binding;
				SObject material;
				if ( Element( first, bOfReferences, i, &binding )
				     && Follow( binding, "Material", &material ) )
				{
					pBindings[i].pMaterial = Intern( material, &CConverter::MakeMaterial );
				}
			}
			p->nMaterialBindingCount = nCount;
			p->pMaterialBindings = pBindings;
		}
	}

	if ( Array( object, "BoneBindings", &nCount, &first, &bOfReferences ) )
	{
		SBoneBinding *pBindings = Alloc<SBoneBinding>( static_cast<size_t>( nCount ) );
		if ( pBindings != nullptr )
		{
			for ( int32_t i = 0; i < nCount; ++i )
			{
				SObject binding;
				if ( !Element( first, bOfReferences, i, &binding ) )
				{
					continue;
				}
				pBindings[i].pszBoneName = String( binding, "BoneName" );
				Real32Array( binding, "OBBMin", pBindings[i].OBBMin, 3 );
				Real32Array( binding, "OBBMax", pBindings[i].OBBMax, 3 );
				pBindings[i].pTriangleIndices = RawArray<int32_t>(
					binding, "TriangleIndices", &pBindings[i].nTriangleCount );
			}
			p->nBoneBindingCount = nCount;
			p->pBoneBindings = pBindings;
		}
	}

	return p;
}

SModel *CConverter::MakeModel( const SObject &object )
{
	SModel *p = static_cast<SModel *>( m_File.m_Converted[object.At] );
	p->pszName = String( object, "Name" );
	Transform( object, "InitialPlacement", &p->InitialPlacement );

	SObject skeleton;
	if ( Follow( object, "Skeleton", &skeleton ) )
	{
		p->pSkeleton = Intern( skeleton, &CConverter::MakeSkeleton );
	}

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;
	if ( !Array( object, "MeshBindings", &nCount, &first, &bOfReferences ) )
	{
		return p;
	}

	SModelMeshBinding *pBindings = Alloc<SModelMeshBinding>( static_cast<size_t>( nCount ) );
	if ( pBindings == nullptr )
	{
		return p;
	}
	for ( int32_t i = 0; i < nCount; ++i )
	{
		SObject binding;
		SObject mesh;
		if ( Element( first, bOfReferences, i, &binding ) && Follow( binding, "Mesh", &mesh ) )
		{
			pBindings[i].pMesh = Intern( mesh, &CConverter::MakeMesh );
		}
	}
	p->nMeshBindingCount = nCount;
	p->pMeshBindings = pBindings;
	return p;
}

SPeriodicLoop *CConverter::MakePeriodicLoop( const SObject &object )
{
	SPeriodicLoop *p = static_cast<SPeriodicLoop *>( m_File.m_Converted[object.At] );
	p->fRadius = Real32( object, "Radius" );
	p->fdAngle = Real32( object, "dAngle" );
	p->fdZ = Real32( object, "dZ" );
	Real32Array( object, "BasisX", p->BasisX, 3 );
	Real32Array( object, "BasisY", p->BasisY, 3 );
	Real32Array( object, "Axis", p->Axis, 3 );
	return p;
}

STrackGroup *CConverter::MakeTrackGroup( const SObject &object )
{
	STrackGroup *p = static_cast<STrackGroup *>( m_File.m_Converted[object.At] );
	p->pszName = String( object, "Name" );
	Transform( object, "InitialPlacement", &p->InitialPlacement );
	Real32Array( object, "LoopTranslation", p->LoopTranslation, 3 );

	// Renamed in 2.11, so the file's name is tried first and the new one after,
	// which is the same order the bone's Transform and InverseWorldTransform use.
	// 11,189 groups in the corpus have 2 here and 171 have 0.
	p->nFlags = Int32( object, "AccumulationFlags" );
	if ( Member( object, "AccumulationFlags" ) == nullptr )
	{
		p->nFlags = Int32( object, "Flags" );
	}

	SObject loop;
	if ( Follow( object, "PeriodicLoop", &loop ) )
	{
		p->pPeriodicLoop = Intern( loop, &CConverter::MakePeriodicLoop );
	}
	// Present in 15,457 of the 21,720 files and empty in every one of them, so
	// this is only ever the zero the older files get by not having the member.
	p->pTransformLODErrors =
		RawArray<float>( object, "TransformLODErrors", &p->nTransformLODErrorCount );

	// RootMotion is dropped the way a bone's LightInfo is: 2.11 has nowhere to
	// put it. It is a member of 21,405 of the 21,720 files' track groups, and the
	// oldest 315 do not have it either.

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;

	// Renamed in 2.11: 6,263 files call these ScalarTracks and 15,457 call them
	// VectorTracks, which is one reason members are resolved by name here.
	if ( Array( object, "VectorTracks", &nCount, &first, &bOfReferences )
	     || Array( object, "ScalarTracks", &nCount, &first, &bOfReferences ) )
	{
		SVectorTrack *pTracks = Alloc<SVectorTrack>( static_cast<size_t>( nCount ) );
		if ( pTracks != nullptr )
		{
			for ( int32_t i = 0; i < nCount; ++i )
			{
				SObject track;
				if ( !Element( first, bOfReferences, i, &track ) )
				{
					continue;
				}
				pTracks[i].pszName = String( track, "Name" );
				// Only the newer files carry Dimension, and every vector track in
				// the corpus is in one of those, so what the DLL does without it
				// was not observable. TrackKey is in no file at all and the DLL
				// reports 0 for all 24 of them.
				pTracks[i].nDimension = Int32( track, "Dimension" );
				pTracks[i].nTrackKey = static_cast<uint32_t>( Int32( track, "TrackKey" ) );
				Curve( track, "ValueCurve", &pTracks[i].ValueCurve );
			}
			p->nVectorTrackCount = nCount;
			p->pVectorTracks = pTracks;
		}
	}

	if ( Array( object, "TransformTracks", &nCount, &first, &bOfReferences ) )
	{
		STransformTrack *pTracks = Alloc<STransformTrack>( static_cast<size_t>( nCount ) );
		if ( pTracks != nullptr )
		{
			for ( int32_t i = 0; i < nCount; ++i )
			{
				SObject track;
				if ( !Element( first, bOfReferences, i, &track ) )
				{
					continue;
				}
				pTracks[i].pszName = String( track, "Name" );
				// Absent from every file, and 0 in all 257,581 tracks of the DLL's
				// reading of them.
				pTracks[i].nFlags = Int32( track, "Flags" );
				Curve( track, "PositionCurve", &pTracks[i].PositionCurve );
				Curve( track, "OrientationCurve", &pTracks[i].OrientationCurve );
				Curve( track, "ScaleShearCurve", &pTracks[i].ScaleShearCurve );
			}
			p->nTransformTrackCount = nCount;
			p->pTransformTracks = pTracks;
		}
	}

	// No file in the corpus has a single text track, so this path is written
	// from the type tree, which every file carries whether it uses it or not,
	// and is the one part of this conversion the corpus cannot check.
	if ( Array( object, "TextTracks", &nCount, &first, &bOfReferences ) )
	{
		STextTrack *pTracks = Alloc<STextTrack>( static_cast<size_t>( nCount ) );
		if ( pTracks != nullptr )
		{
			for ( int32_t i = 0; i < nCount; ++i )
			{
				SObject track;
				if ( !Element( first, bOfReferences, i, &track ) )
				{
					continue;
				}
				pTracks[i].pszName = String( track, "Name" );

				int32_t nEntries = 0;
				SObject firstEntry;
				bool bEntriesOfReferences = false;
				if ( !Array( track, "Entries", &nEntries, &firstEntry,
				             &bEntriesOfReferences ) )
				{
					continue;
				}
				// An entry holds a string, so unlike knots and controls it cannot
				// be handed over in place: the pointer in it is 32-bit on disk.
				STextTrackEntry *pEntries =
					Alloc<STextTrackEntry>( static_cast<size_t>( nEntries ) );
				if ( pEntries == nullptr )
				{
					continue;
				}
				for ( int32_t e = 0; e < nEntries; ++e )
				{
					SObject entry;
					if ( !Element( firstEntry, bEntriesOfReferences, e, &entry ) )
					{
						continue;
					}
					pEntries[e].fTimeStamp = Real32( entry, "TimeStamp" );
					pEntries[e].pszText = String( entry, "Text" );
				}
				pTracks[i].nEntryCount = nEntries;
				pTracks[i].pEntries = pEntries;
			}
			p->nTextTrackCount = nCount;
			p->pTextTracks = pTracks;
		}
	}

	return p;
}

SAnimation *CConverter::MakeAnimation( const SObject &object )
{
	SAnimation *p = static_cast<SAnimation *>( m_File.m_Converted[object.At] );
	p->pszName = String( object, "Name" );
	p->fDuration = Real32( object, "Duration" );
	p->fTimeStep = Real32( object, "TimeStep" );
	// 2.11 added this and 15,457 of the files have it; the other 6,263 get the
	// 0.0 the DLL reports for them. Where it is present it is real data: 2.0 in
	// 8,410 animations, 1.0 in 6.
	p->fOversampling = Real32( object, "Oversampling" );
	// In no file, and 0 in all 11,400 animations the DLL read.
	p->nDefaultLoopCount = Int32( object, "DefaultLoopCount" );
	p->nFlags = Int32( object, "Flags" );

	int32_t nCount = 0;
	SObject first;
	bool bOfReferences = false;
	if ( !Array( object, "TrackGroups", &nCount, &first, &bOfReferences ) )
	{
		return p;
	}

	STrackGroup **ppGroups = Alloc<STrackGroup *>( static_cast<size_t>( nCount ) );
	if ( ppGroups == nullptr )
	{
		return p;
	}
	for ( int32_t i = 0; i < nCount; ++i )
	{
		SObject group;
		if ( Element( first, bOfReferences, i, &group ) )
		{
			// The same objects the file-level TrackGroups array holds, in all
			// 11,360 groups of the corpus, which interning is what preserves.
			ppGroups[i] = Intern( group, &CConverter::MakeTrackGroup );
		}
	}
	p->nTrackGroupCount = nCount;
	p->ppTrackGroups = ppGroups;
	return p;
}

SFileInfo *CConverter::ConvertRoot()
{
	const SObject root{ m_File.RootObjectType(), m_File.RootObject() };
	if ( ReadType( m_File, root.Type ) == nullptr )
	{
		Logger().warn( "the root object's type at {}:{} does not read",
		               root.Type.nSection, root.Type.nOffset );
		return nullptr;
	}

	SFileInfo *p = Alloc<SFileInfo>();
	if ( p == nullptr )
	{
		return nullptr;
	}

	p->pszFromFileName = String( root, "FromFileName" );

	SObject child;
	if ( Follow( root, "ArtToolInfo", &child ) )
	{
		p->pArtToolInfo = Intern( child, &CConverter::MakeArtToolInfo );
	}
	if ( Follow( root, "ExporterInfo", &child ) )
	{
		p->pExporterInfo = Intern( child, &CConverter::MakeExporterInfo );
	}

	// Each top level array in turn. The counts are set from the same pass that
	// converts the elements, so a count and the pointer beside it never disagree.
	int32_t nTextures = 0;
	int32_t nMaterials = 0;
	int32_t nSkeletons = 0;
	int32_t nVertexDatas = 0;
	int32_t nTriTopologies = 0;
	int32_t nMeshes = 0;
	int32_t nModels = 0;
	int32_t nTrackGroups = 0;
	int32_t nAnimations = 0;

	const struct
	{
		const char *pszName;
		int32_t *pnCount;
	} arrays[] = {
		{ "Textures", &nTextures },       { "Materials", &nMaterials },
		{ "Skeletons", &nSkeletons },     { "VertexDatas", &nVertexDatas },
		{ "TriTopologies", &nTriTopologies }, { "Meshes", &nMeshes },
		{ "Models", &nModels },           { "TrackGroups", &nTrackGroups },
		{ "Animations", &nAnimations },
	};

	void **ppConverted[9] = {};
	for ( size_t a = 0; a < sizeof( arrays ) / sizeof( arrays[0] ); ++a )
	{
		int32_t nCount = 0;
		SObject first;
		bool bOfReferences = false;
		if ( !Array( root, arrays[a].pszName, &nCount, &first, &bOfReferences ) )
		{
			continue;
		}

		void **ppItems = Alloc<void *>( static_cast<size_t>( nCount ) );
		if ( ppItems == nullptr )
		{
			continue;
		}
		for ( int32_t i = 0; i < nCount; ++i )
		{
			SObject item;
			if ( !Element( first, bOfReferences, i, &item ) )
			{
				continue;
			}
			switch ( a )
			{
				case 0: ppItems[i] = Intern( item, &CConverter::MakeTexture ); break;
				case 1: ppItems[i] = Intern( item, &CConverter::MakeMaterial ); break;
				case 2: ppItems[i] = Intern( item, &CConverter::MakeSkeleton ); break;
				case 3: ppItems[i] = Intern( item, &CConverter::MakeVertexData ); break;
				case 4: ppItems[i] = Intern( item, &CConverter::MakeTriTopology ); break;
				case 5: ppItems[i] = Intern( item, &CConverter::MakeMesh ); break;
				case 6: ppItems[i] = Intern( item, &CConverter::MakeModel ); break;
				case 7: ppItems[i] = Intern( item, &CConverter::MakeTrackGroup ); break;
				default: ppItems[i] = Intern( item, &CConverter::MakeAnimation ); break;
			}
		}
		*arrays[a].pnCount = nCount;
		ppConverted[a] = ppItems;
	}

	p->nTextureCount = nTextures;
	p->ppTextures = ppConverted[0];
	p->nMaterialCount = nMaterials;
	p->ppMaterials = reinterpret_cast<SMaterial **>( ppConverted[1] );
	p->nSkeletonCount = nSkeletons;
	p->ppSkeletons = reinterpret_cast<SSkeleton **>( ppConverted[2] );
	p->nVertexDataCount = nVertexDatas;
	p->ppVertexDatas = reinterpret_cast<SVertexData **>( ppConverted[3] );
	p->nTriTopologyCount = nTriTopologies;
	p->ppTriTopologies = reinterpret_cast<STriTopology **>( ppConverted[4] );
	p->nMeshCount = nMeshes;
	p->ppMeshes = reinterpret_cast<SMesh **>( ppConverted[5] );
	p->nModelCount = nModels;
	p->ppModels = reinterpret_cast<SModel **>( ppConverted[6] );
	p->nTrackGroupCount = nTrackGroups;
	p->ppTrackGroups = reinterpret_cast<STrackGroup **>( ppConverted[7] );
	p->nAnimationCount = nAnimations;
	p->ppAnimations = reinterpret_cast<SAnimation **>( ppConverted[8] );
	return p;
}

}

void *ConvertFileInfo( granny_file &file )
{
	if ( file.m_pFileInfo != nullptr || file.m_bConversionFailed )
	{
		return file.m_pFileInfo;
	}

	CConverter converter( file );
	file.m_pFileInfo = converter.ConvertRoot();
	file.m_bConversionFailed = file.m_pFileInfo == nullptr;
	return file.m_pFileInfo;
}

}
