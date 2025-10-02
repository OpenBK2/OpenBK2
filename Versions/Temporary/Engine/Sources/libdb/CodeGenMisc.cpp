#include "stdafx.h"

#include "CodeGenMisc.h"
#include "TerminalTypesDesc.h"
#include "TypeDef.h"
#include "TypeDefType.h"
#include "Variant.h"

namespace NCodeGen
{

bool IsNoCode( const NDb::NTypeDef::SAttributes *pAttributes )
{
	if ( pAttributes )
	{
		const hash_map<string, CVariant> &attributes = pAttributes->attributes;
		return attributes.find( "noCode" ) != attributes.end();
	}

	return false;
}

bool IsNoCode( const NDb::NTypeDef::STypeStructBase::SField &field )
{
	if ( IsNoCode( field.pAttributes ) )
		return true;

	CDynamicCast<NDb::NTypeDef::STypeStructBase> pStructType = field.pType;
	if ( pStructType && IsNoCode( pStructType->pAttributes ) )
		return true;

	return false;
}

NDb::NTypeDef::STypeDef* GetRealType( NDb::NTypeDef::STypeDef *pType )
{
	if ( CDynamicCast<NDb::NTypeDef::STypedefType> pTypeDefNode = pType )
		return pTypeDefNode->pRealType;
	else
		return pType;
}

}

