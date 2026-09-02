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


void CRefListDialog::SetData( const string &szObjectTypeName, const string &szObjectName, list<string> *_pReferenceObjectsList )
{
	szTargetTypeName = szObjectTypeName;
	szTargetName = szObjectName;
	pReferenceObjectsList = _pReferenceObjectsList;
	CStringManager::GetRefValueFromTypeAndName( &szTargetFullName, szTargetTypeName, szTargetName, TYPE_SEPARATOR_CHAR );
}

void CRefListDialog::BuildReferenceObjectsList()
{
	ASSERT( szTargetTypeName.empty() == false );

	list<string> &referenceObjectsList = *pReferenceObjectsList;

	string szRefObjectTypeName;
	string szRefObjectName;
	string szFullName;

	std::unordered_map<string,SReferenceObject> objsHash;
	list<string> fullNames;
	for ( list<string>::const_iterator i = referenceObjectsList.begin(); i != referenceObjectsList.end(); ++i )
	{
    CStringManager::GetTypeAndNameFromRefValue( &szRefObjectTypeName, &szRefObjectName, (*i), TYPE_SEPARATOR_CHAR, "" );
		if ( !szRefObjectTypeName.empty() )
		{
			CStringManager::GetRefValueFromTypeAndName( &szFullName, szRefObjectTypeName, szRefObjectName, TYPE_SEPARATOR_CHAR );
			SReferenceObject obj;
			obj.szObjectName = szRefObjectName;
			obj.szTypeName = szRefObjectTypeName;
			objsHash[szFullName] = obj;
			fullNames.push_back( szFullName );
		}
		else
		{
			// something wrong with the supplied object's subscript (full name)
			ASSERT( false && "Invalid referencing objects were encountered" );
		}
	}
	fullNames.sort();
	
	int nElement = 0;
	for ( list<string>::iterator it = fullNames.begin(); it != fullNames.end(); ++it )
	{
		const SReferenceObject& obj = objsHash[*it];
    referenceObjects.push_back( obj );
		const string szItemText = obj.szTypeName + TYPE_SEPARATOR_CHAR + obj.szObjectName;
		objectsCtrl.InsertItem( nElement, szItemText.c_str() );
		++nElement;
	}

}

void CRefListDialog::BuildFieldsListForObject( const SReferenceObject &object )
{
	const string &szName = object.szObjectName;
	const string &szTypeName = object.szTypeName;

	string szFieldName;
	string szRefTargetTypeName;
	string szRefTargetName;

	string szText;

	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	NI_VERIFY( pResourceManager, "Cannot find resource manager", return )
	pCurrentManipulator = pResourceManager->CreateObjectManipulator( szTypeName, szName );
	currentFields.clear();
	if ( pCurrentManipulator )
	{
		CPtr<IManipulatorIterator> pFieldIt = pCurrentManipulator->Iterate( true, ECT_NO_CACHE );
		while ( !pFieldIt->IsEnd() )
		{
			pFieldIt->GetName( &szFieldName );
			if ( CManipulatorManager::GetParamsFromReference( szFieldName, pCurrentManipulator, &szRefTargetTypeName, &szRefTargetName, 0 )
				&& szRefTargetTypeName == szTargetTypeName && szRefTargetName == szTargetName )
			{
				currentFields.push_back( szFieldName );
				szText += szFieldName + "\r\n";
			}
			pFieldIt->Next();
		}
	}
	else
	{
		szText = "Object has disappeared from the base since RefList was constructed";
		// object has disappeared from the base since RefList was constructed
	}
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
	string szText = "No object selected.";
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

	const CVariant nullRef;
	bool bEverythingIsOK = true;

	string szText = "Clearing..\r\n";
	if ( pCurrentManipulator )
	{
		for ( list<string>::iterator it = currentFields.begin(); it != currentFields.end() && bEverythingIsOK; )
		{
			szText += string( *it );
			if ( pCurrentManipulator->SetValue( *it, nullRef ) )
			{
				szText += " - ok\r\n";
				it = currentFields.erase( it );
			}
			else
			{
				szText += " - cannot set!\r\n";
				bEverythingIsOK = false;
			}
			fieldsCtrl.SetWindowText( szText.c_str() );
		}
	}

  if ( currentFields.empty() )
	{
		szText += "Complete.\r\n";
		fieldsCtrl.SetWindowText( szText.c_str() );
    objectsCtrl.DeleteItem( nSelectedItem );
		vector<SReferenceObject> temp;
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
		bool bSuccess = true;
		string szFieldName;
		string szRefTargetTypeName;
		string szRefTargetName;
		const CVariant nullRef;
		pCurrentManipulator = 0;
		nSelectedItem = INVALID_NODE_ID;
		for ( int i = 0; i < referenceObjects.size(); ++i )
		{
			const string &szName = referenceObjects[i].szObjectName;
			const string &szTypeName = referenceObjects[i].szTypeName;

			IResourceManager *pResourceManager = Singleton<IResourceManager>();
			CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( szTypeName, szName );
			if ( pManipulator )
			{
				CPtr<IManipulatorIterator> pFieldIt = pManipulator->Iterate( true, ECT_NO_CACHE );
				while ( !pFieldIt->IsEnd() )
				{
					pFieldIt->GetName( &szFieldName );
					if ( CManipulatorManager::GetParamsFromReference( szFieldName, pManipulator, &szRefTargetTypeName, &szRefTargetName, 0 )
						&& szRefTargetTypeName == szTargetTypeName && szRefTargetName == szTargetName )
					{
						bSuccess = bSuccess && pManipulator->SetValue( szFieldName, nullRef );
					}
					pFieldIt->Next();
				}
			}
		}
		if ( !bSuccess )
		{
			CString strMessage( (LPCTSTR)IDS_REF_LIST_SET_EMPTY_FAILURE );
			MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONWARNING | MB_OK );
		}
		CResizeDialog::OnOK();
	}
}


