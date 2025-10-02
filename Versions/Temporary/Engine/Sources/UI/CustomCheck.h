// CustomCheck.h: interface for the CCustomCheck class.
//



#pragma once
#include "IMessageReaction.h"

class CCheckRunScript : public IMessageCheck
{
	OBJECT_BASIC_METHODS(CCheckRunScript)
	CDBPtr<NDb::SCheckRunScript> pDesc;
public:
	CCheckRunScript() {  }
	int operator&( IBinSaver &ss );
	virtual int Check( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags  ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
};

class CCheckPreprogrammed : public IMessageCheck
{
	OBJECT_BASIC_METHODS(CCheckPreprogrammed)
		CDBPtr<NDb::SCheckPreprogrammed> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual int Check( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags  ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

};

class CCheckIsWindowEnabled : public IMessageCheck
{
	OBJECT_BASIC_METHODS(CCheckIsWindowEnabled)
		CDBPtr<NDb::SCheckIsWindowEnabled> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual int Check( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags  ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

};

class CCheckIsWindowVisible : public IMessageCheck
{
	OBJECT_BASIC_METHODS(CCheckIsWindowVisible)
	CDBPtr<NDb::SCheckIsWindowVisible> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual int Check( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags  ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

};

class CCheckIsTabActive : public IMessageCheck
{
	OBJECT_BASIC_METHODS(CCheckIsTabActive)
	CDBPtr<NDb::SCheckIsTabActive> pDesc;
public:
	int operator&( IBinSaver &ss );
	virtual int Check( interface IScreen *pScreen, interface IScriptWrapper *pScript, interface IProgrammedReactionsAndChecks *pProg, WORD wKeyboardFlags  ) const;
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

};


