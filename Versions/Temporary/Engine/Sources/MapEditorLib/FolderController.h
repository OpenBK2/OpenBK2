#pragma once

#include "DefaultController.h"


// TYPE_INSERT:		( используется только для Update )
// szDestination	- имя вставляемого объекта
//
// TYPE_REMOVE:		( используется только для Update )
// szDestination	- имя удаляемого объекта
//
// TYPE_COPY:			( используется только для Update )
// szDestination	- куда копировать
// szSource				- откуда копировать
//
// TYPE_RENAME:		( используется только для Update )
// szDestination	- куда перемещать
// szSource				- откуда перемещать
// newValue				- true / false, true - смена HTREEITEM, false - только переименование текущего объекта в базе и в дереве
//
// TYPE_COLOR		( используется только для Update )
// szDestination	- имя объекта
// newValue				- новое (установленное) значение
//
// TYPE_EXPAND		( используется только для Update )
// szName					- имя объекта
// newValue				- 1 Expand, 0 Collapse
//
// TYPE_CHECK		( используется только для Update )
// szName					- имя объекта
//
// TYPE_EXPORT		( используется только для Update )
// szName					- имя объекта
// newValue				- 1 Force Export, 0 Export
class CFolderController : public CDefaultController
{
	OBJECT_NOCOPY_METHODS( CFolderController );

public:	
	struct SUndoData
	{
		enum EType
		{
			TYPE_INSERT				= 0,
			TYPE_REMOVE				= 1,
			TYPE_COPY					= 2,
			TYPE_RENAME				= 3,
			TYPE_COLOR				= 4,
			TYPE_EXPAND				= 6,
		};
		//
		std::string szDestination;
		std::string szSource;
		CVariant newValue;
		//
		EType eType;
	};
	//
	typedef std::list<SUndoData> CUndoDataList;
	
	// список данных подвергнутых изменениям
	CUndoDataList undoDataList;

protected:
	// CUndoObject
	virtual bool UndoWithoutUpdateViews();
	virtual bool RedoWithoutUpdateViews();

public:
	// IUndoObject
	virtual bool IsEmpty() const { return undoDataList.empty(); }
	virtual bool IsAbsolute() const { return true; }
	// Helpers
	bool AddInsertOperation( const std::string &rszObjectName );
	bool AddRemoveOperation( const std::string &rszObjectName );
	bool AddCopyOperation( const std::string &rszDestination, const std::string &rszSource );
	bool AddRenameOperation( const std::string &rszDestination, const std::string &rszSource, bool bNewHTREEITEM );
	bool AddColorOperation( const std::string &rszObjectName, int nNewColor );
	bool AddExpandOperation( const std::string &rszObjectName, bool bExpand );
};




