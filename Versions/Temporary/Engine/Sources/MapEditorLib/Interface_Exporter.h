#pragma once

#include "Interface_Builder.h"
/**

struct IReportCollector : public CObjectBase
{
	virtual void AddReport( const std::string &szObjectType,
													const std::string &szObjectName, 
		                      const int nObjectID,
													const std::string &szUserReport ) = 0;
};
/**/

#define DEFAULT_EXPORTER_LABEL __DefaultHierarchicalExporter__
#define DEFAULT_EXPORTER_LABEL_TXT "__DefaultHierarchicalExporter__"

// При иерархическом экспорте вызовы StartExport и FinishExpost производятся в произвольном порядке
// Вызов Export для родителя производятся раньше чем для потомков

// в cpp файле написать макрос: REGISTER_EDITOR_IN_...( typeName, className )

// При создании конвертера
// Constructor()
// StartExport 
// Export()
// ...
// Export()
// FinishExport 

// При разрушении конвертера
// Destructor()

// При переключении конвертеров ( не удаляется и не создается )
// StartExport 
// Export()
// ...
// Export()
// FinishExport 

// Первый параметр
#define FORCE_EXPORT true
#define NOT_FORCE_EXPORT false

// Второй параметр
#define START_EXPORT_TOOLS true
#define FINISH_EXPORT_TOOLS true
#define NOT_START_EXPORT_TOOLS false
#define NOT_FINISH_EXPORT_TOOLS false

// Третий параметр
#define EXPORT_REFERENCES true
#define CHECK_REFERENCES true
#define NOT_EXPORT_REFERENCES false
#define NOT_CHECK_REFERENCES false

enum EXPORT_TYPE
{
	ET_BEFORE_REF	= 0,
	ET_AFTER_REF	= 1,
	ET_NO_REF			= 2,
};

enum EXPORT_RESULT
{
	ER_FAIL					= 0,
	ER_SUCCESS			= 1,
	ER_BREAK				= 2,
	ER_NOT_CHANGED	= 3,
	ER_UNKNOWN			= 4,
};

struct IManipulator;
struct IExporter : public CObjectBase
{
	// bForce - безусловный экспорт объектов (различные пункты меню)
	// rszObjectTypeName используется при написании экспортеров для нескольких объектов 
	// (на каждый тип по своему экземпляру экпортера)
	// Вызывается перед конвертацией данных
	// true - все нормально
	// false - для всех объектов этого типа экспорт не вызывается, опрос результата экспорта 
	// возвращает ER_FAIL, FinishExport не вызывается
	virtual bool StartExport( const std::string &rszObjectTypeName, bool bForce ) = 0;
	// Вызывается после конвертации данных
	virtual void FinishExport( const std::string &rszObjectTypeName, bool bForce ) = 0;
	// Вызывается на каждый объект
	// ER_FAIL - не вызывается экспорт ссылок и экспорт после ссылок
	// ER_BREAK - немедленное прекращение текущего экспорта
	// ER_SUCCES - все впорядке, можно продолжать
	virtual EXPORT_RESULT ExportObject( IManipulator* pManipulator,
																			const std::string &rszObjectTypeName,
																			const std::string &rszObjectName,
																			bool bForce,
																			EXPORT_TYPE exportType ) = 0;
	// Вызывается перед проверкой даных
	// bExport - вызывается после экспорта объекта
	// true - все нормально
	// false - для всех объектов этого типа экспорт не вызывается, опрос результата экспорта
	// возвращает ER_FAIL, FinishExport не вызывается
	virtual bool StartCheck( const std::string &rszObjectTypeName, bool bExport ) = 0;
	// Вызывается после конвертации данных
	virtual void FinishCheck( const std::string &rszObjectTypeName, bool bExport ) = 0;
	// Вызывается на каждый объект
	// bExport - вызывается после экспорта объекта
	// ER_FAIL - не вызывается экспорт ссылок и экспорт после ссылок
	// ER_BREAK - немедленное прекращение текущего экспорта
	// ER_SUCCES - все впорядке, можно продолжать
	virtual EXPORT_RESULT CheckObject( IManipulator* pManipulator,
																			const std::string &rszObjectTypeName,
																			const std::string &rszObjectName,
																			bool bExport,
																			EXPORT_TYPE exportType ) = 0;
};


struct IExportTool : public CObjectBase
{
	// Вызывается из StartDefaultExport
	virtual void StartExportTool() = 0;
	// Вызывается из FinishDefaultExport
	virtual void FinishExportTool() = 0;
};


struct IExporterContainer : public CObjectBase
{
	enum { tidTypeID = 0x1408A3C1 };
	//
	// Проверить на возможность конвертирования объекта
	virtual bool CanExportObject( const std::string &rszObjectTypeName ) = 0;
	//Получить конкретный экспортер
	virtual IExporter* GetExporter( const std::string &rszObjectTypeName ) = 0;
	//
	// Методы создания и удаления експортеров
	virtual void Create( const std::string &rszObjectTypeName ) = 0;
	virtual void Destroy( const std::string &rszObjectTypeName ) = 0;
	//
	// Методы регистрирования ExportTool
	virtual void RegisterExportTool( IExportTool *pExportTool ) = 0;
	virtual void UnRegisterExportTool( IExportTool *pExportTool ) = 0;
	//
	// Методы для экпорта по умолчанию, так запускается иерархический экспорт
	virtual bool StartExport( const std::string &rszObjectTypeName, 
														bool bForce, 
														bool bStartTools, 
														bool bExportReferences ) = 0;
	virtual void FinishExport( const std::string &rszObjectTypeName, 
														bool bForce, 
														bool bFinishTools, 
														bool bExportReferences ) = 0;
	virtual EXPORT_RESULT ExportObject( IManipulator* pManipulator,
																			const std::string &rszObjectTypeName,
																			const std::string &rszObjectName,
																			bool bForce,
																			bool bExportReferences ) = 0;
	// Методы для проверки по умолчанию, так запускается иерархическая проверка
	virtual bool StartCheck( const std::string &rszObjectTypeName, 
													bool bStartTools, 
													bool bCheckReferences ) = 0;
	virtual void FinishCheck( const std::string &rszObjectTypeName, 
														bool bFinishTools, 
														bool bCheckReferences ) = 0;
	virtual EXPORT_RESULT CheckObject( IManipulator* pManipulator,
																		 const std::string &rszObjectTypeName,
																		 const std::string &rszObjectName,
																		 bool bCheckReferences ) = 0;
	//
	// Проверить на удачность конвертации данного объекта ( имя объекта в формате: szObjectTypeName:szObjectName )
	virtual EXPORT_RESULT GetExportResult( const std::string &rszObjectRefName ) = 0;
	// Проверить на удачность конвертации данного объекта
	virtual EXPORT_RESULT GetExportResult( const std::string &rszObjectTypeName, const std::string &rszObjectName ) = 0;
};



