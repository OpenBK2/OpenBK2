#pragma once

#include "Interface_View.h"

class CDefaultView : public IView
{
	// Метка для последующего удаления Undo Operation из очереди
	std::string szTemporaryLabel;
	// Набор изменяемых объектов
	SObjectSet objectSet;
	// Манипулятордля изменений обьектов (может быть MultiManipulator)
	CPtr<IManipulator> pViewManipulator;

public:
	CDefaultView() {}
	~CDefaultView() { RemoveViewManipulator(); }
	//
	// Функция создания Controller
	template <class TController>
	TController* CreateController( TController *pTYPEObject )
	{
		if ( pTYPEObject == 0 )
		{
			pTYPEObject = new TController();
		}
		//
		if ( CDefaultController* pController = dynamic_cast<CDefaultController*>( pTYPEObject ) )
		{
			pController->objectSet = objectSet;
			pController->szTemporaryLabel = szTemporaryLabel;
			pController->GetNameList().clear();
			if ( pViewManipulator )
			{
				pViewManipulator->GetNameList( &( pController->GetNameList() ) );
			}
		}
		return pTYPEObject;
	}
	inline const SObjectSet& GetObjectSet() const { return objectSet; }
	inline const std::string& GetTemporaryLabel() const { return szTemporaryLabel; }
	//
	virtual void SetViewManipulator( IManipulator* _pViewManipulator,
																	 const SObjectSet &rObjectSet,
																	 const std::string &rszTemporaryLabel );
	virtual IManipulator* GetViewManipulator() { return pViewManipulator; }
	virtual void RemoveViewManipulator();
	//
	virtual void SetTemporaryLabel( const std::string &rszTemporaryLabel ) { szTemporaryLabel = rszTemporaryLabel; }
	virtual void GetTemporaryLabel( std::string *pszTemporaryLabel ) { if ( pszTemporaryLabel ) { ( *pszTemporaryLabel ) = szTemporaryLabel; } }
	//
	virtual void GetObjectSet( SObjectSet *pObjectSet ) { if ( pObjectSet != 0 ) { ( *pObjectSet ) = objectSet; } }
	virtual void Enter();
	virtual void Leave();
};



