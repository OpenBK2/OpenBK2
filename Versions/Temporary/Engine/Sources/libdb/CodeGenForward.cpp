#include "stdafx.h"

#include "CodeGenForward.h"
#include "CodeGenMisc.h"
#include "Hungarian.h"
#include "StrStream.h"
#include "../Parser/LangNodesDefinitions.h"

namespace NCodeGen
{

CForwardDefinition::CForwardDefinition( NLang::CTypeNode *pTypeNode, const CNodes2TypeDefs &nodes2TypeDefs )
{
	CNodes2TypeDefs::const_iterator iter = nodes2TypeDefs.find( pTypeNode->GetRealType() );
	NI_VERIFY( iter != nodes2TypeDefs.end(), StrFmt( "can't find typedef for node %s", pTypeNode->GetName().c_str() ), return );
	pType = GetRealType( iter->second );
}

void CForwardDefinition::GenerateCode( SCodeStreams *pCode, const string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const string &szQualifiedName )
{
	if ( IsNoCode( pType->GetAttributes() ) )
		return;

	pCode->h << szTabs;
	if ( pType->eType == NDb::NTypeDef::TYPE_TYPE_ENUM )
		pCode->h << "enum ";
	else
		pCode->h << "struct ";

	pCode->h << NHungarian::GetTypeNameInCode( pType, 0 ) << ";" << endl;
}

}

using namespace NCodeGen;
REGISTER_SAVELOAD_CLASS( 0x301B6D04, CForwardDefinition );
