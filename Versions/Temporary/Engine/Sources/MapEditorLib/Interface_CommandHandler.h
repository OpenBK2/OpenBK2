#pragma once

#include <cstdint>

#define INVALID_COMMAND_ID (0xFFffFFff)
#define INVALID_COMMAND_HANDLER_ID (0xFFffFFff)

// Наследуется от CWnd служит только для получения доступа, удалять не требуется
struct ICommandHandler
{
	virtual ~ICommandHandler() {}
	//
	// Обработать команду от User Interface, если вернули false, то команда не обработана
	virtual bool HandleCommand( unsigned nCommandID, uint32_t dwData ) = 0;
	// Можно ли сейчас обрабатывать команду? Если вернули false, то команда не обработана
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) = 0;
};


// Служит для регистрации активного обработцика команд
// Обычно это некоторое активное окно, если их много то сообщения получает только одно
// выделенное по некоторому признаку (активизация)
struct ICommandHandlerContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A381 };
	// Зарегистрироать обработчик в MainFrame ( команды от пользовательского интерфейса )
	virtual void Register( unsigned nType, unsigned nFirstCommandID, unsigned nLastCommandID ) = 0;
	// Удалить регистрацию обработчика из Mainframe
	virtual void UnRegister( unsigned nType ) = 0;
	// Установить обработчик команды от User Interface
	virtual void Set( unsigned nType, ICommandHandler *pCommandHandler ) = 0;
	// Удалить обработчик команды, при условии что указанный pCommandHandler является owner
	virtual void Remove( unsigned nType, ICommandHandler *pCommandHandler ) = 0;
	// Удалить обработчик команды
	virtual void Remove( unsigned nType ) = 0;
	// Получить обработчик команды 
	virtual ICommandHandler* Get( unsigned nType ) = 0;
	// Передать команду на обработку, последнему зарегистрированному обработчику
	virtual bool HandleCommand( unsigned nType, unsigned nCommandID, uint32_t dwData ) = 0;
	// Проверить возможность обработки команды у последнего зарегистрированного обработчика
	virtual bool UpdateCommand( unsigned nType, unsigned nCommandID, bool *pbEnable, bool *pbCheck ) = 0;
	// Передать команду на обработку, обработчик получить из ранее зарегистрированных
	virtual bool HandleCommand( unsigned nCommandID, uint32_t dwData ) = 0;
	// Проверить возможность обработки команды, обработчик получить из ранее зарегистрированных
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) = 0;
};



