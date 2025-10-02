#pragma once

#include "Interface_Controller.h"

// Последовательность создания объектов должна быть такова чтобы при удалении объектов сначала могли удалится более ранние объекты
// это значить, что объект на который ссылаются необходимо создавать позже
struct IFolderCallback : public CObjectBase
{
	enum { tidTypeID = 0x140A7000 };

	// lock and unlock objects for Remove (not for rename)
	virtual void LockObjects( const SObjectSet &rObjectSet ) = 0;
	virtual void UnockObjects( const SObjectSet &rObjectSet ) = 0;
	// define locked object
	virtual bool IsObjectLocked( const string &rszTypeName, const CDBID &rDBID ) const = 0;
	// Очистить буфер
	virtual void ClearUndoData() = 0;
	// Удалить созданные объекты объекты 
	virtual void UndoChanges() = 0;
	// Уникальное ли имя
	virtual bool IsUniqueName( const string &rszTypeName, const string &rszName ) = 0;
	// Сделать имя уникальным
	virtual bool UniqueName( const string &szTypeName, string *pszName ) = 0;
	// Создать объект в базе ( и в дереве )
	virtual bool InsertObject( const string &rszObjectTypeName, const string &rszObjectName ) = 0;
	// Скопировать объект
	virtual bool CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource ) = 0;
	// Переименовать объект
	virtual bool RenameObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource ) = 0;
	// Удалить объект из базы ( и из дерева )
	virtual bool RemoveObject( const string &rszObjectTypeName, const string &rszObjectName, bool bRecursive ) = 0;
	// Установить свойство объекта
	virtual bool SetColor( const string &rszObjectTypeName, const string &rszObjectName, const int nNewColor ) = 0;
};



