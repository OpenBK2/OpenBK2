#include "stdafx.h"

#include "IconCapture.h"
#include "Image/DDS.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "MechIconPak.h"
#include "System/VFSOperations.h"

#include <algorithm>
#include <filesystem>
#include <limits>

#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/xml/xml.h>
#include <wx/zipstrm.h>

namespace
{
wxXmlNode *Field( wxXmlNode *pRoot, const wxString &name )
{
	for ( wxXmlNode *pNode = pRoot->GetChildren(); pNode; pNode = pNode->GetNext() )
	{
		if ( pNode->GetType() == wxXML_ELEMENT_NODE && pNode->GetName() == name )
		{
			return pNode;
		}
	}
	auto *pNode = new wxXmlNode( wxXML_ELEMENT_NODE, name );
	pRoot->AddChild( pNode );
	return pNode;
}

void TextField( wxXmlNode *pRoot, const wxString &name, const wxString &value )
{
	Field( pRoot, name )->AddChild( new wxXmlNode( wxXML_TEXT_NODE, wxEmptyString, value ) );
}
} // namespace

struct CMechIconPak::SImpl
{
	wxMemoryOutputStream data;
	// wx sets ZIP flag bit 11 when this converter is UTF-8.
	wxZipOutputStream zip{ data, -1, wxConvUTF8 };
	bool bBroken = false;
	bool bClosed = false;
	int nUnits = 0;

	bool AddEntry( const std::string &name, const void *pData, size_t nSize )
	{
		// Entry names are UTF-8 on both platforms. Use UTC wall-clock fields to
		// match the game's timestamp comparison between loose and packed files.
		auto *pEntry = new wxZipEntry( wxString::FromUTF8( name ), wxDateTime::Now().ToUTC(), nSize );
		if ( !zip.PutNextEntry( pEntry ) )
		{
			return false;
		}
		zip.Write( pData, nSize );
		return zip.IsOk() && zip.LastWrite() == nSize && zip.CloseEntry();
	}
};

CMechIconPak::CMechIconPak()
	: pImpl( std::make_unique<SImpl>() )
{
}

CMechIconPak::~CMechIconPak() = default;

