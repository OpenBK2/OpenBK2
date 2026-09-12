#include "stdafx.h"

#include "ReinfPointsDialogs.h"

#include <fmt/format.h>

// The database half of the typed templates dialog: what its Add and Remove
// buttons do to the map, with no toolkit in sight, so the MFC dialog and the wx
// one run the same code rather than a copy each.
//
// It lives here and not in either implementation because the interesting part
// is a sequence, not a call: Add inserts a node before the dialog that fills it
// in is shown, so cancelling that dialog has to take the node out again. Two
// copies of that would be two chances for the rollback to name the wrong index,
// which is exactly what the original did -- see below.

namespace
{
	// Where a reinforcement point's typed templates live in the map.
	std::string TemplatesNode( int nPlayer, int nReinfPoint )
	{
		return fmt::format( "Players.[{}].ReinforcementPoints.[{}].TypedTemplates", nPlayer, nReinfPoint );
	}


	// How many are there now. The array's own node answers with its size.
	int TemplateCount( CMapInfoEditor *pMapInfoEditor, const std::string &rszNode )
	{
		int nCount = 0;
		CManipulatorManager::GetValue( &nCount, pMapInfoEditor->GetViewManipulator(), rszNode );
		return nCount;
	}


	// One undoable operation, applied and pushed on the editor's undo stack.
	// Both buttons do exactly this, twice over between them.
	bool ApplyRemove( CMapInfoEditor *pMapInfoEditor, const std::string &rszNode, int nIndex )
	{
		CPtr<CObjectBaseController> pObjectController = new CObjectController;
		if ( !pObjectController->AddRemoveOperation( rszNode, nIndex, pMapInfoEditor->GetViewManipulator() ) )
		{
			return false;
		}
		pObjectController->Redo( false, true, 0 );
		Singleton<IControllerContainer>()->Add( pObjectController );
		return true;
	}
}


namespace NReinfPointsTemplates
{
	bool Add( IWidget *pParent, CReinfPointsState::CTypedTemplateType *pTemplates,
						CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint )
	{
		if ( pTemplates == 0 || pMapInfoEditor == 0 )
		{
			return false;
		}
		const std::string szTemplates = TemplatesNode( nPlayer, nReinfPoint );

		// The node has to exist before the dialog can edit it: the dialog reads
		// and writes the map through the manipulator rather than carrying values
		// of its own.
		CPtr<CObjectBaseController> pObjectController = new CObjectController;
		if ( !pObjectController->AddInsertOperation( szTemplates, NODE_ADD_INDEX,
																								 pMapInfoEditor->GetViewManipulator() ) )
		{
			return false;
		}
		pObjectController->Redo( false, true, 0 );
		Singleton<IControllerContainer>()->Add( pObjectController );

		// Appended, so it is the last one.
		const int nAddedIndex = TemplateCount( pMapInfoEditor, szTemplates ) - 1;
		const std::string szAdded = szTemplates + fmt::format( ".[{}]", nAddedIndex );

		if ( NReinfPointsAddTemplate::Run( pParent, szAdded, pMapInfoEditor ) )
		{
			// Read back what the dialog wrote, rather than trusting a copy of it.
			CReinfPointsState::STypedTemplate newTemplate;
			CManipulatorManager::GetValue( &newTemplate.szTemplateType, pMapInfoEditor->GetViewManipulator(),
																		 szAdded + ".Type" );
			CManipulatorManager::GetValue( &newTemplate.szTemplate, pMapInfoEditor->GetViewManipulator(),
																		 szAdded + ".Template" );
			pTemplates->push_back( newTemplate );
			return true;
		}

		// Cancelled, so the node goes back out, by the index it went in at.
		//
		// The original asked for pTypedTemplateDlgData->size() + 1, which is one
		// past the last node -- and got the right one anyway, because
		// CObjectBaseController::AddRemoveOperation clamps an index at or past
		// the end to the last node, which is the one just appended. It works;
		// nothing about it says so. This asks for the node it created.
		ApplyRemove( pMapInfoEditor, szTemplates, nAddedIndex );
		return false;
	}


	void Remove( CReinfPointsState::CTypedTemplateType *pTemplates,
							 CMapInfoEditor *pMapInfoEditor, int nPlayer, int nReinfPoint, int nSelected )
	{
		// The Remove button is only enabled with a row selected, but the index is
		// checked against the list as well: the caller's copy and the map are two
		// things, and this erases from one by an index taken from the other.
		if ( pTemplates == 0 || pMapInfoEditor == 0 ||
				 nSelected < 0 || nSelected >= static_cast<int>( pTemplates->size() ) )
		{
			return;
		}
		ApplyRemove( pMapInfoEditor, TemplatesNode( nPlayer, nReinfPoint ), nSelected );
		pTemplates->erase( pTemplates->begin() + nSelected );
	}
}
