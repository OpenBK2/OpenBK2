#pragma once

#include "Nodes2TypeDefs.h"

namespace NDb
{
namespace NTypeDef
{

struct STypeDef;

class CTerminalTypesDescriptor : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTerminalTypesDescriptor );

	std::unordered_set<STypeDef*, SNodesHash> nonTerminalTypes;
public:
	bool IsTerminalType( STypeDef *pType ) const { return nonTerminalTypes.find( pType ) == nonTerminalTypes.end(); }
	void SetTypeToNonTerminal( STypeDef *pType ) { nonTerminalTypes.insert( pType ); }
};

}
}

