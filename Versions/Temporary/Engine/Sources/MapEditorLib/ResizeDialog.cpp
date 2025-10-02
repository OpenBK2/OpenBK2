#include "stdafx.h"

#include "Tools_Registry.h"
#include "Interface_UserData.h"

#include "ResizeDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP( CResizeDialog, CDialog )
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnNeedToolTipText )
END_MESSAGE_MAP()


int CResizeDialog::SOptions::operator&( IBinSaver &bs )
{
	bs.Add( 1, &rect );	
	bs.Add( 2, &nParameters );	
	bs.Add( 3, &szParameters );	
	bs.Add( 4, &fParameters );
	return 0;
}


int CResizeDialog::SOptions::operator&( IXmlSaver &xs )
{
	xs.Add( "Rect", &rect );	
	xs.Add( "IntParameterss", &nParameters );	
	xs.Add( "StringParameters", &szParameters );	
	xs.Add( "FloatParameters", &fParameters );
	return 0;
}


CResizeDialog::CResizeDialog( UINT nIDTemplate, CWnd* pParent )
	: CDialog( nIDTemplate, pParent ),
	resizeDialogOriginalSize( 0, 0 )
{
}


void CResizeDialog::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );
}


BOOL CResizeDialog::OnEraseBkgnd( CDC* pDC )
{
	return CDialog::OnEraseBkgnd(pDC);
}


BOOL CResizeDialog::OnInitDialog() 
{
	CDialog ::OnInitDialog();

	//получить размеры и позиции элементов
	UpdateControlPositions();

	LoadResizeDialogOptions();
	if ( IsRestoreSize() && ( resizeDialogOptions.rect.Width() != 0 ) && ( resizeDialogOptions.rect.Height() != 0 ) )
	{
		MoveWindow( resizeDialogOptions.rect.minx,
								resizeDialogOptions.rect.miny,
								resizeDialogOptions.rect.Width(),
								resizeDialogOptions.rect.Height() );
	}
	EnableToolTips( IsToolTipsEnable() );
	return true;
}


void CResizeDialog::UpdateControlPositions()
{
	RECT clientRect;
	GetClientRect( &clientRect );

 	resizeDialogOriginalSize.x = clientRect.right - clientRect.left;
	resizeDialogOriginalSize.y = clientRect.bottom - clientRect.top;

	for ( hash_map<UINT, SControlStyle>::iterator itControlStyle = resizeDialogControlStyles.begin(); itControlStyle != resizeDialogControlStyles.end(); ++itControlStyle )
	{
		if ( CWnd* pControlWnd = GetDlgItem( itControlStyle->first ) )
		{
			RECT controlWindowRect;
			pControlWnd->GetWindowRect( &controlWindowRect );
			ScreenToClient( &controlWindowRect );

			itControlStyle->second.position = CTRect<int>( controlWindowRect.left,
																										 controlWindowRect.top,
																										 controlWindowRect.right,
																										 controlWindowRect.bottom );
		}
	}
}


void CResizeDialog::OnOK() 
{
	CRect rect;
	GetWindowRect( &rect );
	resizeDialogOptions.rect = CTRect<int>( rect.left, rect.top, rect.right, rect.bottom );
	SaveResizeDialogOptions();
	CDialog::OnOK();
}


void CResizeDialog::OnCancel() 
{
	CRect rect;
	GetWindowRect( &rect );
	resizeDialogOptions.rect = CTRect<int>( rect.left, rect.top, rect.right, rect.bottom );
	SaveResizeDialogOptions();
	CDialog::OnCancel();
}


void CResizeDialog::SetControlStyle( UINT nControlID, DWORD dwStyle, float fHorCenterAnchorRatio, float fVerCenterAnchorRatio, float fHorResizeRatio, float fVerResizeRatio )
{
	SControlStyle controlStyle;
	controlStyle.position = CTRect<int>( 0, 0 ,0, 0 );
	controlStyle.dwStyle = dwStyle;
	controlStyle.fHorCenterAnchorRatio = fHorCenterAnchorRatio; 
	controlStyle.fVerCenterAnchorRatio = fVerCenterAnchorRatio; 
	controlStyle.fHorResizeRatio = fHorResizeRatio;
	controlStyle.fVerResizeRatio = fVerResizeRatio;
	//
	resizeDialogControlStyles[nControlID] = controlStyle;
}

//ANCHORE_LEFT				resize - относительно левого края ( центральная линия - 0.0f )
//ANCHORE_RIGHT				resize - относительно правого края ( центральная линия - 1.0f )
//ANCHORE_HOR_CENTER	resize - относительно центральной линии ( центральная линия - fHorCenterAnchorRatio)
//RESIZE_HOR					дополнительно меняем размер на fHorResizeRatio

