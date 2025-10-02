#include "stdafx.h"

#include "CodeGenField.h"
#include "CodeGenForward.h"
#include "CodeGenNamespace.h"
#include "CodeGenType.h"
#include "StrStream.h"
#include "Parser/LangNodesDefinitions.h"

namespace NCodeGen
{

CNamespace::CNamespace( NLang::CNamespace *pNM, const CNodes2TypeDefs &nodes2TypeDefs, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc )
{
	for ( NLang::CNamespace::TDefsListIter iter = pNM->DefsListBegin(); iter != pNM->DefsListEnd(); ++iter )
	{
		NLang::CLangNode *pDef = *iter;

		CDynamicCast<NLang::CTypeNode> pTypeNode = pDef;
		if ( pTypeNode )
		{
			if ( pTypeNode->IsForward() )
				definitions.push_back( new CForwardDefinition( pTypeNode, nodes2TypeDefs ) );
			else if ( CDynamicCast<NLang::CComplexTypeNode> pComplexTypeNode = pDef )
				definitions.push_back( new CTypeDefinition( pComplexTypeNode, nodes2TypeDefs, pTermTypesDesc ) );
			else if ( CDynamicCast<NLang::CEnumNode> pEnumNode = pDef )
				definitions.push_back( new CTypeDefinition( pEnumNode, nodes2TypeDefs ) );
		}
		else if ( CDynamicCast<NLang::CVariable> pVariableNode = pDef )
			definitions.push_back( new CFieldDefinition( pVariableNode, nodes2TypeDefs ) );
	}

	badIncludes = pNM->GetBadIncludes();
}

void CNamespace::GenerateCode( SCodeStreams *pCode, const string &szTabs, NDb::NTypeDef::STypeDef *pParentType, const string &szQualifiedName )
{
	for ( list< CObj<ICode> >::iterator iter = definitions.begin(); iter != definitions.end(); ++iter )
		(*iter)->GenerateCode( pCode, szTabs + "\t", pParentType, szQualifiedName );

	if ( !badIncludes.empty() )
	{
		pCode->h << endl;
		for ( list<string>::iterator iter = badIncludes.begin(); iter != badIncludes.end(); ++iter )
			pCode->h << szTabs << tab << "#include " << qcomma << *iter << qcomma << endl;
	}
}

}

using namespace NCodeGen;
REGISTER_SAVELOAD_CLASS( 0x301B6D01, CNamespace );

