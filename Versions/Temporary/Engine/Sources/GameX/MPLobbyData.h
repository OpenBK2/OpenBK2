#pragma once
#include "InterfaceMPBase.h"
#include "UI/UIComponents.h"

enum EMPChatStatus;

class CClientListData : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CClientListData)	
public:
	ZDATA
	string szName;
	EMPChatStatus eStatus;
	CPtr<IListControlItem> pListItem;
	CPtr<IButton> pStatusIcon;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); f.Add(3,&eStatus); f.Add(4,&pListItem); f.Add(5,&pStatusIcon); return 0; }

	CClientListData() {}
	CClientListData( const string &_szName, const EMPChatStatus _eStatus ) : szName( _szName ), eStatus( _eStatus ) { };
};

class CClientListViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CClientListViewer)	
public:
	ZDATA
	CPtr<class CInterfaceMPLobby> pInterface;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pInterface); return 0; }
	CClientListViewer() {};
	CClientListViewer( class CInterfaceMPLobby *_pInterface ) : pInterface(_pInterface) {};
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
	void SetData (  CObjectBase *pWindow, const CObjectBase *pData  ) const;
};

class CChannelListViewer : public IDataViewer
{
	OBJECT_NOCOPY_METHODS(CChannelListViewer)
public:
	void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
};


