#pragma once

#include "../Main/AICmdsAutoMagicInterface.h"

class CAICmdsAutomagic : public IAICmdsAutoMagic
{
	OBJECT_BASIC_METHODS( CAICmdsAutomagic );

	ZDATA
		hash_map<int, BYTE> msg2byte;
	vector<int> byte2msg;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&msg2byte); f.Add(3,&byte2msg); return 0; }
public:	
	CAICmdsAutomagic();
	virtual int GetCommandID( CObjectBase *p );
	virtual CObjectBase *MakeCommand( int nID );
	virtual int GetIDSize() const { return 1; }
};
IAICmdsAutoMagic *CreateAICmdsAutoMagic();


