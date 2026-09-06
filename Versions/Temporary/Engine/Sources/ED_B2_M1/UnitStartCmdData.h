#pragma once

#include "PaletteCommands.h"
#include "StringResources.h"

#include <string>
#include <vector>

// What the unit start commands palette holds, and how the editor asks for it.
// Both used to live in UnitStartCmdWindow.h beside the MFC window; they are
// here so the wx palette can share them rather than restate them.

struct SUnitStartCmdWindowData
{
	//
	struct SCmd // данные для создания row в списке команд
	{
		int nIndex;					// индекс команды в MapInfo.startCommandList[]
		std::string szType;			// тип команды
		std::string szTarget;	// местоназначение команды - имя юнита или координаты
		//
		SCmd()
			:	nIndex(-1),
			szType( RCSTR("<UNKNOWN>") )
		{
		}
		bool operator== ( const SCmd &cmd ) const { return ( this->nIndex == cmd.nIndex ); }
	};
	std::vector<SCmd> commands;
	//
	enum EAction
	{
		NO_CMD,
		ADD_CMD,
		DEL_CMD,
		EDIT_CMD,
		ORDER_DOWN_CMD,
		ORDER_UP_CMD,
		SEL_CHANGE
	};
	EAction eLastAction;							// последнее событие интерфейса пользователя
	std::vector<int> selectedCommands;		// SCmd::nIndex выбранных в списке команд
	//
	SUnitStartCmdWindowData() 
	{
		Clear();
	}
	//
	void Clear()
	{
		eLastAction = NO_CMD; 
		selectedCommands.clear();
		commands.clear();
	}
	//
	void SetLastAction( EAction eAction )
	{
		eLastAction = eAction;
	}
};


// CMapInfoState drives this palette with the two window commands every palette
// takes, so the dispatch is CPaletteCommands' and only the reading and writing
// of controls is written per implementation.
typedef CPaletteCommands<SUnitStartCmdWindowData> CUnitStartCmdCommands;
