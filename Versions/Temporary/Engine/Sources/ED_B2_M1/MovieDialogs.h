#pragma once

#include "MapEditorLib/Interface_Widget.h"
#include "Stats_B2_M1/DBMapInfo.h"

#include <string>

// The script movie editor's two dialogs, behind a boundary that names no
// toolkit. The movies editor window opens the first from its Settings button;
// the second answers ID_MIMOVED_KEY_SETTINGS, which CScriptCameraState turns
// into an edit of the one selected key.
//
// Same shape as the other migrated dialogs: a call that blocks and answers
// whether OK was pressed. Two small dialogs for one editor do not need a
// header each.
namespace NMovieSettings
{
	// Asks for a movie's length in seconds, starting from *pfLength. True and
	// *pfLength set on OK; false and *pfLength untouched on Cancel. What does
	// not parse is 0, as it always was.
	bool Run( IWidget *pParent, float *pfLength );
}


namespace NMovieKeySettings
{
	// Asks for one movie key's tangents and parameter. rszName is shown and not
	// edited -- the MFC dialog's name box is read-only and disabled. On OK the
	// key's bIsTangentIn, bIsTangentOut and szKeyParam are set; on Cancel it is
	// untouched.
	bool Run( IWidget *pParent, NDb::SScriptMovieKeyPos *pKey, const std::string &rszName );
}
