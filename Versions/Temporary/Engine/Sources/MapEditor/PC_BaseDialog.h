#pragma once

struct IView;
#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

class CPCBaseDialog : public ICommandHandler
{
	// ICommandHandler
	virtual bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
	
	// Получить указатель на дерево
	virtual IView* GetView() = 0;
	// Получить указатель на обработчик команд
	virtual ICommandHandler* GetCommandHandler() = 0;
	// Построить дерево
	virtual void CreateTree() = 0;
	// Обновить все значения в дереве без его построения
	// не использовать без надобности, использовать только с MaskManipulator
	virtual void UpdateValues() = 0;
};


