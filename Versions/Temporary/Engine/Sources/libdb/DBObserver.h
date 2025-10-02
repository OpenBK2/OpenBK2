#pragma once

class CDBID;

namespace NDb
{
/**
Интерфейс наблюдателя за изменениями в ресурсной системе.
Все имена объектов передаются в формате описанном в
http://jabberwocky/wiki/ResourceSystem/ObjectIdentifier
*/

interface IDbObserver: public CObjectBase
{
	//! Сигнал на изменение объекта. Вызывается после изменения.
	virtual void ObjectChanged( const CDBID &dbid ) = 0;
	//! Сигнал на создание объекта. Вызывается после создания.
	virtual void ObjectAdded( const CDBID &dbid ) = 0;
	//! Сигнал на удаление объекта. Вызывается до удаления.
	virtual void ObjectRemoved( const CDBID &dbid ) = 0;
	//! Сигнал на перемещение объекта. Вызывается до перемещения объекта.
	virtual void ObjectMoved( const CDBID &dbidSrc, const CDBID &dbidDst ) = 0;
	//! Сигнал на запись всех изменений. Вызывается после записи изменений.
	virtual void SaveAllChanges() = 0;
	//! Сигнал на откат всех non-saved изменений. Вызывается после отказа.
	virtual void DiscardAllChanges() = 0;
};

}
