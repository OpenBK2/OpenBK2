#pragma once

namespace NML
{

class IML;
class IReflowLayout;

// CMLStream

class CMLStream: public CObjectBase
{
	OBJECT_BASIC_METHODS(CMLStream)
private:
	ZDATA
	int nPos;
	std::wstring wsText;
	int nSize;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nPos); f.Add(3,&wsText); f.Add(4,&nSize); return 0; }

public:
	CMLStream(): nPos(0), nSize(0) {}

	int GetSeek() const { return nPos; }
	void Seek( int nSeek ) { ASSERT( nSeek <= wsText.length() ); nPos = nSeek; }
	void IncSeek() { ++nPos; }

	wchar_t GetChar() const { return wsText[nPos]; }
	bool IsEof() const { return nPos >= nSize; }
	void GetString( int nStart, int nSize, std::wstring *pRes );
	void InsertString( const std::wstring &wsText );
};

// IHandler

class IHandler: public CObjectBase
{
public:
	virtual void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet ) = 0;
};

// IML

class IML: public CObjectBase
{
public:
	virtual void SetText( const std::wstring &text ) = 0;
	virtual void SetHandler( const std::wstring &tag, IHandler *pHandler ) = 0;

	virtual void Generate( NGScene::ILayoutFakeView *pView, int nWidth ) = 0;

	virtual void Render( NGScene::ILayoutFakeView *pView, const CTPoint<float> &position, const CTRect<float> &window ) = 0;
	virtual void Render( std::list<CTRect<float> > *pRender, const CTPoint<float> &position, const CTRect<float> &window ) = 0;

	virtual const CTPoint<int>& GetSize() const = 0;
};

IML* CreateML();

} // Namespace


