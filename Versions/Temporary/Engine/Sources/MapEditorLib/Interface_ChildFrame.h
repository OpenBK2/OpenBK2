#pragma once


// в cpp файле написать макрос: REGISTER_CHILD_FRAME_IN_...( typeName, className )
interface IChildFrame : public CObjectBase
{
	//
	virtual bool Create() = 0;
	//
	virtual void Destroy() = 0;
	//
	virtual void Enter() = 0;
	//
	virtual void Leave() = 0;
};


interface IChildFrameContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408B400 };
	//
	// Проверить на возможность создания
	virtual bool CanCreate( const string &rszChildFrameTypeName ) = 0;
	// Проверить что именно этот редактор сейчас активен
	virtual bool IsActive( const string &rszChildFrameTypeName ) = 0;
	// Создать Сhild frame
	virtual bool Create( const string &rszChildFrameTypeName ) = 0;
	// Удалить Child Frame
	virtual void Destroy() = 0;
	//
	virtual void Enter() = 0;
	//
	virtual void Leave() = 0;
};



