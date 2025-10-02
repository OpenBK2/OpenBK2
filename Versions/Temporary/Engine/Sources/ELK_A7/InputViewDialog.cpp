#include "StdAfx.h"
#include "resource.h"
#include "InputViewDialog.h"
#include "WMDefines.h"
#include "ELK_Types.h"
#include "../Misc/2DArray.h"
#include "../Image/Image.h"
#include "../Image/ImageDDS.h"
#include "../MapEditorLib/Tools_Image.h"
#include "../System/VFSOperations.h"

const int CInputViewDialog::vID[] = 
{
	IDC_IV_ORIGINAL_LABEL,										//0
	IDC_IV_ORIGINAL_EDIT,											//1
	IDC_IV_ORIGINAL_DESCRIPTION_LABEL,				//2
	IDC_IV_ORIGINAL_DESCRIPTION_EDIT,					//3
	IDC_IV_TRANSLATE_LABEL,										//4
	IDC_IV_TRANSLATE_EDIT,										//5
	IDC_IV_STATE_LABEL,												//6
	IDC_IV_STATE_FRAME_LABEL,									//7
	IDC_IV_NOT_TRANSLATED_RADIO_BUTTON,				//8
	IDC_IV_OUTDATED_RADIO_BUTTON,							//9
	IDC_IV_TRANSLATED_RADIO_BUTTON,						//10
	IDC_IV_APPROVED_RADIO_BUTTON,							//11
	IDC_IV_NOT_TRANSLATED_RADIO_BUTTON_LABEL,	//12
	IDC_IV_OUTDATED_RADIO_BUTTON_LABEL,				//13
	IDC_IV_TRANSLATED_RADIO_BUTTON_LABEL,			//14
	IDC_IV_APPROVED_RADIO_BUTTON_LABEL,				//15
	IDC_IV_IMAGE_BORDER,											//16
	IDC_IV_NOT_TRANSLATED_RADIO_BUTTON_BITMAP,//17
	IDC_IV_OUTDATED_RADIO_BUTTON_BITMAP,			//18
	IDC_IV_TRANSLATED_RADIO_BUTTON_BITMAP,		//19
	IDC_IV_APPROVED_RADIO_BUTTON_BITMAP,			//20
};


CInputViewDialog::CInputViewDialog( CWnd* pParent )
: CResizeDialog( CInputViewDialog::IDD, pParent ), hNextIcon( 0 ), pwndMainFrame( 0 ), bTranslatedTextChanged( false ), bManualState( false ), nInitialState( SELKTextProperty::STATE_NOT_TRANSLATED )

{
	SetControlStyle( IDC_IV_ORIGINAL_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_ORIGINAL_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f );
	
	SetControlStyle( IDC_IV_ORIGINAL_DESCRIPTION_LABEL, ANCHORE_LEFT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_ORIGINAL_DESCRIPTION_EDIT, ANCHORE_LEFT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	
	SetControlStyle( IDC_IV_TRANSLATE_LABEL, ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_TRANSLATE_EDIT, ANCHORE_RIGHT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f );
	
	SetControlStyle( IDC_IV_STATE_LABEL, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_STATE_FRAME_LABEL, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	
	SetControlStyle( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_OUTDATED_RADIO_BUTTON, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_TRANSLATED_RADIO_BUTTON, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_APPROVED_RADIO_BUTTON, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	
	SetControlStyle( IDC_IV_IMAGE_BORDER, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON_BITMAP, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_OUTDATED_RADIO_BUTTON_BITMAP, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_TRANSLATED_RADIO_BUTTON_BITMAP, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDC_IV_APPROVED_RADIO_BUTTON_BITMAP, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );

	SetControlStyle( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON_LABEL, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_OUTDATED_RADIO_BUTTON_LABEL, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_TRANSLATED_RADIO_BUTTON_LABEL, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_APPROVED_RADIO_BUTTON_LABEL, ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f );
	SetControlStyle( IDC_IV_IMAGE_BORDER, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
}


void CInputViewDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, m_NotTranslatedButton);
	DDX_Control(pDX, IDC_IV_OUTDATED_RADIO_BUTTON, m_OutdatedButton);
	DDX_Control(pDX, IDC_IV_TRANSLATED_RADIO_BUTTON, m_TranslatedButton);
	DDX_Control(pDX, IDC_IV_APPROVED_RADIO_BUTTON, m_ApprovedButton);
	DDX_Control(pDX, IDC_IV_TRANSLATE_EDIT, m_TranslateEdit);
	DDX_Control(pDX, IDC_IV_ORIGINAL_EDIT, m_OriginalEdit);
	DDX_Control(pDX, IDC_IV_ORIGINAL_DESCRIPTION_EDIT, m_DescriptionEdit);
	DDX_Control(pDX, IDC_IV_NEXT_BUTTON, m_NextButton);
	DDX_Control(pDX, IDC_IV_BACK_BUTTON, m_BackButton);
}


BEGIN_MESSAGE_MAP(CInputViewDialog, CResizeDialog)
	ON_BN_CLICKED(IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, OnNotTranslatedRadioButton)
	ON_BN_CLICKED(IDC_IV_TRANSLATED_RADIO_BUTTON, OnTranslatedRadioButton)
	ON_BN_CLICKED(IDC_IV_APPROVED_RADIO_BUTTON, OnApprovedRadioButton)
	ON_EN_CHANGE(IDC_IV_TRANSLATE_EDIT, OnChangeTranslateEdit)
	ON_BN_CLICKED(IDC_IV_BACK_BUTTON, OnBackButton)
	ON_BN_CLICKED(IDC_IV_NEXT_BUTTON, OnNextButton)
	ON_WM_PAINT()
END_MESSAGE_MAP()


BOOL CInputViewDialog::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	
	hBackIcon = AfxGetApp()->LoadIcon( IDI_IV_BACK );
	m_BackButton.SetIcon( hBackIcon );
	
	hNextIcon = AfxGetApp()->LoadIcon( IDI_IV_NEXT );
	m_NextButton.SetIcon( hNextIcon );

	return true;
}


void CInputViewDialog::OnNotTranslatedRadioButton() 
{
	bManualState = true;
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		CheckRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON, IDC_IV_NOT_TRANSLATED_RADIO_BUTTON );
		pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_STATE_CHANGED, SELKTextProperty::STATE_NOT_TRANSLATED );
	}
}


