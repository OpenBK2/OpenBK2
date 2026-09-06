#pragma once

#include "MapEditorLib_export.h"
#include "Misc/Geom.h"

#include <string>
#include <vector>

struct IXmlSaver;
class IBinSaver;

// What a dialog remembers between sessions: where it was and how big, plus
// whatever it wants to carry itself -- Open MOD keeps the last MOD chosen, New
// Object keeps a checkbox.
//
// This used to be CResizeDialog::SOptions, a nested struct on an MFC class, and
// it is out here because the wx dialogs need the same thing and must have it
// **in the same file with the same format**. Two definitions of one on-disk
// format drift, and a user who switched between the MFC and wx implementations
// would silently lose their remembered state at the boundary. CResizeDialog now
// typedefs SOptions to this, so nothing that used it had to change.
//
// A word about "IntParameterss". The doubled s is in the on-disk format and has
// been since 2005. It is not a typo to fix: correcting it would orphan every
// existing Editor/ResizeDialogStyles/*.xml the moment someone upgraded.
struct MAPEDITORLIB_EXPORT SDialogState
{
	CTRect<int> rect;
	std::vector<int> nParameters;
	std::vector<std::string> szParameters;
	std::vector<float> fParameters;

	SDialogState() : rect( 0, 0, 0, 0 ) {}
	virtual ~SDialogState() {}

	virtual int operator&( IBinSaver &bs );
	virtual int operator&( IXmlSaver &xs );

	// nParameters is what most dialogs use, and every one of them opens by
	// making sure the slot exists. Saying it once is less error prone than
	// resize( n, 0 ) at each call site.
	int GetIntParameter( size_t nIndex, int nDefault = 0 ) const
	{
		return nIndex < nParameters.size() ? nParameters[nIndex] : nDefault;
	}

	void SetIntParameter( size_t nIndex, int nValue )
	{
		if ( nParameters.size() <= nIndex )
		{
			nParameters.resize( nIndex + 1, 0 );
		}
		nParameters[nIndex] = nValue;
	}
};


// Load and save go to Editor/ResizeDialogStyles/<name>.xml under the install,
// which is where CResizeDialog has always put them. rszDialogName is the label
// CResizeDialog's GetXMLFilePath answers with -- the dialog's class name, by
// convention and by the DECLARE_RESIZE_DLG_WND_COMMON_METHODS macro.
namespace NDialogState
{
	MAPEDITORLIB_EXPORT bool Load( const std::string &rszDialogName, SDialogState *pState );
	MAPEDITORLIB_EXPORT bool Save( const std::string &rszDialogName, SDialogState *pState );
}
