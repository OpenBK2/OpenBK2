#pragma once

namespace NML
{

// CBRHandler

struct SFadeValue: public CObjectBase
{
	OBJECT_BASIC_METHODS( SFadeValue )
	ZDATA
public:
	float fFadeValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fFadeValue); return 0; }
	SFadeValue(): fFadeValue( 1.0f ){}
	SFadeValue( float _fFadeValue ): fFadeValue( _fFadeValue ){}
};
class CBRHandler: public IHandler
{
	OBJECT_BASIC_METHODS(CBRHandler)
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet );
};

// CTabHandler

class CTabHandler: public IHandler
{
	OBJECT_BASIC_METHODS(CTabHandler)
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet );
};

// CColorHandler

class CColorHandler: public IHandler
{
	OBJECT_BASIC_METHODS(CColorHandler)
private:
	ZDATA
	CPtr<SFadeValue> sFadeValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&sFadeValue); return 0; }

public:
	CColorHandler(){}
	CColorHandler( SFadeValue* pFadeValue ): sFadeValue( pFadeValue ){}
	void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet );
};

// CFontHandler

class CFontHandler: public IHandler
{
	OBJECT_BASIC_METHODS(CFontHandler)
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet );
};

// CMinFontSizeHandler

class CMinFontSizeHandler: public IHandler
{
	OBJECT_BASIC_METHODS(CMinFontSizeHandler)
private:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet );
};

#define DECALRE_SIMPLE_HANDLER( Name, Fnc, Value )\
class Name: public IHandler												\
{																									\
	OBJECT_BASIC_METHODS( Name );										\
private:																					\
	int operator&( IBinSaver &f ) { return 0; }			\
public:																						\
	void Exec( CMLStream *pStream, IReflowLayout *pLayout, const std::vector<std::wstring> &paramsSet )	\
	{																								\
		pLayout->AddObject( Fnc( Value ) );						\
	}																								\
};

DECALRE_SIMPLE_HANDLER( CLEFTHandler, CreateHAlignObject, EHA_LEFT )
DECALRE_SIMPLE_HANDLER( CRIGHTHandler, CreateHAlignObject, EHA_RIGHT )
DECALRE_SIMPLE_HANDLER( CCENTERHandler, CreateHAlignObject, EHA_CENTER )
DECALRE_SIMPLE_HANDLER( CNOWRAPHandler, CreateHAlignObject, EHA_NOWRAP )
DECALRE_SIMPLE_HANDLER( CJUSTIFYHandler, CreateHAlignObject, EHA_JUSTIFY )
DECALRE_SIMPLE_HANDLER( CWRAPLEFTHandler, CreateHAlignObject, EHA_WRAP_LEFT )
DECALRE_SIMPLE_HANDLER( CWRAPRIGHTHandler, CreateHAlignObject, EHA_WRAP_RIGHT )
DECALRE_SIMPLE_HANDLER( CTOPHandler, CreateVAlignObject, EVA_TOP )
DECALRE_SIMPLE_HANDLER( CMIDDLEHandler, CreateVAlignObject, EVA_MIDDLE )
DECALRE_SIMPLE_HANDLER( CBOTTOMHandler, CreateVAlignObject, EVA_BOTTOM )

} // NAMESPACE


