#pragma once

#include "TypeDef.h"

namespace NDb
{
namespace NTypeDef
{
// helper type to build types structure
struct STypedefType : public STypeDef
{
	OBJECT_NOCOPY_METHODS( STypedefType );
public:
	ZDATA
		CObj<STypeDef> pRealType;
		CObj<SAttributes> pAttributes;
		string szName;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pRealType); f.Add(3,&pAttributes); f.Add(4,&szName); return 0; }

	STypedefType() : STypeDef( TYPE_TYPE_UNKNOWN ) { }
	STypedefType( const string &_szName ) : STypeDef( TYPE_TYPE_UNKNOWN ), szName( _szName ) { }

	virtual bool IsSimpleType() const { return false; }
	virtual int GetTypeSize() const { return 0; }
	virtual const string GetTypeName() const { return szName; }
	//
	virtual void ToString( string *pRes, const CVariant &value ) const {  }
	virtual void FromString( CVariant *pRes, const string &szValue ) const {  }
	virtual SAttributes* GetAttributes() const { return pAttributes; }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.AddTypedSuper( static_cast<STypeDef*>(this) );
		saver.AddInPlace( "RealType", &pRealType );
		saver.AddInPlace( "Attributes", &pAttributes );
		saver.Add( "Name", &szName );

		return 0;
	}
};

}
}
