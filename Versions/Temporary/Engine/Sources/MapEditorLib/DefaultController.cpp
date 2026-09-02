#include "stdafx.h"

#include "Interface_View.h"
#include "DefaultController.h"

bool CDefaultController::Undo(  bool bUpdateManipulator, bool bUpdateViews, IView *pViewToExlude )
{
	// самостоятельно выполнить команду
	bool bResult = true;
	if ( bUpdateManipulator )
	{ 
		bResult = UndoWithoutUpdateViews();
	}
	// Обновить Views
	if ( bResult &&	bUpdateViews )
	{
		CViewSet viewSet;
		Singleton<IViewContainer>()->GetViewSet( &viewSet, objectSet, pViewToExlude );
		for ( CViewSet::iterator itView = viewSet.begin(); itView != viewSet.end(); ++itView )
		{
			itView->first->Undo( this );
		}
	}
	return bResult;
}


bool CDefaultController::Redo(  bool bUpdateManipulator, bool bUpdateViews, IView *pViewToExlude )
{
	// самостоятельно выполнить команду
	bool bResult = true;
	if ( bUpdateManipulator )
	{
		bResult = RedoWithoutUpdateViews();
	}
	// Обновить Views
	if ( bResult && bUpdateViews )
	{
		CViewSet viewSet;
		Singleton<IViewContainer>()->GetViewSet( &viewSet, objectSet, pViewToExlude );
		for ( CViewSet::iterator itView = viewSet.begin(); itView != viewSet.end(); ++itView )
		{
			itView->first->Redo( this );
		}
		return true;
	}
	return false;
}


void CDefaultController::GetNameListToUpdate( IManipulator::CNameMap *pNameMap, const IManipulator::CNameMap &rManipulatorNameMap, const std::string &rszName ) const
{
	if ( pNameMap )
	{
		// если мы что-то маскируем
		if ( !rManipulatorNameMap.empty() )
		{
			// попробуем найти все полные пути
			for ( IManipulator::CNameMap::const_iterator posName = nameMap.begin(); posName != nameMap.end(); ++posName )
			{
				( *pNameMap )[posName->first + rszName] = 0;
			}
			// это может быть прямая команда
			for ( IManipulator::CNameMap::const_iterator posName = rManipulatorNameMap.begin(); posName != rManipulatorNameMap.end(); ++posName )
			{
				if ( rszName.compare( 0, posName->first.size(), posName->first ) == 0 )
				{
					const std::string szName = rszName.substr( posName->first.size() );
					( *pNameMap )[szName] = 0;
				}
			}
			// это может быть команда от маскировочного манипулятора
			( *pNameMap )[rszName] = 0;
		}
		// если мы ничего не маскируем
		else
		{
			if ( nameMap.empty() )
			{
				( *pNameMap )[rszName] = 0;
			}
			else
			{
				for ( IManipulator::CNameMap::const_iterator posName = nameMap.begin(); posName != nameMap.end(); ++posName )
				{
					( *pNameMap )[posName->first + rszName] = 0;
				}
			}
		}
		/**
		for ( IManipulator::CNameMap::const_iterator posName = pNameMap->begin(); posName != pNameMap->end(); ++posName )
		{
			DebugTrace( "GetNameListToUpdate: <%s>", posName->first.c_str() );
		}
		/**/
	}
}

// basement storage  


