#include "stdafx.h"

#include "RefListDialog.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_UserData.h"

BEGIN_MESSAGE_MAP(CRefListDialog, CResizeDialog)
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_REF_LIST_OBJECTS, OnItemChanged )
	ON_BN_CLICKED( IDC_REF_LIST_EMPTY_CURRENT, OnSetEmptyCurrent )
	ON_BN_CLICKED( IDC_REF_LIST_EMPTY_ALL, OnClearAll )
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CRefListDialog::CRefListDialog( CWnd* pParent )
	: CResizeDialog( CRefListDialog::IDD, pParent )
{
	SetControlStyle( IDC_REF_LIST_OBJECTS, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER, 0.5f, 0.5f, 1.0f, 0.5f );
	SetControlStyle( IDC_REF_LIST_FIELDS, ANCHORE_LEFT_BOTTOM | RESIZE_HOR | RESIZE_VER, 0.5f, 0.5f, 1.0f, 0.5f );
	SetControlStyle( IDC_REF_LIST_EMPTY_CURRENT, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER, 1.0f / 3.0f, 0.5f, 1.0f, 1.0f );
	SetControlStyle( IDC_REF_LIST_EMPTY_ALL, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER, 0.5f, 0.5f, 1.0f, 1.0f );
	SetControlStyle( IDOK, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER, 2.0f / 3.0f, 0.5f, 1.0f, 1.0f );
}


void CRefListDialog::SetData( const std::string &szObjectTypeName, const std::string &szObjectName, std::list<std::string> *_pReferenceObjectsList )
{
	szTargetTypeName = szObjectTypeName;
	szTargetName = szObjectName;
	pReferenceObjectsList = _pReferenceObjectsList;
	CStringManager::GetRefValueFromTypeAndName( &szTargetFullName, szTargetTypeName, szTargetName, TYPE_SEPARATOR_CHAR );
}

void CRefListDialog::BuildReferenceObjectsList()
{
	ASSERT( szTargetTypeName.empty() == false );

	// Parsed and sorted by the boundary, so that the wx dialog lists the same
	// objects in the same order; see RefListView.h.
	NRefList::BuildObjects( &referenceObjects, *pReferenceObjectsList );
	for ( int i = 0; i < referenceObjects.size(); ++i )
	{
		objectsCtrl.InsertItem( i, referenceObjects[i].szDisplayName.c_str() );
	}
}

void CRefListDialog::BuildFieldsListForObject( const NRefList::SReferenceObject &object )
{
	std::string szText;
	// Shared with the wx dialog; see RefListView.h.
	pCurrentManipulator = NRefList::FindFields( &currentFields, &szText, object,
																							szTargetTypeName, szTargetName );
	fieldsCtrl.SetWindowText( szText.c_str() );
}

BOOL CRefListDialog::OnInitDialog()
{
	CWaitCursor wait;

	CResizeDialog::OnInitDialog();

	CString szDialogTitle;
	GetWindowText( szDialogTitle );
	szDialogTitle += " ";
	szDialogTitle += szTargetFullName.c_str();
	SetWindowText( szDialogTitle );

	fieldsCtrl.SubclassDlgItem( IDC_REF_LIST_FIELDS, this );
	objectsCtrl.SubclassDlgItem( IDC_REF_LIST_OBJECTS, this );

	objectsCtrl.InsertColumn( 0, "Objects", LVCFMT_LEFT, 10000 );
	std::string szText = "No object selected.";
	fieldsCtrl.SetWindowText( szText.c_str() );

	BuildReferenceObjectsList();

	// Move focus from "Set Empty" button to the Cancel button to avoid
	// accidental nullification.
	//
	CWnd *pwndControl = GetDlgItem( IDOK );
	ASSERT( pwndControl );
	GotoDlgCtrl( pwndControl );

	pwndControl = GetDlgItem( IDC_REF_LIST_EMPTY_CURRENT );
	pwndControl->EnableWindow( false );

	nSelectedItem = INVALID_NODE_ID;

	return true;
}


void CRefListDialog::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CRefListDialog::OnOK()
{
	CResizeDialog::OnOK();
}


void CRefListDialog::OnItemChanged( NMHDR* pNMHDR, LRESULT* pResult )
{
	CWaitCursor wait;

	if ( objectsCtrl.GetSelectedCount() > 0 )
	{
		int nNewSelectedItem = objectsCtrl.GetNextItem( -1, LVNI_FOCUSED );
		if ( nNewSelectedItem != INVALID_NODE_ID )
		{
			CWnd *pwndControl = GetDlgItem( IDC_REF_LIST_EMPTY_CURRENT );
			pwndControl->EnableWindow( true );
			if ( nNewSelectedItem != nSelectedItem )
			{
				nSelectedItem = nNewSelectedItem;
				BuildFieldsListForObject( referenceObjects[nSelectedItem] );
			}
		}
		else
			nSelectedItem = INVALID_NODE_ID;
	}
}

void CRefListDialog::OnSetEmptyCurrent()
{
	if ( nSelectedItem == INVALID_NODE_ID )
		return;
	CWaitCursor wait;

	// Shared with the wx dialog; see RefListView.h. The running commentary is
	// shown once at the end here rather than after every field: the MFC dialog
	// rewrote the box inside the loop, which nothing could see -- it never
	// yields to a paint between fields.
	std::string szText = "Clearing..\r\n";
	const bool bCleared = NRefList::ClearFields( &currentFields, &szText, pCurrentManipulator );
	fieldsCtrl.SetWindowText( szText.c_str() );

  if ( bCleared )
	{
    objectsCtrl.DeleteItem( nSelectedItem );
		std::vector<SReferenceObject> temp;
		for ( int i = 0; i < referenceObjects.size(); ++i )
		{
			if ( i != nSelectedItem )
				temp.push_back( referenceObjects[i] );
		}
		temp.swap( referenceObjects ); 
		nSelectedItem = INVALID_NODE_ID;
		CWnd *pwndControl = GetDlgItem( IDC_REF_LIST_EMPTY_CURRENT );
		pwndControl->EnableWindow( false );

	}
	
}

void CRefListDialog::OnClearAll()
{
	CString strMessage( (LPCTSTR)IDS_REF_LIST_EMPTY_ALL_LONG_TIME_WARNING );
	const int nResult = 
		MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_OKCANCEL );
	if ( nResult == IDOK )
	{
		CWaitCursor wait;
		pCurrentManipulator = 0;
		nSelectedItem = INVALID_NODE_ID;
		// Shared with the wx dialog; see RefListView.h.
		const bool bSuccess = NRefList::ClearAll( referenceObjects, szTargetTypeName, szTargetName );
		if ( !bSuccess )
		{
			CString strMessage( (LPCTSTR)IDS_REF_LIST_SET_EMPTY_FAILURE );
			MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONWARNING | MB_OK );
		}
		CResizeDialog::OnOK();
	}
}


