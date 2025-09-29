#if !defined(__INTERFACE__EDITOR__)
#define __INTERFACE__EDITOR__
#pragma once

#include "Interface_View.h"
#include "Interface_InputState.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// в cpp файле написать макрос: REGISTER_EDITOR_IN_...( typeName, className )

// При создании редактора
// Constructor()
// Create()
// GetView->SetViewManipulator();
// GetInputState->Enter();

// При разрушении редактора
// GetInputState->Leave();
// GetView->RemoveViewManipulator();
// Destroy()
// Destructor()
interface IEditor : public CObjectBase
{
	// передается в View
	virtual void GetTemporaryLabel( string *pszTemporaryLabel ) = 0;
	// получить тип child frame
	virtual void GetChildFrameType( string *pszChildFrameTypeName ) = 0;
	// получить непосредственно - редактор.
	virtual IView* GetView() = 0;
	// получить непосредственно - IInputState
	virtual IInputState* GetInputState() = 0;
	// создать дополнительные контролы ( вызывается при создании MainFrame до считывания положения панелей)
	virtual void CreateControls() = 0;
	// создать дополнительные контролы ( вызывается при создании MainFrame после считывания панелей)
	virtual void PostCreateControls() = 0;
	// удалить дополнительные контролы ( вызывается при удалении MainFrame до запоминания положения панелей)
	virtual void PreDestroyControls() = 0;
	// удалить дополнительные контролы ( вызывается при удалении MainFrame после запоминания положения панелей)
	virtual void DestroyControls() = 0;
	// Создать общую часть редактора (не объект-специфичную)
	virtual void Create() = 0;
	// Удалить общую часть редактора (не объект-специфичную)
	virtual void Destroy() = 0;
	// Вызывается из интерфейса по кнопке Save
	virtual void Save( bool bSaveChanges ) = 0;
	// Определяет можно ли нажимать Save
	virtual bool IsModified() = 0;
	// Модифицирует состояние заголовка
	virtual void SetModified( bool bModified ) = 0;
	// Показывать ли прогресс диалог автоматически
	virtual bool ShowProgress() = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IEditorContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A3C0 };
	//
	// Проверить на существование редактора
	virtual bool CanCreate( const string &rszObjectTypeName ) = 0;
	// Методы создания и удаления редакторов
	virtual void Create( const string &rszObjectTypeName ) = 0;
	virtual void AddExtendObjectType( const string &rszBaseObjectTypeName, const string &rszExtendObjectTypeName ) = 0;
	virtual void Destroy( const string &rszObjectTypeName, bool bDestroyChildFrame ) = 0;
	// Получить редактор
	virtual IEditor* Create( IManipulator* _pManipulator, const SObjectSet &rObjectSet ) = 0;
	// Перегрузить редактор ( тот которому сделали Enter )
	virtual void ReloadActiveEditor( bool bClearResources ) = 0;
	// Закрыть активный редактор ( тот которому сделали Enter )
	virtual void DestroyActiveEditor( bool bDestroyChildFrame ) = 0;
	// Получить активный редактор
	virtual IEditor* GetActiveEditor() = 0;
	// Получить IInputState куда отсылать сообщения из DirectX окна
	virtual IInputState* GetActiveInputState() = 0;
	//
	// Специальные методы, вызываются из MainFrame
	// Вызвать методы CreateControls() у всех зарегистрированныз редакторов
	virtual void CreateControls() = 0;
	virtual void PostCreateControls() = 0;
	// Вызвать методы DestroyControls() у всех зарегистрированныз редакторов
	virtual void PreDestroyControls() = 0;
	virtual void DestroyControls() = 0;
	// Вызывается из интерфейса по кнопке Save
	virtual void Save( bool bSaveChanges ) = 0;
	// Определяет можно ли нажимать Save
	virtual bool IsModified() = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__INTERFACE__EDITOR__)

