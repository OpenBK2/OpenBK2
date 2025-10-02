#include "stdafx.h"

#include "Nodes2TypeDefs.h"
#include "TypeDef.h"
#include "../Parser/LangNode.h"

int SNodesHash::operator()( NLang::CLangNode *pNode ) const
{ 
	return reinterpret_cast<int>( pNode ); 
}

int SNodesHash::operator()( NDb::NTypeDef::STypeDef *pNode ) const
{
	return reinterpret_cast<int>( pNode );
}

