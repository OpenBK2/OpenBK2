#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_ITEM_EDITOR__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_ITEM_EDITOR__
#pragma once

#include "../MapEditorLib/Interface_PCItemEditor.h"


class CPCItemEditor : public IPCItemEditor
{
private:
	string szName;
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
	const string& GetName() const { return szName; }
	EPCIEType GetItemEditorType() const { return nEditorType; }
	const	SPropertyDesc* GetPropertyDesc() const { return pPropertyDesc; }
	int GetControlID() const { return nControlID; }
	CWnd* GetTargetWindow() { return pwndTargetWindow; }
	//	
	virtual bool CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow );
	virtual void SetDefaultValue() { bDefaultValue = true; }
	virtual bool IsDefaultValue() { return bDefaultValue; }
	virtual void EnableEdit( bool bEnable ) { bEnableEdit = bEnable; }
	virtual bool IsEditEnabled() { return bEnableEdit; }
	//
	void SetValueChanged() { bDefaultValue = false; }
};

bool GetPCItemStringValue( string *pszValue,
													 const CVariant &rValue,
													 const string &rszDefaultValue,
													 EPCIEType nType,
													 const SPropertyDesc *pDesc,
													 bool bMultiline );

bool GetPCItemValue( CVariant *pValue,
										 const string &rszValue,
										 const CVariant &rDefaultValue,
										 EPCIEType nType,
										 const SPropertyDesc *pDesc );

//bool CheckPCValue( IManipulator *pManipulator, const string &rszName, const CVariant &rValue );

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_ITEM_EDITOR__)

