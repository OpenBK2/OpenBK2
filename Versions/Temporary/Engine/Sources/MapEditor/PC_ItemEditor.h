#pragma once

#include "MapEditorLib/Interface_PCItemEditor.h"


struct IPCItemEditor : public CObjectBase
{
	// Получить значение имени элемента дерева ( для совместимости )
	virtual const std::string& GetName() const = 0;
	// Получить значение типа элемента дерева ( для совместимости )
	virtual EPCIEType GetItemEditorType() const = 0;
	// Получить описатель элемента дерева ( для совместимости )
	virtual const	SPropertyDesc* GetPropertyDesc() const = 0;
	// Получить идентификатор окна связанного с редактором 
	virtual int GetControlID() const = 0;
	// Получить окно куда необходимо посылать все сообщения ( IC_... )
	virtual class CWnd* GetTargetWindow() = 0;
	//
	// Создать окно редактора ( CDialog - для того чтобы можно было перескакивать по контролам )
	virtual bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, class CWnd *_pwndTargetWindow ) = 0;
	// Разместить окно редактора в произвольной области
	virtual bool PlaceEditor( const CTRect<int> &rPlaceRect ) = 0;
	// Перевести фокус на окно редактора
	virtual bool ActivateEditor( class CDialog *pwndActiveDialog ) = 0;
	//
	// Установить значение
	virtual void SetValue( const CVariant &rValue ) = 0;
	// Получить значение
	virtual void GetValue( CVariant *pValue ) = 0;
	// Установить значение по умолчанию
	virtual void SetDefaultValue() = 0;
	// Определить что значение не менялось
	virtual bool IsDefaultValue() = 0;
	// установить режим работы редактора
	virtual void EnableEdit( bool bEnable ) = 0;
	//
	virtual bool IsEditEnabled() = 0;
	// Различного рода сообщения приходящие от контролов, которые не связяны c основным редактором
	// Используется для получения сообщений от Slider
	virtual void ProcessMessage( unsigned nMessage, WPARAM wParam, LPARAM lParam ) = 0;
};


class CPCItemEditor : public IPCItemEditor
{
private:
	std::string szName;
	EPCIEType nEditorType;
	const SPropertyDesc* pPropertyDesc;
	int nControlID;
	SObjectSet objectSet;
	CWnd *pwndTargetWindow;
	bool bDefaultValue;
	bool bEnableEdit;

protected:
	const SObjectSet& GetObjectSet() const { return objectSet; }
public:
	CPCItemEditor() : nEditorType( PCIE_UNKNOWN ), pPropertyDesc( 0 ), nControlID( -1 ), pwndTargetWindow( 0 ), bDefaultValue( true ), bEnableEdit( true ) {}

	// IPCItemEditor
	const std::string& GetName() const { return szName; }
	EPCIEType GetItemEditorType() const { return nEditorType; }
	const	SPropertyDesc* GetPropertyDesc() const { return pPropertyDesc; }
	int GetControlID() const { return nControlID; }
	CWnd* GetTargetWindow() { return pwndTargetWindow; }
	CFont* GetEditorFont();
	//	
	virtual bool CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	virtual void SetDefaultValue() { bDefaultValue = true; }
	virtual bool IsDefaultValue() { return bDefaultValue; }
	virtual void EnableEdit( bool bEnable ) { bEnableEdit = bEnable; }
	virtual bool IsEditEnabled() { return bEnableEdit; }
	//
	void SetValueChanged() { bDefaultValue = false; }
};

bool GetPCItemStringValue( std::string *pszValue,
													 const CVariant &rValue,
													 const std::string &rszDefaultValue,
													 EPCIEType nType,
													 const SPropertyDesc *pDesc,
													 bool bMultiline );

bool GetPCItemValue( CVariant *pValue,
										 const std::string &rszValue,
										 const CVariant &rDefaultValue,
										 EPCIEType nType,
										 const SPropertyDesc *pDesc );

//bool CheckPCValue( IManipulator *pManipulator, const std::string &rszName, const CVariant &rValue );


