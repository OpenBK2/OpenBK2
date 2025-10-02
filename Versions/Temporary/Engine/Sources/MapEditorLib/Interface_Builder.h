#if !defined(__INTERFACE__BUILDER__)
#define __INTERFACE__BUILDER__
#pragma once

#include "Interface_FolderCallback.h"
#include "StringManager.h"
//
interface IManipulator;
interface IView;

#define DEFAULT_BUILDER_LABEL __DEFAULT__BUILDER__
#define DEFAULT_BUILDER_LABEL_TXT "__DEFAULT__BUILDER__"


// BDF = build data flag
#define BDF_EXPORT_CHECK_BOX	0x00000001
#define BDF_EDIT_CHECK_BOX		0x00000002
//
#define BDF_CHECK_FILE_NAME		0x00000010
#define BDF_CHECK_PROPERTIES	0x00000020
#define BDF_ALL								0xFFFFFFFF

struct SBuildDataParams
{
	UINT nFlags;										// Параметры редактрования
	string szObjectTypeName;				// Тип объекта
	// имя формируется следующим образом:
	// string szFileName = szObjectNamePrefix + szObjectName + szObjectNamePostfix;
	// CStringManager::ExtendFileExtention( &szFileName, szObjectNameExtention );
	// Если установлено, что редактируем каталог - не забудте в szObjectNamePostfix в начале поставить слеш
	string szObjectNamePrefix;			// Нередактируемая часть
	string szObjectName;						// Объект редактирования
	string szObjectNamePostfix;			// Нерадактируемая часть
	string szObjectNameExtention;		// Расширение (необходимо разделять)
	//
	bool bNeedExport;								// Необходимо ли экспортировать объект после создания
	bool bNeedEdit;									// Необходимо ли загружать одъект после экспорта или создания
	SBuildDataParams() : nFlags( BDF_ALL ), bNeedExport( false ), bNeedEdit( false ) {}
	void GetObjectName( string *pszObjectName )
	{
		if ( pszObjectName )
		{
			( *pszObjectName ) = szObjectNamePrefix + szObjectName + szObjectNamePostfix;
			CStringManager::ExtendFileExtention( pszObjectName, szObjectNameExtention );
			CStringManager::RemoveDoubleSlashes( pszObjectName );
		}
	}
	inline bool DoesNameEmpty() { return szObjectName.empty(); }
};


// CRAP{ HASH_SET
typedef hash_map<string, DWORD> CTableSet;
typedef hash_map<int, CTableSet> CTableSetMap;
// CRAP} HASH_SET


interface IBuildDataCallback
{
	// проверить данные на правильность заполнения, если данные не верны вернуть краткое описание ошибки
	// pszDescription - может быть нулевой
	virtual bool IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView ) = 0;
	// проверить имя объекта на уникальность
	virtual bool IsUniqueObjectName( const string &szObjectType, const string &szObjectName ) = 0;
};


// в cpp файле написать макрос: REGISTER_EDITOR_IN_...( typeName, className )

// При создании компоновщика
// Constructor()
// Build();

// При разрушении компоновщика
// Destructor()

// При переключении компоновщиков ( не удаляется и не создается )
// Build();
interface IBuilder : public CObjectBase
{
	// Создать и скомпоновать объекты ( может быть добавлено сразу много объектов )
	// true - создали какие либо объекты ( необходимо обработать ситуацию дальше )
	// false - нажали Cancel ( ситуацию обрабатывать нет необходимости )
	virtual bool InsertObject( string *pszObjectTypeName,											// Необходимо для создания Builder'ов для несколько типов объектов
														 string *pszUniqueObjectName,										// Имя выбранное пользователем ( полное )
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,										// Можно ли после создания объекта менять его имя
														 bool *pbNeedExport,														// Нужно ли экспортить сразу после создания
														 bool *pbNeedEdit ) = 0;
	// Скопировать объект
	virtual bool CopyObject( const string &rszObjectTypeName,									// Необходимо для создания Builder'ов для несколько типов объектов
													 const string &rszDestination,										// Имя куда копировать
													 const string &rszSource ) = 0;										// Имя откуда копировать
	// Переименовать объект
	virtual bool RenameObject( const string &rszObjectTypeName,								// Необходимо для создания Builder'ов для несколько типов объектов
														 const string &rszDestination,									// Имя куда копировать
														 const string &rszSource ) = 0;									// Имя откуда копировать
	// Удалить объект
	virtual bool RemoveObject( const string &rszObjectTypeName,								// Необходимо для создания Builder'ов для несколько типов объектов
														 const string &rszObjectName ) = 0;							// Имя выбранное пользователем ( полное )
	//
	virtual void GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder ) = 0;
};

interface IBuilderContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A3C3 };
	//
	// Проверить на существование компоновщика
	virtual bool CanBuildObject( const string &rszObjectTypeName ) = 0;
	virtual bool CanDefaultBuildObject( const string &rszObjectTypeName ) = 0;
	// Методы создания и удаления компоновщиков
	virtual void Create( const string &rszObjectTypeName ) = 0;
	virtual void Destroy( const string &rszObjectTypeName ) = 0;
	//
	virtual bool InsertObject( string *pszObjectTypeName,											// Необходимо для создания Builder'ов для несколько типов объектов
														 string *pszUniqueObjectName,										// Имя выбранное пользователем ( полное )
														 bool bFromMainMenu,
														 bool *pbCanChangeObjectName,										// Можно ли после создания объекта менять его имя
														 bool *pbNeedExport,														// Нужно ли экспортить сразу после создания
														 bool *pbNeedEdit ) = 0;												// Нужно ли загружать редактор после экспорта или после создания объекта
	virtual bool CopyObject( const string &rszObjectTypeName,									// Необходимо для создания Builder'ов для несколько типов объектов
													 const string &rszDestination,										// Имя куда копировать
													 const string &rszSource ) = 0;										// Имя откуда копировать
	virtual bool RenameObject( const string &rszObjectTypeName,								// Необходимо для создания Builder'ов для несколько типов объектов
														 const string &rszDestination,									// Имя куда копировать
														 const string &rszSource ) = 0;									// Имя откуда копировать
	virtual bool RemoveObject( const string &rszObjectTypeName,								// Необходимо для создания Builder'ов для несколько типов объектов
														 const string &rszObjectName ) = 0;							// Имя выбранное пользователем ( полное )
	virtual void GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder ) = 0;
	// заполнить BuildData
	virtual bool FillBuildData(	string *pszBuildDataTypeName,
															string *pszBuildDataName,
															SBuildDataParams *pBuildDataParams,					
															IBuildDataCallback *pBuildDataCallback ) = 0;
	virtual bool FillNewObjectName( SBuildDataParams *pBuildDataParams ) = 0;
};

#endif // !defined(__INTERFACE__BUILDER__)


