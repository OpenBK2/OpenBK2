#pragma once
//
namespace NScript
{

class CLUACallParam: public CObjectBase
{
public:
	enum EParamType { PT_POINTER, PT_INT, PT_FLOAT, PT_STRING, PT_UNDEFINED };
	//
	OBJECT_BASIC_METHODS( CLUACallParam );
	ZDATA
public:
	EParamType type;
	CPtr<CObjectBase> pObject;
	int nInt;
	float fFloat;
	std::string szString;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&type); f.Add(3,&pObject); f.Add(4,&nInt); f.Add(5,&fFloat); f.Add(6,&szString); return 0; }
	//
	CLUACallParam(): type( PT_UNDEFINED ) {}
	CLUACallParam( CObjectBase *_pObject ): type( PT_POINTER ), pObject( _pObject ) {}
	CLUACallParam( const std::string &_szString ): type( PT_STRING ), szString( _szString ) {}
	CLUACallParam( int _nInt ): type( PT_INT ), nInt( _nInt ) {}
	CLUACallParam( float _fFloat ): type( PT_FLOAT ), fFloat( _fFloat ) {}
};

void luaCallFunction( const std::string &szName, char *szParams, ... );
void luaCallFunction( const std::string &szName, const std::vector< CObj<CLUACallParam> > &params );
void luaMakeCallParamsVector( char *szParams, va_list *pL, std::vector< CObj<CLUACallParam> > *pParams );
void luaCreateCPtrVar( const std::string &szVarName, CObjectBase *pObj );

}
//

