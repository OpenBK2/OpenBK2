#include "StdAfx.h"
#include "resource.h"

#include "StatisticDialog.h"

const int CStatisticDialog::vID[] = 
{
	IDC_STATS_TREE,	//0
	IDOK,						//1
};


CStatisticDialog::CStatisticDialog( CWnd* pParent )
	: CResizeDialog( CStatisticDialog::IDD, pParent ), pELK( 0 )
{
	SetControlStyle( IDC_STATS_TREE, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDOK, ANCHORE_HOR_CENTER | ANCHORE_BOTTOM );
}

const int   STATICSTIC_COLUMN_START = 0;
const int   STATICSTIC_COLUMN_COUNT = 5;
const char *STATICSTIC_COLUMN_NAME  [STATICSTIC_COLUMN_COUNT] = { _T( "Tree" ), _T( "Items" ), _T( "Words" ), _T( "Word Symbols" ), _T( "Symbols" ) };
const int   STATICSTIC_COLUMN_FORMAT[STATICSTIC_COLUMN_COUNT] = { LVCFMT_LEFT, LVCFMT_RIGHT, LVCFMT_RIGHT, LVCFMT_RIGHT, LVCFMT_RIGHT };
const int		STATICSTIC_COLUMN_WIDTH [STATICSTIC_COLUMN_COUNT] = { 200, 100, 100, 100, 100 };


void CStatisticDialog::GetRegistryKey( string *pszRegistryKey )
{
	if ( pszRegistryKey )
	{
		CString strPath;
		CString strProgramKey;
		CString strKey;
		strPath.LoadString( IDS_REGISTRY_PATH );
		strProgramKey.LoadString( AFX_IDS_APP_TITLE );
		strKey.LoadString( IDS_STATS_REGISTRY_KEY );
		( *pszRegistryKey ) = StrFmt( _T( "Software\\%s\\%s\\%s" ), LPCTSTR( strPath ), LPCTSTR( strProgramKey ), LPCTSTR( strKey ) );
	}
}


void CStatisticDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStatisticDialog, CResizeDialog)
END_MESSAGE_MAP()


BOOL CStatisticDialog::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	wndTree.SubclassTreeCtrlId( IDC_STATS_TREE, this );
	DWORD dwStyle = TVS_HASBUTTONS |
									TVS_HASLINES |
									TVS_LINESATROOT |
									TVS_SHOWSELALWAYS;
	wndTree.ModifyTreeCtrlStyles( 0, dwStyle, 0, 0 );
	wndTree.ModifyTreeCtrlStyles( 0, 0, 0, TVXS_COLUMNHEADER );
	wndTree.ModifyListCtrlStyles( 0, 0, 0, LVXS_HILIGHTSUBITEMS );
	wndTree.EnableToolTips( true );
	//wndTree.ModifyListCtrlStyles( 0, 0, 0, LVXS_LINESBETWEENITEMS );
	//wndTree.ModifyListCtrlStyles( 0, 0, 0, LVXS_LINESBETWEENCOLUMNS );

	resizeDialogOptions.nParameters.resize( STATICSTIC_COLUMN_COUNT, 0 );

	CreateControls();
	FillTree();

  return true;
}


void CStatisticDialog::InitImageList()
{
	CBitmap bmp;

	// normal tree images
	imageListNormal.Create( 16, 16, true, IMAGE_COUNT, IMAGE_COUNT );
	NI_ASSERT( imageListNormal.m_hImageList != 0, StrFmt( _T( "CStatisticDialog::InitImageLists(), can't create normal image list" ) ) );

	bmp.LoadBitmap( IDB_ELK_STATISTIC_NORMAL_IMAGE_LIST );
	imageListNormal.Add( &bmp, RGB( 255,255,255 ) );
	bmp.DeleteObject();
}


void CStatisticDialog::CreateControls()
{
	InitImageList();

	wndTree.SetImageList( &imageListNormal, TVSIL_NORMAL );
	wndTree.StoreSubItemText( true );

	int nColumnIndex = 0;
	if ( resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] == 0 )
	{
		resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] = STATICSTIC_COLUMN_WIDTH[nColumnIndex];
	}

	wndTree.SetColumnHeading( nColumnIndex, STATICSTIC_COLUMN_NAME[nColumnIndex] );
	wndTree.SetColumnWidth( nColumnIndex, resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] );
	wndTree.SetColumnFormat( nColumnIndex, STATICSTIC_COLUMN_FORMAT[nColumnIndex] );

	for ( nColumnIndex = 1; nColumnIndex < STATICSTIC_COLUMN_COUNT; ++nColumnIndex )
	{
		if ( resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] == 0 )
		{
			resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] = STATICSTIC_COLUMN_WIDTH[nColumnIndex];
		}
		wndTree.InsertColumn( nColumnIndex, STATICSTIC_COLUMN_NAME[nColumnIndex], STATICSTIC_COLUMN_FORMAT[nColumnIndex], resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] );
	}
}

