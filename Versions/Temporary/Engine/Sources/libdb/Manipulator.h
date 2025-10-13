#pragma once

#include "Variant.h"

#include <cstdint>

namespace NDb
{
	struct IObjMan;
}

#define REF_TYPE_UNKNOWN (-1)                   //subject to remove
#define NODE_ADD_INDEX (-1)                     //Manipulator - add to the end of list 
#define NODE_REMOVEALL_INDEX (-1)               //Manipulator - remove all
#define INVALID_NODE_ID (0xffFFffFF)            //error when asking for node id
//
#define ARRAY_NODE_START_CHAR   '['             // Стартовый символ номера элемента массива
#define ARRAY_NODE_END_CHAR     ']'             // Комечный символ элемента массива
//
#define LEVEL_SEPARATOR_CHAR    '.'             //separator for fields in structs
#define TYPE_SEPARATOR_CHAR     ':'             //Разделитель между типом и именем объекта в мульти ссылках
#define PATH_SEPARATOR_CHAR     '\\'            //разделитель между папками в FolderManipulator
//#define PATH_SEPARATOR_CHAR_STR "\\"            //он же, в виде с-строки
//
#define ID_PREFIX_CHAR          '#'             //Если перед индексом масмва установлен этот символ то это ID а не индекс массива
//
#define FS_EXLUDE_SYMBOLS       "*?""<>|"       //subject to remove
//


// ************************************************************************************************************************ //
// **
// ** manipulator basic interface
// **
// **
// **
// ************************************************************************************************************************ //

struct SIteratorDesc
{
	virtual ~SIteratorDesc() {}
};

//structure with property description
struct SPropertyDesc : public SIteratorDesc
{
	typedef std::list<std::string> CValuesList;
	// CRAP{ HASH_SET
	typedef std::unordered_map<std::string, uint32_t> CTypesMap;
	// CRAP} HASH_SET
	//value checks flags
	enum EPropertyCheckType
	{
		PCT_NONE	= 0x0000,
		PCT_MIN		= 0x0001,
		PCT_MAX		= 0x0002,
	};
	
	std::string szDesc;                                  //property description MBCS
	bool bDescInitialized;

	float fLeftBorder;                              //left border value
	float fRightBorder;                             //right border value
	int nMinOccurs;
	int nMaxOccurs;
	uint32_t dwChecks;                                 //check flags contained here
	std::string szDefault;                               // default field value in string representation
	
	std::string szTypeName;                              //type name
	CTypesMap refTypes;                             //possible types of objects, to which references can be set  
	CValuesList values;                             //possible values (i.e. for combo boxes), MBCS
	std::string szEnumName;
	
	bool bArray;                                    //Массив
	bool bStruct;                                   //Структура
	bool bMultiArray;                               //Является массивом в массиве (always false)
	bool bHidden;                                   //ограничение доступа к объекту через конструктор IManipulator
	bool bReadOnly;                                 //флаг считываемый из GDB, используется в Property Control
	bool bNoCode;
	bool bNoHeader;
	bool bUseUpperType;
	bool bNoBase;
	bool bUnsafe;
	std::string szTypeRename;
	std::string szTypePrefix;
	std::string szTypePath;
	
	std::string szLocalDef;                //if related type should be defined in local namespace (for code generator)
	bool bCheckSum;                   //if the field should be counted when calculationg checksum (for code generator)
	int nTypeID;                      //struct typeID (for code generator)
	std::string szHSection;
	std::string szInclude;
	int nSize;
	bool bPrivate;										// is this var private
	std::string szRndType;

	std::string szPropControlType;                       //string whith editor control type
	std::string szStringParam;                           // additional parameter
	int nIntParam;                                  // additional parameter

