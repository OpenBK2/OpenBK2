#include "stdafx.h"
#include "MDDLDialog.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "libdb/Manipulator.h"

CMDDLDialog::CMDDLDialog() : CDialog( CMDDLDialog::IDD, 0 ), nCommandID( INVALID_NODE_ID ) {}


void CMDDLDialog::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_MDDL_OPERATIONS_LIST, wndValueList);
}


BEGIN_MESSAGE_MAP(CMDDLDialog, CDialog)
	ON_WM_NCDESTROY()
	ON_LBN_SELCHANGE(IDC_MDDL_OPERATIONS_LIST, OnSelChange)
	ON_LBN_KILLFOCUS(IDC_MDDL_OPERATIONS_LIST, OnListKillFocus)
END_MESSAGE_MAP()


void CMDDLDialog::SetParams( unsigned _nCommandID, const CDescriptionList &rValueList )
{
	nCommandID = _nCommandID;
	valueList = rValueList;
	//
	CreateList();
}


void CMDDLDialog::CreateList()
{
	wndValueList.ResetContent();
	int nUndoDepth = 0;
	for ( CDescriptionList::const_iterator itValue = valueList.begin(); itValue != valueList.end(); ++itValue )
	{
		const int nInsertedIndex = wndValueList.InsertString( -1, *itValue );
		if ( nInsertedIndex != LB_ERR )
		{
			wndValueList.SetItemData( nInsertedIndex, nUndoDepth );
		}
		++nUndoDepth;
	}
	wndValueList.SetCurSel( 0 );
}


BOOL CMDDLDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	return true;
}


void CMDDLDialog::OnSelChange()
{
	ShowWindow( SW_HIDE );
	int nSelectedValue = wndValueList.GetCurSel();
	nSelectedValue = wndValueList.GetItemData( nSelectedValue );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CONTROLLER_CONTAINER, nCommandID, nSelectedValue );
}


void CMDDLDialog::OnOK()
{
}


void CMDDLDialog::OnCancel()
{
	ShowWindow( SW_HIDE );
}


void CMDDLDialog::OnListKillFocus()
{
	ShowWindow( SW_HIDE );
}

// basement storage  


