#pragma once

#include "TypeDef.h"

namespace NCodeGen
{
	bool IsNoCode( const NDb::NTypeDef::SAttributes *pAttributes );
	bool IsNoCode( const NDb::NTypeDef::STypeStructBase::SField &field );
	NDb::NTypeDef::STypeDef* GetRealType( NDb::NTypeDef::STypeDef *pType );
}

