#pragma once

#include "TypeDef.h"

namespace NHungarian
{
const std::string GetTypePrefix( const NDb::NTypeDef::ETypeType eType, NDb::NTypeDef::SAttributes *pTypeAttributes );
const std::string GetTypeNameInCode( NDb::NTypeDef::STypeDef *pType, const NDb::NTypeDef::STypeStructBase::SField *pField );

bool ConvertToShortName( std::string *pszShortFieldName, const std::string &szFullFieldName, NDb::NTypeDef::ETypeType eType, NDb::NTypeDef::SAttributes *pTypeAttributes );
const std::string GetFieldNameInCode( const NDb::NTypeDef::STypeClass::SField &field );
}


