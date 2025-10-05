#pragma once
#include "InterfaceMPBase.h"
#include "UI/Background.h"

namespace NMPSetData
{
	void SetText( ITextView *pText, const std::wstring &szText );
	void SetNum( ITextView *pWindow, int nText );
	void SetChildText( IListControlItem *pItem, const std::string &szName, const std::wstring &szText );
};

class CColorData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CColorData )
public:
	ZDATA
		int nColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nColor); return 0; }

	CColorData() {}
	CColorData( int _nColor ) : nColor ( _nColor ) { };
};

class CColorViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CColorViewer)
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
};

class CColorBackground: public CBackground
{
	OBJECT_BASIC_METHODS(CColorBackground);		
public:
	int nColor;
	virtual void Visit( struct IUIVisitor * pVisitor );
};

class CTextData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextData )
public:
	ZDATA
		std::wstring wszText;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszText); return 0; }

	CTextData() {}
	CTextData( const std::wstring &_wszText ) : wszText(_wszText) {}
	CTextData( int i );
};
//--------------
class CTextDataViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CTextDataViewer)
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
};

class CTextureData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTextureData )
public:
	ZDATA
	CDBPtr<NDb::STexture> pTexture;
	std::wstring wszTooltip;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTexture); f.Add(3,&wszTooltip); return 0; }

	CTextureData() {}
	CTextureData( const NDb::STexture *_pTexture, const std::wstring &_wszTooltip ) : pTexture( _pTexture ), wszTooltip(_wszTooltip) { };
};

class CTextureViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CTextureViewer)
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
};


