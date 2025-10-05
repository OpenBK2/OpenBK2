#pragma once

struct IML;

// window may have string of text
class CForegroundTextString : public IWindowPart
{
	OBJECT_BASIC_METHODS( CForegroundTextString )
	ZDATA
	std::wstring wszCustomText;
	CTRect<float> rcParent;
	CPtr<NDb::SForegroundTextString> pInstance;
	CObj<IML> pGfxText;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszCustomText); f.Add(3,&rcParent); f.Add(4,&pInstance); f.Add(5,&pGfxText); return 0; }
private:
	void InitText();
public:

	CForegroundTextString() {}

	virtual void Init();
	void SetText( const std::wstring &_szText );
	int GetOptimalWidth() const;
	const std::wstring &GetText() const;
	const std::wstring& GetDBInstanceText() const;
	const std::wstring& GetDBFormatText() const;
	const NDb::SWindowPlacement* GetPlacement() const;

	//IWindowPart{
	virtual void Visit( struct IUIVisitor *pVisitor );
	virtual void SetPos( const CVec2 &vPos, const CVec2 &vSize );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	void SetFadeValue( float fValue );
	void SetInternalFadeValue( float fValue );
	//IWindowPart}
};

// For internal use
class CPlacedText : public CObjectBase
{
	OBJECT_BASIC_METHODS( CPlacedText )

	ZDATA
	std::wstring wszText;
	NDb::SWindowPlacement placement;
	CTRect<float> rcParent;
	CObj<IML> pGfxText;
	CVec2 vScreenRect;
	float fFadeValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszText); f.Add(3,&placement); f.Add(4,&rcParent); f.Add(5,&pGfxText); f.Add(6,&vScreenRect); f.Add(7,&fFadeValue); return 0; }
private:
	void InitGfxText();
public:
	CPlacedText();
	
	void Init();

	void Visit( struct IUIVisitor *pVisitor );
	
	const std::wstring& GetText() const { return wszText; }
	void SetText( const std::wstring &wszText );

	int GetOptimalWidth() const;

	CTPoint<int> GetSize() const;

	void SetPlacement( const struct NDb::SWindowPlacement &placement );

	void SetPos();
	void Reposition( const CTRect<float> &parentRect );

	void SetInternalFadeValue( float fValue );
};


