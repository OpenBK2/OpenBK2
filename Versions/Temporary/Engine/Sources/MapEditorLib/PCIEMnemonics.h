#pragma once

#include "Interface_PCItemEditor.h"
#include "Tools_MnemonicsCollector.h"


class CPCIEMnemonics : public CMnemonicsCollector<int>
{
	public:
	CPCIEMnemonics();
	EPCIEType Get( const SPropertyDesc *pDesc, bool bArrayNode );
	EPCIEType Get( const SPropertyDesc *pDesc, const std::string &rszName );
	bool IsPointer( EPCIEType nType );
	bool IsLeaf( EPCIEType nType );
	bool IsRef( EPCIEType nType );
	bool IsSingleRef( EPCIEType nType );
	bool IsMultiRef( EPCIEType nType );
};


EXTERNVAR CPCIEMnemonics typePCIEMnemonics;


