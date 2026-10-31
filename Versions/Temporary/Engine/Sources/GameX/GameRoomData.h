#pragma once
#include "InterfaceMPBase.h"
#include "UI/Background.h"

namespace NMPSetData
{
void SetText( ITextView *pText, const std::wstring &szText );
void SetNum( ITextView *pWindow, int nText );
void SetChildText( IListControlItem *pItem, const std::string &szName, const std::wstring &szText );
}; // namespace NMPSetData

class CColorData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CColorData )
public:
	ZDATA
	int nColor;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nColor );
		return 0;
	}

	CColorData() {}

	CColorData( int _nColor )
		: nColor( _nColor )
	{
	}
};

class CColorViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS( CColorViewer )
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const override;
};

class CColorBackground : public CBackground
{
	OBJECT_BASIC_METHODS( CColorBackground );

public:
	int nColor;
	void Visit( struct IUIVisitor *pVisitor ) override;
};

class CTextData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextData )
public:
	ZDATA
	std::wstring wszText;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &wszText );
		return 0;
	}

	CTextData() {}

	CTextData( const std::wstring &_wszText )
		: wszText( _wszText )
	{
	}

	CTextData( int i );
};

//--------------
class CTextDataViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS( CTextDataViewer )
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const override;
};

class CTextureData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextureData )
public:
	ZDATA
	CDBPtr<NDb::STexture> pTexture;
	std::wstring wszTooltip;
	// An SVG flag drawn instead of pTexture when not empty; pTexture stays the
	// fallback for data without flag records.
	std::string szVectorImage;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pTexture );
		f.Add( 3, &wszTooltip );
		f.Add( 4, &szVectorImage );
		return 0;
	}

	CTextureData() {}

	CTextureData( const NDb::STexture *_pTexture,
	              const std::wstring &_wszTooltip,
	              const std::string &_szVectorImage = std::string() )
		: pTexture( _pTexture )
		, wszTooltip( _wszTooltip )
		, szVectorImage( _szVectorImage )
	{
	}
};

class CTextureViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS( CTextureViewer )
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const override;
};
