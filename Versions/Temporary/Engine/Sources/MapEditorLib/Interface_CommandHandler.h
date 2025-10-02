#pragma once

#define INVALID_COMMAND_ID (0xFFffFFff)
#define INVALID_COMMAND_HANDLER_ID (0xFFffFFff)

// Наследуется от CWnd служит только для получения доступа, удалять не требуется
interface ICommandHandler
{
	virtual ~ICommandHandler() {}
	//
	// Обработать команду от User Interface, если вернули false, то команда не обработана
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData ) = 0;
	// Можно ли сейчас обрабатывать команду? Если вернули false, то команда не обработана
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck ) = 0;
};


// Служит для регистрации активного обработцика команд
// Обычно это некоторое активное окно, если их много то сообщения получает только одно
// выделенное по некоторому признаку (активизация)
interface ICommandHandlerContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A381 };
	// Зарегистрироать обработчик в MainFrame ( команды от пользовательского интерфейса )
	virtual void Register( UINT nType, UINT nFirstCommandID, UINT nLastCommandID ) = 0;
	// Удалить регистрацию обработчика из Mainframe
	virtual void UnRegister( UINT nType ) = 0;
	// Установить обработчик команды от User Interface
	virtual void Set( UINT nType, ICommandHandler *pCommandHandler ) = 0;
	// Удалить обработчик команды, при условии что указанный pCommandHandler является owner
	virtual void Remove( UINT nType, ICommandHandler *pCommandHandler ) = 0;
	// Удалить обработчик команды
	virtual void Remove( UINT nType ) = 0;
	// Получить обработчик команды 
	virtual ICommandHandler* Get( UINT nType ) = 0;
	// Передать команду на обработку, последнему зарегистрированному обработчику
	virtual bool HandleCommand( UINT nType, UINT nCommandID, DWORD dwData ) = 0;
	// Проверить возможность обработки команды у последнего зарегистрированного обработчика
	virtual bool UpdateCommand( UINT nType, UINT nCommandID, bool *pbEnable, bool *pbCheck ) = 0;
	// Передать команду на обработку, обработчик получить из ранее зарегистрированных
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData ) = 0;
	// Проверить возможность обработки команды, обработчик получить из ранее зарегистрированных
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck ) = 0;
};



