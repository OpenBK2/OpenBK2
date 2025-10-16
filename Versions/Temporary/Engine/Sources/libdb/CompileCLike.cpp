#include "stdafx.h"

#include "CompileCLike.h"
#include "CompileCLikeVisitor.h"
#include "TerminalTypesDesc.h"
#include "TypeDef.h"
#include "Parser/ErrorsAndMessages.h"

#include <fmt/format.h>

namespace NCompileCLike
{

bool Compile( std::vector< CObj<NDb::NTypeDef::STypeDef> > *pTypes, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc,
						 CNodes2TypeDefs *pNodes2TypeDefs, NLang::CNamespace *pRootNN )
{
	CPtr<CVisitor> pVisitor = new CVisitor( pTypes, pTermTypesDesc );
	pVisitor->Visit( pRootNN );
	if ( pVisitor->IsFailed() )
		return false;

	int nCnt = 0;
	for ( int i = 0; i < pTypes->size(); ++i )
	{
		CPtr<NDb::NTypeDef::STypeDef> pTypeDef = (*pTypes)[i];
		if ( pTermTypesDesc->IsTerminalType( pTypeDef ) )
		{
			(*pTypes)[nCnt++] = pTypeDef;

			CDynamicCast<NDb::NTypeDef::STypeClass> pClass = pTypeDef;
			if ( pClass && pClass->nClassTypeID == -1 )
			{
				NErrors::ShowErrorNoLine( fmt::format( "attribute \"typeID\" in class {} doesn't exist", pClass->szTypeName ) );
				return false;
			}
		}
	}
	pTypes->resize( nCnt );
	*pNodes2TypeDefs = pVisitor->GetNodes2TypeDefs();

	return true;
}

}


