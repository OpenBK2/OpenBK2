#pragma once

#include "DialogData.h"
#include "MapEditorLib/DefaultTabWindow.h"

#include <string>

class CWnd;

// The building editor's point lists, behind a boundary that names no toolkit.
//
// There are five of them, one tab each -- smoke, fire, entrance and surface
// points, and damage levels -- built from one template and told apart by an
// instance ID, which is also how the states address them. That is what makes
// this palette a different shape from the others: the states do not talk to a
// list, they talk to CHID_POINTS_LIST_DIALOG and name the instance in the
// data, and one handler behind that ID finds the right list.
//
// The MFC palette did that with a static list of every CPointListDialog and
// whichever one was constructed last registered itself for all five. Here the
// lists register with NPointListView instead, and it owns the one registration
// and the lookup, so the MFC and wx lists share it rather than each carrying a
// copy.
namespace NPointListView
{
	// One list, whichever toolkit draws it: what the shared dispatch needs.
	class IPointList
	{
	public:
		virtual ~IPointList() {}

		virtual unsigned GetInstanceID() const = 0;
		// ID_WINDOW_GET_DIALOG_DATA and ID_WINDOW_SET_DIALOG_DATA, for the list
		// whose instance ID the data names.
		virtual void GetDialogData( SPointListDialogData *pData ) = 0;
		virtual void SetDialogData( const SPointListDialogData *pData ) = 0;
		// Setting one list's data moves every other list's season selection to
		// match -- the season is the building's, not the list's.
		virtual void FollowSeason( NDb::ESeason eSeason ) = 0;
	};

	// Called by a list as it is constructed and destroyed. While any list is
	// registered, CHID_POINTS_LIST_DIALOG answers for all of them.
	void Register( IPointList *pList );
	void Unregister( IPointList *pList );
	// Every registered list except pSource follows eSeason.
	void FollowSeason( IPointList *pSource, NDb::ESeason eSeason );

	// Creates the list for nInstanceID inside pTabWindow, registers it in the
	// tab list, and returns it ready to be handed to AddTab. rszLabel names its
	// rows: "<label>   0", "<label>   1", ... Null if it could not be created.
	CWnd* Create( CDefault3DTabWindow *pTabWindow, unsigned nInstanceID, const std::string &rszLabel );

	// Named so the factory can reach them; not for anything else to call.
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow, unsigned nInstanceID, const std::string &rszLabel );
#ifdef OBK2_WITH_WX
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow, unsigned nInstanceID, const std::string &rszLabel );
#endif
}