bool CMechIconPak::Add( const CDBID &dbid,
                        const CArray2D<NGfx::SPixel8888> &capture,
                        const NIconRendering::SOptions &options,
                        std::string *pError )
{
	// wx errors go through the caller's Log Window message instead of opening
	// a modal error dialog for every bad resource in a batch.
	wxLogNull quiet;
	auto Fail = [pError]( const std::string &message )
	{
		*pError = message;
		return false;
	};
	if ( pImpl->bBroken || pImpl->bClosed )
	{
		return Fail( "the icon archive is no longer writable" );
	}
	std::string szUnitName = NDb::GetFileName( dbid );
	std::replace( szUnitName.begin(), szUnitName.end(), '\\', '/' );
	const auto unitPath = std::filesystem::u8path( szUnitName ).lexically_normal();
	if ( unitPath.empty() || unitPath.is_absolute() || unitPath.has_root_name() || *unitPath.begin() == ".." ||
	     unitPath.extension() != ".xdb" )
	{
		return Fail( "invalid unit resource path: " + szUnitName );
	}
	szUnitName = unitPath.generic_u8string();
	CArray2D<NGfx::SPixel8888> pixels;
	if ( !NIconRendering::CropCapture( &pixels, capture, options ) )
	{
		return Fail( "invalid icon capture dimensions" );
	}

	CMemoryStream snapshot;
	if ( !NDb::SerializeObject( NDb::GetManipulator( dbid ), &snapshot ) )
	{
		return Fail( "could not serialize the unit for the icon archive" );
	}
	wxMemoryInputStream input( snapshot.GetBuffer(), snapshot.GetSize() );
	wxXmlDocument unit;
	if ( !unit.Load( input ) || !unit.GetRoot() || unit.GetRoot()->GetName() != "MechUnitRPGStats" )
	{
		return Fail( "could not read the unit's XML snapshot" );
	}

	// Give each unit its own texture, including variants that previously shared
	// an icon. All references live in the archive, including previously iconless
	// or unsaved units. The original records and their dirty flags stay untouched.
	const std::string szBase = "MechUnitIcons/" + szUnitName.substr( 0, szUnitName.size() - 4 );
	const std::string szTextureName = szBase + ".icon.xdb";
	const std::string szDDSName = szBase + ".icon.dds";
	wxXmlNode *pIcon = Field( unit.GetRoot(), "IconTexture" );
	pIcon->DeleteAttribute( "href" );
	pIcon->AddAttribute( "href", wxString::FromUTF8( "/" + szTextureName + "#xpointer(/Texture)" ) );

	wxXmlDocument texture;
	auto *pRoot = new wxXmlNode( wxXML_ELEMENT_NODE, "Texture" );
	texture.SetRoot( pRoot );
	Field( pRoot, "DestName" )->AddAttribute( "href", wxString::FromUTF8( "/" + szDDSName ) );
	TextField( pRoot, "Type", "TEXTURE_2D" );
	TextField( pRoot, "ConversionType", "CONVERT_ORDINARY" );
	TextField( pRoot, "AddrType", "CLAMP" );
	TextField( pRoot, "Format", "TF_8888" );
	// Width/Height are UI coordinates, not the DDS resolution. Existing unit
	// widgets address 42x42; publishing 512 here makes them crop a blank corner.
	TextField( pRoot, "Width", wxString::Format( "%d", NIconRendering::DEFAULT_SIZE ) );
	TextField( pRoot, "Height", wxString::Format( "%d", NIconRendering::DEFAULT_SIZE ) );
	TextField( pRoot, "NMips", "1" );
	TextField( pRoot, "IsDXT", "false" );
	TextField( pRoot, "FlipY", "false" );
	TextField( pRoot, "StandardExport", "false" );
	wxMemoryOutputStream unitXML;
	wxMemoryOutputStream textureXML;
	if ( !unit.Save( unitXML ) || !texture.Save( textureXML ) )
	{
		return Fail( "could not serialize the icon archive records" );
	}

	// Icons use one uncompressed BGRA mip. Write the existing DDS container
	// directly in memory, preserving the capture's alpha and avoiding loose TGA
	// sources or temporary texture records. ZIP handles the lossless compression.
	SDDSFileHeader header;
	header.header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_PITCH;
	header.header.dwWidth = options.nWidth;
	header.header.dwHeight = options.nHeight;
	header.header.dwPitchOrLinearSize = options.nWidth * sizeof( uint32_t );
	header.header.dwMipMapCount = 1;
	header.header.ddspf = DDSPF_A8R8G8B8;
	header.header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	CMemoryStream dds;
	dds.Write( &header, sizeof( header ) );
	for ( int y = 0; y < options.nHeight; ++y )
	{
		for ( int x = 0; x < options.nWidth; ++x )
		{
			dds.Write( &pixels[y][x].dwColor, sizeof( uint32_t ) );
		}
	}

	// If a ZIP write fails or throws partway through a unit, never publish that
	// partial archive. Per-unit rendering/validation failures may still be skipped.
	pImpl->bBroken = true;
	if ( !pImpl->AddEntry( szDDSName, dds.GetBuffer(), dds.GetSize() ) ||
	     !pImpl->AddEntry( szTextureName, textureXML.GetOutputStreamBuffer()->GetBufferStart(), textureXML.GetSize() ) ||
	     !pImpl->AddEntry( szUnitName, unitXML.GetOutputStreamBuffer()->GetBufferStart(), unitXML.GetSize() ) )
	{
		return Fail( "could not write the icon archive" );
	}
	pImpl->bBroken = false;
	++pImpl->nUnits;
	return true;
}

bool CMechIconPak::Save( const std::string &szDestination, std::string *pError )
{
	wxLogNull quiet;
	if ( pImpl->bBroken || pImpl->bClosed || pImpl->nUnits == 0 )
	{
		*pError = "no complete icon archive to save; the previous archive was kept";
		return false;
	}
	pImpl->bClosed = true;
	if ( !pImpl->zip.Close() || pImpl->data.GetSize() > static_cast<size_t>( std::numeric_limits<int>::max() ) )
	{
		*pError = "could not finalize the icon archive (the game supports archives smaller than 2 GB)";
		return false;
	}
	CMemoryStream data;
	data.SetSize( static_cast<int>( pImpl->data.GetSize() ) );
	pImpl->data.CopyTo( data.GetBufferForWrite(), data.GetSize() );
	return NVFS::WriteFile( NVFS::GetMainFileCreator(), szDestination, data, pError );
}
