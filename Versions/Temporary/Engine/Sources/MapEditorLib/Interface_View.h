#if !defined(__INTERFACE__VIEW__)
#define __INTERFACE__VIEW__
#pragma once

#include "Interface_Controller.h"
#include "../libdb/Manipulator.h"
#include "..\Misc\HashFuncs.h"

#define VIEW_COLLECTION_ID ("_VIEW_COLLECTION_ID_")

// Те, кто создает Undo Operation могут самостоятельно выполнить их снова
// Если такой цели нет, то команда должна сама о себе позаботится
// Controller, View
interface IView
{
	virtual ~IView() {}
	//
	//	Выполнить операции View после начала редактирования
	virtual void Enter() = 0;
	//	Выполнить операции View после конца редактирования
	virtual void Leave() = 0;
	// Выполнить Undo
	virtual void Undo( IController* pController ) = 0; 
	// Выполнить Redo
	virtual void Redo( IController* pController ) = 0; 
	// Обновить без изменения значений (вызывается при добавлении команды в IControllerContainer)
	//virtual void Update( IController* pController ) = 0; 
	//Установить манипулятор
	virtual void SetViewManipulator( IManipulator* _pViewManipulator,
																	 const SObjectSet &rObjectSet,
																	 const string &rszTemporaryLabel ) = 0;
	//Получить ранее установленный манипулятор
	virtual IManipulator* GetViewManipulator() = 0;
	//Убрать манипулятор
	virtual void RemoveViewManipulator() = 0;
	virtual void GetObjectSet( SObjectSet *pObjectDet ) = 0;
};

// CRAP{ HASH_SET
typedef hash_map<IView*, DWORD, SDefaultPtrHash> CViewSet;
// CRAP} HASH_SET


// Хранилище View
// rszObjectTypeName - тип View
// nOnbjectID - номер редактируемого объекта

interface IViewContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A380 };
	// Добавить target
	virtual void Add( IView *pView, const SObjectSet &rObjectSet ) = 0;
	// Удалить target
	virtual void Remove( IView *pView, const SObjectSet &rObjectSet ) = 0;
	// получить все зарегистрированные UndoTаrgets, за исключением указанного, указанный может быть 0
	// false	- если нет ни одного
	// true		- если есть хотя бы один
	// Если в качестве pViewSet указан 0 то просто возвращает наличие обработчиков
	virtual bool GetViewSet( CViewSet *pViewSet, const SObjectSet &rObjectSet, IView *pViewToExlude ) const = 0;
};

#endif // !defined(__INTERFACE__VIEW__)


