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
	virtual bool IsObjectLocked( const std::string &rszTypeName, const CDBID &rDBID ) const = 0;
	// Очистить буфер
	virtual void ClearUndoData() = 0;
	// Удалить созданные объекты объекты 
	virtual void UndoChanges() = 0;
	// Уникальное ли имя
	virtual bool IsUniqueName( const std::string &rszTypeName, const std::string &rszName ) = 0;
	// Сделать имя уникальным
	virtual bool UniqueName( const std::string &szTypeName, std::string *pszName ) = 0;
	// Создать объект в базе ( и в дереве )
	virtual bool InsertObject( const std::string &rszObjectTypeName, const std::string &rszObjectName ) = 0;
	// Скопировать объект
	virtual bool CopyObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource ) = 0;
	// Переименовать объект
	virtual bool RenameObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource ) = 0;
	// Удалить объект из базы ( и из дерева )
	virtual bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName, bool bRecursive ) = 0;
	// Установить свойство объекта
	virtual bool SetColor( const std::string &rszObjectTypeName, const std::string &rszObjectName, const int nNewColor ) = 0;
};



