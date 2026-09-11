#pragma once

#include "MapEditorLib/Interface_Widget.h"
#include "Stats_B2_M1/DBMapInfo.h"

// The AI general palette's two modal dialogs, behind a boundary that names no
// toolkit. CAIGeneralPointsState opens both when the palette reports an action:
// Add under the mobile reinforcements list asks for a script ID, and editing a
// parcel asks for its type and importance.
//
// Same shape as NSearchObject::Run and the other migrated dialogs: a modal
// dialog is a call that blocks until the user answers, not an object with a
// lifetime. Both are here together because they are two small dialogs serving
// one state, and a header each would be three files per fifty lines.
namespace NAIGenMobileDialog
{
	// Asks for a mobile reinforcement's script ID, starting from *pMobileID.
	// True and *pMobileID set on OK; false and *pMobileID untouched on Cancel.
	// What does not parse as a number is 0, as it always was.
	bool Run( IWidget *pParent, int *pMobileID );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, int *pMobileID );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, int *pMobileID );
#endif
}


namespace NAIGenParcelDialog
{
	// Asks for a parcel's type and importance, starting from what is passed in.
	// True and both set on OK; false and both untouched on Cancel.
	bool Run( IWidget *pParent, NDb::EParcelType *pType, float *pImportance );

	// Named so the dispatcher can reach them; not for anything else to call.
	bool RunMfc( IWidget *pParent, NDb::EParcelType *pType, float *pImportance );
#ifdef OBK2_WITH_WX
	bool RunWx( IWidget *pParent, NDb::EParcelType *pType, float *pImportance );
#endif
}
