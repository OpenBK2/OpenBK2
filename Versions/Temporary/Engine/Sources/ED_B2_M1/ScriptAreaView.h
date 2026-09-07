#pragma once

#include "PaletteCommands.h"
#include "MapEditorLib/DefaultTabWindow.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "libdb/Manipulator.h"

#include <string>
#include <vector>

class CWnd;

//
//
//		SCRIPT AREA WINDOW DATA
//
//

// What the palette and CScriptAreaState hand each other. It was in
// ScriptAreaWindow.h, which is an MFC header, and it is out here for the two
// reasons the other palettes' data types are beside their states: the wx
// palette needs it without MFC, and ScriptAreaState.h was including an MFC
// dialog header to get a plain struct.
struct SScriptAreaWindowData
{
	NDb::EScriptAreaTypes eAreaType;	// какая радио-кнопка выбрана в диалоге
	//
	struct SScriptArea	// данные для создания row в списке областей
	{
		std::string szName;
		int nScriptAreaID;
		NDb::EScriptAreaTypes eType;
		//
		SScriptArea() : nScriptAreaID( INVALID_NODE_ID ), eType( NDb::EAT_CIRCLE ) {}
	};
	std::vector<SScriptArea> scriptAreaList;				// содержимое лист-контрола
	std::vector<unsigned> selectedScriptAreaIDList;		// ID поселекченных областей
	//
	enum EChangeMask	// что изменилось ( GET ) или что нужно изменить в диалоге ( SET )
	{
		CHANGE_NONE				= 0x00000000,
		CHANGE_AREAS			= 0x00000001,	// обновить список областей ( SET )
		CHANGE_SELECTION	= 0x00000002,	// обновить selection ( GET+SET )
		CHANGE_DEL_SEL		= 0x00000004,	// удалить выделенные области ( GET )
		CHANGE_AREA_TYPE	= 0x00000008,	// какая радио-кнопка выбрана в диалоге ( GET + SET )
		CHANGE_SET_ALL		= ( CHANGE_AREAS | CHANGE_SELECTION | CHANGE_AREA_TYPE ),
	};
	EChangeMask eChangeMask;
	//
	SScriptAreaWindowData()
	{
		Clear();
	}
	//
	void Clear()
	{
		eAreaType = NDb::EAT_CIRCLE;
		scriptAreaList.clear();
		selectedScriptAreaIDList.clear();
		eChangeMask = CHANGE_NONE;
	}
};


// The palette's half of the exchange, whichever toolkit draws it.
//
// The first palette to use the *whole* of CPaletteCommands rather than override
// it: this one answers the two dialog-data commands and nothing else, so
// neither HandleCommand nor UpdateCommand needs writing at all. Both palettes
// implement the two halves and inherit the dispatch.
typedef CPaletteCommands<SScriptAreaWindowData> CScriptAreaCommands;


// The script area palette, behind a boundary that names no toolkit.
//
// A dialog-data palette rather than an edit-parameter one, and the difference
// shows in how it talks: instead of a flag saying which field changed, it fills
// in eChangeMask to say what the state should do about it -- rebuild the list,
// re-apply the selection, delete what is selected, or take a new area type.
namespace NScriptAreaView
{
	// Creates the palette inside pTabWindow, registers it in the tab list, and
	// returns it ready to be handed to AddTab with a label. Null if it could not
	// be created.
	CWnd* Create( CDefault3DTabWindow *pTabWindow );

	// Named so the factory can reach them; not for anything else to call.
	CWnd* CreateMfc( CDefault3DTabWindow *pTabWindow );
#ifdef OBK2_WITH_WX
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow );
#endif
}
