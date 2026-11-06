#pragma once

#include "System/XmlSaver.h"

#include "MapEditorLib_export.h"

namespace NIconRendering
{
inline constexpr int DEFAULT_SIZE = 42;
// The editor's frame-transition capture target is 512 by 512.
inline constexpr int TARGET_SIZE = 512;

// The render command carries this batch type in its data argument; zero keeps
// the existing mech-unit command compatible with callers that pass no data.
enum EBatchType
{
	MECH_UNITS,
	BUILDINGS,
	BRIDGES,
	FENCES,
	OBJECTS,
	BATCH_COUNT
};

struct SOptions
{
	int nWidth = DEFAULT_SIZE;
	int nHeight = DEFAULT_SIZE;
	bool bSeparatePak = true;
	bool bBuildingPak = true;
	bool bBridgePak = true;
	bool bFencePak = true;
	bool bObjectPak = true;

	bool IsValid() const
	{
		return nWidth > 0 && nWidth <= TARGET_SIZE && nHeight > 0 && nHeight <= TARGET_SIZE;
	}

	int operator&( IXmlSaver &xs )
	{
		xs.Add( "Width", &nWidth );
		xs.Add( "Height", &nHeight );
		xs.Add( "SeparatePak", &bSeparatePak );
		xs.Add( "BuildingPak", &bBuildingPak );
		xs.Add( "BridgePak", &bBridgePak );
		xs.Add( "FencePak", &bFencePak );
		xs.Add( "ObjectPak", &bObjectPak );
		return 0;
	}
};

struct SBatchInfo
{
	const char *pszTypeName;
	const char *pszButtonLabel;
	const char *pszArchiveBase;
	bool SOptions::*pSeparatePak;
};

// Share the type, output namespace and preference between the dialog and exporter.
inline constexpr SBatchInfo BATCHES[BATCH_COUNT] = {
	{ "MechUnitRPGStats", "Re-Render All Mech Units", "MechUnitIcons", &SOptions::bSeparatePak },
	{ "BuildingRPGStats", "Re-Render All Buildings", "BuildingIcons", &SOptions::bBuildingPak },
	{ "BridgeRPGStats", "Re-Render All Bridges", "BridgeIcons", &SOptions::bBridgePak },
	{ "FenceRPGStats", "Re-Render All Fences", "FenceIcons", &SOptions::bFencePak },
	{ "ObjectRPGStats", "Re-Render All Objects", "ObjectIcons", &SOptions::bObjectPak } };

MAPEDITORLIB_EXPORT SOptions LoadOptions();
MAPEDITORLIB_EXPORT bool SaveOptions( SOptions options );
} // namespace NIconRendering
