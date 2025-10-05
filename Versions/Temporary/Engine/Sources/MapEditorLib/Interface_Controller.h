#pragma once


typedef std::unordered_map<CDBID, int> CObjectNameSet;	// важно наличие
typedef std::list<CDBID> CObjectNameList;					// важна последовательность

struct SObjectSet
{
	std::string szObjectTypeName;
	CObjectNameSet objectNameSet;

	inline void Clear()
	{
		szObjectTypeName.clear();
		objectNameSet.clear();
	}
};


// 
struct SSelectionSet
{
	std::string szObjectTypeName;
	CObjectNameList objectNameList;

	inline void Clear()
	{
		szObjectTypeName.clear();
		objectNameList.clear();
	}
};

// Операция Undo сама знает что надо отменить
// Должна уметь выполнять обратную операцию
struct IView;
struct IController : public CObjectBase
{
	// Выполнить Undo ( кроме указанного View )
	virtual bool Undo( bool bUpdateManipulator, bool bUpdateViews, IView *pViewToExlude ) = 0;
	// Выполнить Redo ( кроме указанного View )
	virtual bool Redo( bool bUpdateManipulator, bool bUpdateViews, IView *pViewToExlude ) = 0;
	// Пустой или не пустой контроллер (если он пустой он не запоминается в Container)
	virtual bool IsEmpty() const = 0;
	// Применение этогй операции влечет за собой очистку буфера UNDO в Containter
	virtual bool IsAbsolute() const = 0;
	// Получить краткое описание
	virtual void GetDescription( CString *pstrDescription ) const = 0;
	// Если Undo Operation временная, то ее можно удалить по этому ID
	virtual void GetTemporaryLabel( std::string *pszTemporaryLabel ) const = 0;
};


// Управляющий операциями Undo
// Складывает из в буфера и умеет из перекладывать
typedef std::list<CString> CDescriptionList;
struct IControllerContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A3C2 };
	//
	// Добавить операцию
	virtual void Add( IController *pOperation ) = 0;
	// Очистить буфер
	virtual void Clear() = 0;
	// Есть ли для операции Undo данные
	virtual bool CanUndo() const = 0;
	// Есть ли для операции Redo данные
	virtual bool CanRedo() const = 0;
	// Undo na nCount операций	
	virtual bool Undo( int nCount ) = 0;
	// Redo na nCount операций
	virtual bool Redo( int nCount ) = 0;
	//Получить список описаний
	virtual int GetDescriptionList( CDescriptionList *pDescriptionList, bool bUndoList ) const = 0;
	// Удалить временные Undo Operations
	virtual int RemoveTemporaryControllers( const std::string &rszTemporaryLabel ) = 0;
};



