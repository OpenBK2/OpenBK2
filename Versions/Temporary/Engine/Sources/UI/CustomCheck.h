// CustomCheck.h: interface for the CCustomCheck class.
//


#if !defined(AFX_CUSTOMCHECK_H__4B1CA8EE_B9DF_40E8_A49E_866FE4D81D91__INCLUDED_)
#define AFX_CUSTOMCHECK_H__4B1CA8EE_B9DF_40E8_A49E_866FE4D81D91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#endif // !defined(AFX_CUSTOMCHECK_H__4B1CA8EE_B9DF_40E8_A49E_866FE4D81D91__INCLUDED_)
