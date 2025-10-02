#pragma once

#include "TypeDef.h"

namespace NHungarian
{
const string GetTypePrefix( const NDb::NTypeDef::ETypeType eType, NDb::NTypeDef::SAttributes *pTypeAttributes );
const string GetTypeNameInCode( NDb::NTypeDef::STypeDef *pType, const NDb::NTypeDef::STypeStructBase::SField *pField );

bool ConvertToShortName( string *pszShortFieldName, const string &szFullFieldName, NDb::NTypeDef::ETypeType eType, NDb::NTypeDef::SAttributes *pTypeAttributes );
const string GetFieldNameInCode( const NDb::NTypeDef::STypeClass::SField &field );
}

