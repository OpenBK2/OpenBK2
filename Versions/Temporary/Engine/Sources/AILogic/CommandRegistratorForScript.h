#pragma once


class CCommandRegistratorForScript
{
	typedef hash_map<int,bool> CRegisteredCommands;
	ZDATA
		CRegisteredCommands registeredCommands;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&registeredCommands); return 0; }
private:
public:
	void Register( int nCommand );
	void Called( int nCommand );
	void Clear() { registeredCommands.clear(); }
};


