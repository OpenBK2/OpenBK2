#pragma once

#include "Main/AICmdsAutoMagicInterface.h"

#include "GameX_export.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

class CAICmdsAutomagic : public IAICmdsAutoMagic
{
	OBJECT_BASIC_METHODS( CAICmdsAutomagic );

	ZDATA
	std::unordered_map<int, uint8_t> msg2byte;
	std::vector<int> byte2msg;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &msg2byte );
		f.Add( 3, &byte2msg );
		return 0;
	}

public:
	CAICmdsAutomagic();
	int GetCommandID( CObjectBase *p ) override;
	CObjectBase *MakeCommand( int nID ) override;

	int GetIDSize() const override
	{
		return 1;
	}
};

GAMEX_EXPORT IAICmdsAutoMagic *CreateAICmdsAutoMagic();