void CResizeDialog::OnSize( UINT nType, int cx, int cy ) 
{
	CDialog::OnSize( nType, cx, cy );
	
	CTPoint<int> dSize( ( cx - resizeDialogOriginalSize.x ), ( cy - resizeDialogOriginalSize.y ) );
	for ( hash_map<UINT, SControlStyle>::iterator itControlStyle = resizeDialogControlStyles.begin(); itControlStyle != resizeDialogControlStyles.end(); ++itControlStyle )
	{
		if ( CWnd* pControlWnd = GetDlgItem( itControlStyle->first ) )
		{
			RECT controlWindowRect;
			pControlWnd->GetWindowRect( &controlWindowRect );
			ScreenToClient( &controlWindowRect );

			CTRect<int> newPosition( itControlStyle->second.position );
			CTRect<int> oldPosition( itControlStyle->second.position );

			
			if ( ( itControlStyle->second.dwStyle & ANCHORE_LEFT ) == ANCHORE_LEFT )
			{
				if ( ( itControlStyle->second.dwStyle & RESIZE_HOR ) == RESIZE_HOR )
				{
					newPosition.minx = oldPosition.minx + dSize.x * 0.0f;
					newPosition.maxx = oldPosition.maxx + dSize.x * itControlStyle->second.fHorResizeRatio;
				}
				else
				{
					//do nothing
					newPosition.minx = oldPosition.minx + dSize.x * 0.0f;
					newPosition.maxx = oldPosition.maxx + dSize.x * 0.0f;
				}
			}
			else if ( ( itControlStyle->second.dwStyle & ANCHORE_RIGHT ) == ANCHORE_RIGHT )
			{
				if ( ( itControlStyle->second.dwStyle & RESIZE_HOR ) == RESIZE_HOR )
				{
					newPosition.minx = oldPosition.minx + dSize.x * ( 1.0f - itControlStyle->second.fHorResizeRatio );
					newPosition.maxx = oldPosition.maxx + dSize.x * 1.0f;
				}
				else
				{
					newPosition.minx = oldPosition.minx + dSize.x * 1.0f;
					newPosition.maxx = oldPosition.maxx + dSize.x * 1.0f;
				}
			
			}
			else if ( ( itControlStyle->second.dwStyle & ANCHORE_HOR_CENTER ) == ANCHORE_HOR_CENTER )
			{
				if ( ( itControlStyle->second.dwStyle & RESIZE_HOR ) == RESIZE_HOR )
				{
					newPosition.minx = oldPosition.minx + dSize.x * ( itControlStyle->second.fHorCenterAnchorRatio - ( itControlStyle->second.fHorResizeRatio / 2.0f ) );
					newPosition.maxx = oldPosition.maxx + dSize.x * ( itControlStyle->second.fHorCenterAnchorRatio + ( itControlStyle->second.fHorResizeRatio / 2.0f ) );
				}
				else
				{
					newPosition.minx = oldPosition.minx + dSize.x * itControlStyle->second.fHorCenterAnchorRatio;
					newPosition.maxx = oldPosition.maxx + dSize.x * itControlStyle->second.fHorCenterAnchorRatio;
				}
			}
			//
			if ( ( itControlStyle->second.dwStyle & ANCHORE_TOP ) == ANCHORE_TOP )
			{
				if ( ( itControlStyle->second.dwStyle & RESIZE_VER ) == RESIZE_VER )
				{
					newPosition.miny = oldPosition.miny + dSize.y * 0.0f;
					newPosition.maxy = oldPosition.maxy + dSize.y * itControlStyle->second.fVerResizeRatio;
				}
				else
				{
					//do nothing
					newPosition.miny = oldPosition.miny + dSize.y * 0.0f;
					newPosition.maxy = oldPosition.maxy + dSize.y * 0.0f;
				}
			}
			else if ( ( itControlStyle->second.dwStyle & ANCHORE_BOTTOM ) == ANCHORE_BOTTOM )
			{
				if ( ( itControlStyle->second.dwStyle & RESIZE_VER ) == RESIZE_VER )
				{
					newPosition.miny = oldPosition.miny + dSize.y * ( 1.0f - itControlStyle->second.fVerResizeRatio );
					newPosition.maxy = oldPosition.maxy + dSize.y * 1.0f;
				}
				else
				{
					newPosition.miny = oldPosition.miny + dSize.y * 1.0f;
					newPosition.maxy = oldPosition.maxy + dSize.y * 1.0f;
				}
			
			}
			else if ( ( itControlStyle->second.dwStyle & ANCHORE_VER_CENTER ) == ANCHORE_VER_CENTER )
			{
				if ( ( itControlStyle->second.dwStyle & RESIZE_VER ) == RESIZE_VER )
				{
					newPosition.miny = oldPosition.miny + dSize.y * ( itControlStyle->second.fVerCenterAnchorRatio - ( itControlStyle->second.fVerResizeRatio / 2.0f ) );
					newPosition.maxy = oldPosition.maxy + dSize.y * ( itControlStyle->second.fVerCenterAnchorRatio + ( itControlStyle->second.fVerResizeRatio / 2.0f ) );
				}
				else
				{
					newPosition.miny = oldPosition.miny + dSize.y * itControlStyle->second.fVerCenterAnchorRatio;
					newPosition.maxy = oldPosition.maxy + dSize.y * itControlStyle->second.fVerCenterAnchorRatio;
				}
			}
			
			//
			if ( newPosition.minx > newPosition.maxx )
			{
				 newPosition.maxx = newPosition.minx;
			}
			if ( newPosition.miny > newPosition.maxy )
			{
				 newPosition.maxy = newPosition.miny;
			}
			//
			pControlWnd->MoveWindow( newPosition.minx, newPosition.miny, newPosition.Width(), newPosition.Height() );
		}
	}

	InvalidateRect( 0, true );
}


