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
		std::string szName;

		ZEND int operator&( IBinSaver &f ) override
		{
			f.Add( 2, &pRealType );
			f.Add( 3, &pAttributes );
			f.Add( 4, &szName );
			return 0;
		}

		STypedefType()
			: STypeDef( TYPE_TYPE_UNKNOWN )
		{
		}

		STypedefType( const std::string &_szName )
			: STypeDef( TYPE_TYPE_UNKNOWN )
			, szName( _szName )
		{
		}

		bool IsSimpleType() const override
		{
			return false;
		}

		int GetTypeSize() const override
		{
			return 0;
		}

		const std::string GetTypeName() const override
		{
			return szName;
		}

		//
		void ToString( std::string *pRes, const CVariant &value ) const override {}

		void FromString( CVariant *pRes, const std::string &szValue ) const override {}

		SAttributes *GetAttributes() const override
		{
			return pAttributes;
		}

		//
		int operator&( IXmlSaver &saver ) override
		{
			saver.AddTypedSuper( static_cast<STypeDef *>( this ) );
			saver.AddInPlace( "RealType", &pRealType );
			saver.AddInPlace( "Attributes", &pAttributes );
			saver.Add( "Name", &szName );

			return 0;
		}
	};

} // namespace NTypeDef
} // namespace NDb
