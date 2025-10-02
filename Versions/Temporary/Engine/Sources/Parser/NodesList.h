#pragma once

#include "LangNode.h"

namespace NLang
{

template<class T>
class CNodesList : public CLangNode
{
	OBJECT_NOCOPY_METHODS( CNodesList );

	list< CPtr<T> > nodesList;
public:
	CNodesList() { }

	void AddNode( T *pNode ) { nodesList.push_back( pNode ); }

	const list< CPtr<T> >& GetNodes() const { return nodesList; }
	list< CPtr<T> >& GetNodes() { return nodesList; }
};

}