void CResizeDialog::OnSizing( UINT fwSide, LPRECT pRect ) 
{
	CDialog::OnSizing( fwSide, pRect );

	//CRect mainWndRect;
	//GetClientRect( &mainWndRect );
	//CRect newWndRect;
	//GetWindowRect( &newWndRect );

	RECT clientRect;
	GetClientRect( &clientRect );
	RECT windowRect;
	GetWindowRect( &windowRect );


	LONG nXSize = clientRect.right - clientRect.left;
	LONG nYSize = clientRect.bottom - clientRect.top;
	nXSize = ( pRect->right - pRect->left ) - ( ( windowRect.right - windowRect.left ) - nXSize );
	nYSize = ( pRect->bottom - pRect->top ) - ( ( windowRect.bottom - windowRect.top ) - nYSize );

	if ( nXSize < GetMinimumXDimension() )
	{
	if ( ( fwSide == WMSZ_LEFT ) ||
		   ( fwSide == WMSZ_TOPLEFT ) ||
		   ( fwSide == WMSZ_BOTTOMLEFT ) )
		{
			pRect->left = pRect->right - GetMinimumXDimension() - ( ( pRect->right - pRect->left ) - nXSize );
		}
		else
		{
			pRect->right = pRect->left + GetMinimumXDimension() + ( ( pRect->right - pRect->left ) - nXSize );
		}
	}
	if ( nYSize < GetMinimumYDimension() )
	{
		if ( (fwSide == WMSZ_TOP ) ||
		     ( fwSide == WMSZ_TOPLEFT ) ||
		     ( fwSide == WMSZ_TOPRIGHT ) )
		{
			pRect->top = pRect->bottom - GetMinimumYDimension() - ( ( pRect->bottom - pRect->top ) - nYSize );
		}
		else
		{
			pRect->bottom = pRect->top + GetMinimumYDimension() + ( ( pRect->bottom - pRect->top ) - nYSize );
		}
	}
	//DebugTrace( "CResizeDialog::OnSizing[%d, %d]\n", nXSize, nYSize );
}


void CResizeDialog::LoadResizeDialogOptions()
{
	if ( SerializeToRegistry() )
	{
		string szRegistryKey;
		GetRegistryKey( &szRegistryKey );
		if ( !szRegistryKey.empty() )
		{
			CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_READ, szRegistryKey.c_str() );
			
			registrySection.LoadRect( "Rect", "%d", &( resizeDialogOptions.rect ), CTRect<int>( 0, 0, 0, 0 ) );
			int nParameters = 0;
			int szParameters = 0;
			int fParameters = 0;
			registrySection.LoadNumber( "nParams", "%d", &nParameters, 0 );
			registrySection.LoadNumber( "szParams", "%d", &szParameters, 0 );
			registrySection.LoadNumber( "fParams", "%d", &fParameters, 0 );
			for ( int nParameterIndex = 0; nParameterIndex < nParameters; ++nParameterIndex )
			{
				vector<int>::iterator pos = resizeDialogOptions.nParameters.insert( resizeDialogOptions.nParameters.end(), 0 );
				string szFormat = StrFmt( "nParam%d", nParameterIndex );
				registrySection.LoadNumber( szFormat.c_str(), "%d", &( *pos ), 0 );
			}
			for ( int nParameterIndex = 0; nParameterIndex < szParameters; ++nParameterIndex )
			{
				vector<string>::iterator pos = resizeDialogOptions.szParameters.insert( resizeDialogOptions.szParameters.end(), "" );
				string szFormat = StrFmt( "szParam%d", nParameterIndex );
				registrySection.LoadString( szFormat.c_str(), &( *pos ), "" );
			}
			for ( int nParameterIndex = 0; nParameterIndex < fParameters; ++nParameterIndex )
			{
				vector<float>::iterator pos = resizeDialogOptions.fParameters.insert( resizeDialogOptions.fParameters.end(), 0.0f );
				string szFormat = StrFmt( "fParam%d", nParameterIndex );
				registrySection.LoadNumber( szFormat.c_str(), "%g", &( *pos ), 0.0f );
			}
		}
	}
	else
	{
		string szLabel;
		GetXMLFilePath( &szLabel );
		if ( !szLabel.empty() )
		{
			LoadXMLResource( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + RESIZE_DIALOG_OPTIONS_FILE_NAME + szLabel, ".xml", szLabel, resizeDialogOptions );
		}
	}
}
	

