#pragma once

#include "Interface_View.h"
// CreateController below casts to it and writes its members.
#include "DefaultController.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CDefaultView : public IView
{
	// Метка для последующего удаления Undo Operation из очереди
	std::string szTemporaryLabel;
	// Набор изменяемых объектов
	SObjectSet objectSet;
	// Манипулятордля изменений обьектов (может быть MultiManipulator)
	CPtr<IManipulator> pViewManipulator;

public:
	CDefaultView() {}

	~CDefaultView() override
	{
		RemoveViewManipulator();
	}

	//
	// Функция создания Controller
	template <class TController>
	TController *CreateController( TController *pTYPEObject )
	{
		if ( pTYPEObject == 0 )
		{
			pTYPEObject = new TController();
		}
		//
		if ( CDefaultController *pController = dynamic_cast<CDefaultController *>( pTYPEObject ) )
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

	inline const SObjectSet &GetObjectSet() const
	{
		return objectSet;
	}

	inline const std::string &GetTemporaryLabel() const
	{
		return szTemporaryLabel;
	}

	//
	void SetViewManipulator( IManipulator *_pViewManipulator,
	                         const SObjectSet &rObjectSet,
	                         const std::string &rszTemporaryLabel ) override;

	IManipulator *GetViewManipulator() override
	{
		return pViewManipulator;
	}

	void RemoveViewManipulator() override;

	//
	virtual void SetTemporaryLabel( const std::string &rszTemporaryLabel )
	{
		szTemporaryLabel = rszTemporaryLabel;
	}

	virtual void GetTemporaryLabel( std::string *pszTemporaryLabel )
	{
		if ( pszTemporaryLabel )
		{
			( *pszTemporaryLabel ) = szTemporaryLabel;
		}
	}

	//
	void GetObjectSet( SObjectSet *pObjectSet ) override
	{
		if ( pObjectSet != 0 )
		{
			( *pObjectSet ) = objectSet;
		}
	}

	void Enter() override;
	void Leave() override;
};
