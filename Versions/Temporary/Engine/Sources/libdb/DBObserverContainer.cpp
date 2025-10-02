#include "StdAfx.h"
#include "DBObserverContainer.h"
#include "DBObserver.h"

namespace NDb
{

void CDbObserverContainer::ReportObjectChanged( const CDBID &dbid )
{
	for ( CObserversList::iterator it = observers.begin(); it != observers.end(); ++it )
		(*it)->ObjectChanged( dbid );
}

void CDbObserverContainer::ReportObjectAdded( const CDBID &dbid )
{
	for ( CObserversList::iterator it = observers.begin(); it != observers.end(); ++it )
		(*it)->ObjectAdded( dbid );
}

void CDbObserverContainer::ReportObjectRemoved( const CDBID &dbid )
{
	for ( CObserversList::iterator it = observers.begin(); it != observers.end(); ++it )
		(*it)->ObjectRemoved( dbid );
}

void CDbObserverContainer::ReportObjectMoved( const CDBID &dbidSrc, const CDBID &dbidDst)
{
	for ( CObserversList::iterator it = observers.begin(); it != observers.end(); ++it )
		(*it)->ObjectMoved( dbidSrc, dbidDst );
}

void CDbObserverContainer::ReportSaveAllChanges()
{
	for ( CObserversList::iterator it = observers.begin(); it != observers.end(); ++it )
		(*it)->SaveAllChanges();
}

void CDbObserverContainer::ReportDiscardAllChanges()
{
	for ( CObserversList::iterator it = observers.begin(); it != observers.end(); ++it )
		(*it)->DiscardAllChanges();
}

}