void CResizeDialog::SaveResizeDialogOptions()
{
	if ( SerializeToRegistry() )
	{
		string szRegistryKey;
		GetRegistryKey( &szRegistryKey );
		if ( !szRegistryKey.empty() )
		{
			CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_WRITE, szRegistryKey.c_str() );
			
			registrySection.SaveRect( "Rect", "%d", resizeDialogOptions.rect );
			registrySection.SaveNumber( "nParams", "%d", resizeDialogOptions.nParameters.size() );
			registrySection.SaveNumber( "szParams", "%d", resizeDialogOptions.szParameters.size() );
			registrySection.SaveNumber( "fParams", "%d", resizeDialogOptions.fParameters.size() );
			for ( int nParameterIndex = 0; nParameterIndex < resizeDialogOptions.nParameters.size(); ++nParameterIndex )
			{
				string szFormat = StrFmt( "nParam%d", nParameterIndex );
				registrySection.SaveNumber( szFormat.c_str(), "%d", resizeDialogOptions.nParameters[nParameterIndex] );
			}
			for ( int nParameterIndex = 0; nParameterIndex < resizeDialogOptions.szParameters.size(); ++nParameterIndex )
			{
				string szFormat = StrFmt( "szParam%d", nParameterIndex );
				registrySection.SaveString( szFormat.c_str(), resizeDialogOptions.szParameters[nParameterIndex] );
			}
			for ( int nParameterIndex = 0; nParameterIndex < resizeDialogOptions.fParameters.size(); ++nParameterIndex )
			{
				string szFormat = StrFmt( "fParam%d", nParameterIndex );
				registrySection.SaveNumber( szFormat.c_str(), "%g", resizeDialogOptions.fParameters[nParameterIndex] );
			}
		}
	}
	else
	{
		string szLabel;
		GetXMLFilePath( &szLabel );
		if ( !szLabel.empty() )
			SaveXMLResource( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + RESIZE_DIALOG_OPTIONS_FILE_NAME + szLabel, ".xml", szLabel, resizeDialogOptions );
	}
}


void CResizeDialog::OnPaint() 
{
	CPaintDC dc( this );

	if ( IsDrawGripper() )
	{
		CRect clientRect;
		GetClientRect( &clientRect );

		clientRect.left = clientRect.right - ::GetSystemMetrics( SM_CXHSCROLL );
		clientRect.top = clientRect.bottom - ::GetSystemMetrics( SM_CYVSCROLL );

		dc.DrawFrameControl( clientRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP );
	}
}


BOOL CResizeDialog::OnNeedToolTipText( UINT id, NMHDR *pTTTStruct, LRESULT *pResult )
{
	if ( pTTTStruct )
	{
		TOOLTIPTEXT *pTTT = (TOOLTIPTEXT*)( pTTTStruct );
		UINT nID = pTTTStruct->idFrom;
		if ( ( pTTT->uFlags & TTF_IDISHWND ) != 0 )
		{
			nID = ::GetDlgCtrlID( (HWND)( nID ) );
		}
		if ( nID )
		{
			if ( GetToolTipText( &szToolTipText, nID ) )
			{
				pTTT->lpszText = (LPSTR)( szToolTipText.c_str() );
				pTTT->hinst = 0;
			}
			else
			{
				pTTT->lpszText = MAKEINTRESOURCE( nID );
				pTTT->hinst = GetResourceHandle();
				if ( pTTT->hinst == 0 )
				{
					pTTT->hinst = AfxGetResourceHandle();
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}

// basement storage  