void SizeToStringWithSpaces( DWORD dwSize, CString& szBuffer )
{
  DWORD dwRest = 0;
  DWORD dwOldRest = 0;
  CString szValue;

  dwRest = dwSize - ( dwSize / 1000 ) * 1000;
  dwOldRest = dwRest;

  szValue.Format( _T( "%d" ), dwRest );
  szBuffer.Format( _T( "%s" ), szValue );
  dwSize /= 1000;

  while(dwSize > 0)
  {
    dwRest = dwSize - ( dwSize / 1000 ) * 1000;
    if(dwOldRest < 10)
    {
      szValue.Format( _T( "%d 00%s" ), dwRest, szBuffer );
    }
    else if(dwOldRest < 100)
    {
      szValue.Format( _T( "%d 0%s" ), dwRest, szBuffer );
    }
    else
    {
      szValue.Format( _T( "%d %s" ), dwRest, szBuffer );
    }
    szBuffer.Format( _T( "%s" ), szValue );
    dwOldRest = dwRest;
    dwSize /= 1000;
  }
}


void CStatisticDialog::FillTree()
{
	if ( pELK )
	{
		CString strNumber;
		for ( int nIndex = 0; nIndex < 2; ++nIndex )
		{
			string szBaseName;
			vector<SELKElementStatistic> *pStatistic = 0;
			if ( nIndex == 0 )
			{
				szBaseName = _T( "Original Text" );
				pStatistic = &( pELK->statistic.original );
			}
			else
			{
				szBaseName = _T( "Translation" );
				pStatistic = &( pELK->statistic.translation );
			}
			if ( HTREEITEM originalItem = wndTree.InsertItem( szBaseName.c_str(), IMAGE_ROOT_NOT_TRANSLATED_EXPANDED, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED, TVI_ROOT ) )
			{
				HTREEITEM elementItem = 0;
				for ( int nStateIndex = 0; nStateIndex < SELKTextProperty::STATE_COUNT; ++nStateIndex )
				{
					if ( elementItem = wndTree.InsertItem( SELKTextProperty::STATE_NAMES[nStateIndex], IMAGE_TEXT_NOT_TRANSLATED + nStateIndex, IMAGE_TEXT_NOT_TRANSLATED + nStateIndex, originalItem, elementItem ) )
					{
						int nLocalItemsCount = 0;
						int nLocalWordsCount = 0;
						int nLocalWordSymbolsCount = 0;
						int nLocalSymbolsCount = 0;

						for ( int nELKElementIndex = 0; nELKElementIndex < pELK->elementList.size(); ++nELKElementIndex )
						{
							if ( nELKElementIndex < pStatistic->size() )
							{
								const SELKElementStatistic &rELKElementStatistic = ( *pStatistic )[nELKElementIndex];

								nLocalItemsCount += rELKElementStatistic.states[nStateIndex].nTextsCount;
								nLocalWordsCount += rELKElementStatistic.states[nStateIndex].nWordsCount;
								nLocalWordSymbolsCount += rELKElementStatistic.states[nStateIndex].nWordSymbolsCount;
								nLocalSymbolsCount += rELKElementStatistic.states[nStateIndex].nSymbolsCount;
							}
						}
						SizeToStringWithSpaces( nLocalItemsCount, strNumber );
						wndTree.SetItemText( elementItem, 1, strNumber );
						
						SizeToStringWithSpaces( nLocalWordsCount, strNumber );
						wndTree.SetItemText( elementItem, 2, strNumber );

						SizeToStringWithSpaces( nLocalWordSymbolsCount, strNumber );
						wndTree.SetItemText( elementItem, 3, strNumber );

						SizeToStringWithSpaces( nLocalSymbolsCount, strNumber );
						wndTree.SetItemText( elementItem, 4, strNumber );
					}
				}

				int nItemsCount = 0;
				int nWordsCount = 0;
				int nWordSymbolsCount = 0;
				int nSymbolsCount = 0;

				for ( int nELKElementIndex = 0; nELKElementIndex < pELK->elementList.size(); ++nELKElementIndex )
				{
					if ( nELKElementIndex < pStatistic->size() )
					{
						int nLocalItemsCount = 0;
						int nLocalWordsCount = 0;
						int nLocalWordSymbolsCount = 0;
						int nLocalSymbolsCount = 0;

						const SELKElement &rELKElement = pELK->elementList[nELKElementIndex];
						const SELKElementStatistic &rELKElementStatistic = ( *pStatistic )[nELKElementIndex];

						szBaseName = StrFmt( _T( "%s, VERSION: %s" ), rELKElement.description.szName.c_str(), rELKElement.szVersion.c_str() );
						if ( elementItem = wndTree.InsertItem( szBaseName.c_str(), IMAGE_ROOT_NOT_TRANSLATED_EXPANDED, IMAGE_ROOT_NOT_TRANSLATED_EXPANDED, originalItem, elementItem ) )
						{
							HTREEITEM stateItem = 0;

							for ( int nStateIndex = 0; nStateIndex < SELKTextProperty::STATE_COUNT; ++nStateIndex )
							{
								if ( nStateIndex < rELKElementStatistic.states.size() )
								{
									if ( stateItem = wndTree.InsertItem( SELKTextProperty::STATE_NAMES[nStateIndex], IMAGE_TEXT_NOT_TRANSLATED + nStateIndex, IMAGE_TEXT_NOT_TRANSLATED + nStateIndex, elementItem, stateItem ) )
									{
										SizeToStringWithSpaces( rELKElementStatistic.states[nStateIndex].nTextsCount, strNumber );
										wndTree.SetItemText( stateItem, 1, strNumber );
										
										SizeToStringWithSpaces( rELKElementStatistic.states[nStateIndex].nWordsCount, strNumber );
										wndTree.SetItemText( stateItem, 2, strNumber );

										SizeToStringWithSpaces( rELKElementStatistic.states[nStateIndex].nWordSymbolsCount, strNumber );
										wndTree.SetItemText( stateItem, 3, strNumber );

										SizeToStringWithSpaces( rELKElementStatistic.states[nStateIndex].nSymbolsCount, strNumber );
										wndTree.SetItemText( stateItem, 4, strNumber );

										nLocalItemsCount += rELKElementStatistic.states[nStateIndex].nTextsCount;
										nLocalWordsCount += rELKElementStatistic.states[nStateIndex].nWordsCount;
										nLocalWordSymbolsCount += rELKElementStatistic.states[nStateIndex].nWordSymbolsCount;
										nLocalSymbolsCount += rELKElementStatistic.states[nStateIndex].nSymbolsCount;
									}
								}
							}
							SizeToStringWithSpaces( nLocalItemsCount, strNumber );
							wndTree.SetItemText( elementItem, 1, strNumber );
							
							SizeToStringWithSpaces( nLocalWordsCount, strNumber );
							wndTree.SetItemText( elementItem, 2, strNumber );

							SizeToStringWithSpaces( nLocalWordSymbolsCount, strNumber );
							wndTree.SetItemText( elementItem, 3, strNumber );

							SizeToStringWithSpaces( nLocalSymbolsCount, strNumber );
							wndTree.SetItemText( elementItem, 4, strNumber );

							nItemsCount += nLocalItemsCount;
							nWordsCount += nLocalWordsCount;
							nWordSymbolsCount += nLocalWordSymbolsCount;
							nSymbolsCount += nLocalSymbolsCount;
						}
					}
				}
				SizeToStringWithSpaces( nItemsCount, strNumber );
				wndTree.SetItemText( originalItem, 1, strNumber );
				
				SizeToStringWithSpaces( nWordsCount, strNumber );
				wndTree.SetItemText( originalItem, 2, strNumber );

				SizeToStringWithSpaces( nWordSymbolsCount, strNumber );
				wndTree.SetItemText( originalItem, 3, strNumber );

				SizeToStringWithSpaces( nSymbolsCount, strNumber );
				wndTree.SetItemText( originalItem, 4, strNumber );
			}
		}
		wndTree.ReMeasureAllItems();
		wndTree.Invalidate();
	}
}


void CStatisticDialog::OnOK() 
{
	for ( int nColumnIndex = 0; nColumnIndex < STATICSTIC_COLUMN_COUNT; ++nColumnIndex )
	{
		resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] = wndTree.GetColumnWidth( nColumnIndex );
	}

	CResizeDialog::OnOK();
}


void CStatisticDialog::OnCancel() 
{
	for ( int nColumnIndex = 0; nColumnIndex < STATICSTIC_COLUMN_COUNT; ++nColumnIndex )
	{
		resizeDialogOptions.nParameters[nColumnIndex + STATICSTIC_COLUMN_START] = wndTree.GetColumnWidth( nColumnIndex );
	}

	CResizeDialog::OnCancel();
}