void CInputViewDialog::OnTranslatedRadioButton() 
{
	bManualState = true;
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		CheckRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON, IDC_IV_TRANSLATED_RADIO_BUTTON );
		pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_STATE_CHANGED, SELKTextProperty::STATE_TRANSLATED );
	}
}


void CInputViewDialog::OnApprovedRadioButton() 
{
	bManualState = true;
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		CheckRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON );
		pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_STATE_CHANGED, SELKTextProperty::STATE_APPROVED );
	}
}


void CInputViewDialog::OnChangeTranslateEdit() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_TRANSLATION_CHANGED, 0 );
	}

	bool bPreviousTranslatedTextChanged = bTranslatedTextChanged;
	CString strActualText;
	if ( CWnd *pWnd = GetDlgItem( IDC_IV_TRANSLATE_EDIT ) )
	{
		pWnd->GetWindowText( strActualText );
	}
	bTranslatedTextChanged = ( strInitialTranslatedText != strActualText );

	if ( ( bTranslatedTextChanged != bPreviousTranslatedTextChanged ) && ( !bManualState ) )
	{
		if ( bTranslatedTextChanged )
		{
			CheckRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON, IDC_IV_NOT_TRANSLATED_RADIO_BUTTON + SELKTextProperty::STATE_TRANSLATED );
			if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
			{
				pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_STATE_CHANGED, SELKTextProperty::STATE_TRANSLATED );
			}
		}
		else
		{
			CheckRadioButton( IDC_IV_NOT_TRANSLATED_RADIO_BUTTON, IDC_IV_APPROVED_RADIO_BUTTON, IDC_IV_NOT_TRANSLATED_RADIO_BUTTON + nInitialState );
			if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
			{
				pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_STATE_CHANGED, nInitialState );
			}
		}
	}
}


void CInputViewDialog::OnNextButton() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_NEXT_BUTTON_PRESED, 0 );
	}
}


void CInputViewDialog::OnBackButton() 
{
	if ( pwndMainFrame && ( pwndMainFrame->GetSafeHwnd() != 0 ) )
	{
		pwndMainFrame->SendMessage( WM_INPUT_FORM_NOTIFY, IFN_BACK_BUTTON_PRESED, 0 );
	}
}


void CInputViewDialog::LoadGameImage( const string &rszGameImagePath )
{
	szGameImagePath = rszGameImagePath;
	if ( !szGameImagePath.empty() )
	{
		CFileStream dataStream( NVFS::GetMainVFS(), szGameImagePath );
		if ( dataStream.IsOk() )
		{
			CArray2D<DWORD> imageSource;
			NImage::LoadImageDDS( &imageSource, &dataStream );
			gameImageBitmap.DeleteObject();
			NImage::Load2Bitmap( &gameImageBitmap, imageSource );
		}
	}
	
	if ( CWnd *pWnd = GetDlgItem( IDC_IV_IMAGE_BORDER ) )
	{
		CRect rect;
		pWnd->GetWindowRect( &rect );
		ScreenToClient( &rect );
		InvalidateRect( rect, false );
	}
}


void CInputViewDialog::OnPaint() 
{
	CPaintDC dc(this);
	
	if ( CWnd *pWnd = GetDlgItem( IDC_IV_IMAGE_BORDER ) )
	{
		CRect rect;
		pWnd->GetWindowRect( &rect );
		ScreenToClient( &rect );
		rect.left += 2;
		rect.right -= 3;
		rect.top += 2;
		rect.bottom -= 3;

		if ( ( rect.Width() > 0 ) && ( rect.Height() > 0 ) )
		{
			dc.FillSolidRect( rect, RGB( 0x92, 0x92, 0x92 ) );
			if ( !szGameImagePath.empty() )
			{
				CDC memDC;
				int nRes = memDC.CreateCompatibleDC( &dc );
				CBitmap *pOldBitmap = memDC.SelectObject( &gameImageBitmap );
				dc.StretchBlt( rect.left + ( rect.Width() - gameImageSize.x ) / 2,
											 rect.top + ( rect.Height() - gameImageSize.y ) / 2,
											 gameImageSize.x,
											 gameImageSize.y,
											 &memDC,
											 0, 0, gameImageSize.x, gameImageSize.y,
											 SRCCOPY );
				//::SetBrushOrgEx( pDC->m_hDC, orgPoint.x, orgPoint.y, &point );
				memDC.SelectObject( pOldBitmap );
			}
		}
	}
	// Do not call CResizeDialog::OnPaint() for painting messages
}


// basement storage  


