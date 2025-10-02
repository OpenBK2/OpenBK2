#pragma once

namespace NDb
{
interface IDbObserver;

class CDbObserverContainer
{
	typedef list< CPtr<IDbObserver> > CObserversList;
	CObserversList observers;
protected:
	void ReportObjectChanged( const CDBID &dbid );
	void ReportObjectAdded( const CDBID &dbid );
	void ReportObjectRemoved( const CDBID &dbid );
	void ReportObjectMoved( const CDBID &dbidSrc, const CDBID &dbidDst );
	void ReportSaveAllChanges();
	void ReportDiscardAllChanges();
public:
	void AddDbObserver( IDbObserver *pObserver ) { observers.push_back( pObserver ); }
};

}

