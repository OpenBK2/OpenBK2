#pragma once

#include "MapEditorLib/Interface_Widget.h"
#include "Main/MODs.h"

// The Open MOD dialog, behind a boundary that names no toolkit.
//
// Same shape as NSelectTables::Run and for the same reason: a modal dialog is a
// call that blocks until the user answers, not an object with a lifetime.
//
// It collapses two steps into one. The MFC call site was
//
//     COpenMODDialog dlg;
//     if ( dlg.DoModal() == IDOK ) { NMOD::SMOD mod; if ( dlg.GetMOD( &mod ) ) ... }
//
// -- accept, and then ask separately whether anything was actually chosen,
// which are the same question. Run answers it once: true means pMod was filled.
namespace NOpenMod
{
	// Runs the dialog modally over pParent. Returns true if the user chose a MOD
	// and accepted, with *pMod set to it; false on cancel, or on OK with nothing
	// valid selected, which the dialog does not allow anyway.
	bool Run( IWidget *pParent, NMOD::SMOD *pMod );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, NMOD::SMOD *pMod );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, NMOD::SMOD *pMod );
#endif
}