	SPropertyDesc()
		: fLeftBorder( 0.0f ),
			fRightBorder( 0.0f ),
			nMinOccurs( 0 ),
			nMaxOccurs( -1 ),
			dwChecks( PCT_NONE ),
			bArray( false ), 
			bStruct( false ),
			bMultiArray( false ),
			bHidden( false ),
			bReadOnly( false ),
			szLocalDef( "" ),
			bCheckSum( false ),
			nTypeID( 0 ),
			bNoCode( false ),
			bNoHeader( false ),
			bNoBase( false ),
			nIntParam( 0 ),
			szHSection( "" ),
			szStringParam( "" ),
			szTypeRename( "" ),
			szTypePath( "" ),
			bDescInitialized( false ),
			bUseUpperType( false ),
			bUnsafe( false ),
			bPrivate( false ),
			nSize( 0 ),
			szRndType( "" ),
			szEnumName("Unknown")
			{ }
	virtual ~SPropertyDesc() {}
	//
};


// manipulator property iterator
struct IManipulatorIterator : public CObjectBase
{
	// go to the next property/item
	virtual bool Next() = 0;
	// is all properties/items already iterated?
	virtual bool IsEnd() const = 0;
	// get current property descriptor (if availible)
	virtual const SIteratorDesc* GetDesc() const = 0;
	// get current item name
	virtual bool GetName( std::string *pszName ) const = 0;
	// get current item type
	virtual bool GetType( std::string *pszType ) const = 0;
	// get current item id
	virtual UINT GetID() const = 0;
	// is it folder
	virtual bool IsFolder() const = 0;
};


// manipulator
enum ECacheType
{
	ECT_NO_CACHE,
	ECT_CACHE_LOCAL,
	ECT_CACHE_GLOBAL,
};

struct IManipulator : public CObjectBase
{
	typedef std::unordered_map<std::string, UINT> CNameMap;
	//
	// begin to iterate through all properties
	virtual IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache ) = 0;
	// get property descriptor by name
	virtual const SIteratorDesc* GetDesc( const std::string &rszName ) const = 0;
	// get item type by name
	virtual bool GetType( const std::string &rszName, std::string *pszType ) const = 0;
	// get item id by name
	virtual UINT GetID( const std::string &rszName ) const = 0;
	// get object DBID
	virtual CDBID GetDBID() const { return CDBID(); }
	// get item id by name
	virtual bool GetName( UINT nID, std::string *pszName ) const = 0;
	// add one more element to list
	virtual bool InsertNode( const std::string &rszName, int nNodeIndex = NODE_ADD_INDEX ) = 0;
	// remove element from list (or all elements)
	virtual bool RemoveNode( const std::string &rszName, int nNodeIndex = NODE_REMOVEALL_INDEX ) = 0;
	virtual bool RemoveNodeByID( const std::string &szName, int nNodeID ) = 0;
	// rename element ( if possible )
	virtual bool RenameNode( const std::string &rszName, const std::string &rszNewName ) = 0;
	// retrieve value
	virtual bool GetValue( const std::string &rszName, CVariant *pValue ) const = 0;
	// set value
	virtual bool SetValue( const std::string &rszName, const CVariant &rValue ) = 0;
	// is name exists and have a value
	virtual bool IsNameExists( const std::string &rszName ) const = 0;
	//
	virtual void GetNameList( CNameMap *pNameMap ) const = 0;
	//
	virtual void ClearCache() {};
	// check value (still out of order)
	virtual bool CheckValue( const std::string &rszName, const CVariant &rValue, bool *pResult ) const = 0;
	//
	virtual NDb::IObjMan *GetObjMan() = 0;
};


// multimanipulator
struct IMultiManipulator : public IManipulator
{
	// remove all manipulators
	virtual void Clear() = 0;
	// add new manipulator
	virtual void AddManipulator( const IManipulator *pManipulator ) = 0;
};

inline bool IsDBIDEmpty( const CVariant &var )
{
	return var.GetType() == CVariant::VT_NULL || ( var.GetType() == CVariant::VT_DBID && var.GetDBID().IsEmpty() );
}


