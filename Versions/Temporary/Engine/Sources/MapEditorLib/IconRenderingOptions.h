#pragma once

#include "System/XmlSaver.h"

#include "MapEditorLib_export.h"

namespace NIconRendering
{
inline constexpr int DEFAULT_SIZE = 42;
// The editor's frame-transition capture target is 512 by 512.
inline constexpr int TARGET_SIZE = 512;

struct SOptions
{
	int nWidth = DEFAULT_SIZE;
	int nHeight = DEFAULT_SIZE;
	bool bSeparatePak = true;

	bool IsValid() const
	{
		return nWidth > 0 && nWidth <= TARGET_SIZE && nHeight > 0 && nHeight <= TARGET_SIZE;
	}

	int operator&( IXmlSaver &xs )
	{
		xs.Add( "Width", &nWidth );
		xs.Add( "Height", &nHeight );
		xs.Add( "SeparatePak", &bSeparatePak );
		return 0;
	}
};

MAPEDITORLIB_EXPORT SOptions LoadOptions();
MAPEDITORLIB_EXPORT bool SaveOptions( SOptions options );
} // namespace NIconRendering
