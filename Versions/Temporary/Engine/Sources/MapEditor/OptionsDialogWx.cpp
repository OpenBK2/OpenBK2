#include "stdafx.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/DefaultsOptions.h"
#include "MapEditorLib/IconRenderingOptions.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxOwnership.h"
#include "OptionsDialogWx.h"

#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/simplebook.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/textctrl.h>

namespace
{
class COptionsDialog : public wxDialog
{
	wxSpinCtrl *pWidth;
	wxSpinCtrl *pHeight;
	wxCheckBox *pSeparatePak;
	wxTextCtrl *pDeployTemplate;

	bool Apply()
	{
		// Validate the text too: an unfinished or invalid edit must not silently
		// reuse the spin control's previous value for a batch export.
		long nWidth = 0;
		long nHeight = 0;
		if ( !pWidth->GetTextValue().ToLong( &nWidth ) || !pHeight->GetTextValue().ToLong( &nHeight ) || nWidth < 1 ||
		     nWidth > NIconRendering::TARGET_SIZE || nHeight < 1 || nHeight > NIconRendering::TARGET_SIZE )
		{
			wxMessageBox(
					"Enter a whole number from 1 to 512 for each dimension.", "Icon Rendering", wxOK | wxICON_ERROR, this );
			return false;
		}
		NIconRendering::SOptions options;
		options.nWidth = static_cast<int>( nWidth );
		options.nHeight = static_cast<int>( nHeight );
		options.bSeparatePak = pSeparatePak->GetValue();
		if ( !NIconRendering::SaveOptions( options ) )
		{
			wxMessageBox( "Could not save the icon rendering settings.", "Options", wxOK | wxICON_ERROR, this );
			return false;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_ICON_RENDERING, ID_ICON_SETTINGS_CHANGED, 0 );
		NEditorDefaults::SOptions defaults;
		defaults.szDeployTemplate = pDeployTemplate->GetValue().ToStdString( wxConvUTF8 );
		if ( !NEditorDefaults::SaveOptions( defaults ) )
		{
			wxMessageBox( "Could not save the default settings.", "Options", wxOK | wxICON_ERROR, this );
			return false;
		}
		return true;
	}

public:
	explicit COptionsDialog( wxWindow *pParent )
		: wxDialog(
					pParent, wxID_ANY, "Options", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
	{
		const NIconRendering::SOptions options = NIconRendering::LoadOptions();
		auto *pRoot = new wxBoxSizer( wxVERTICAL );
		auto *pBody = new wxBoxSizer( wxHORIZONTAL );
		auto *pTabs = NWx::Child<wxListBox>( this, wxID_ANY );
		pTabs->Append( "Icon Rendering" );
		pTabs->Append( "Defaults" );
		pTabs->SetSelection( 0 );
		pTabs->SetMinSize( FromDIP( wxSize( 155, -1 ) ) );
		auto *pBook = NWx::Child<wxSimplebook>( this, wxID_ANY );
		auto *pPage = NWx::Child<wxPanel>( pBook );
		pBook->AddPage( pPage, "Icon Rendering", true );
		pTabs->Bind( wxEVT_LISTBOX, [pBook]( wxCommandEvent &event ) { pBook->SetSelection( event.GetSelection() ); } );

		auto *pPageSizer = new wxBoxSizer( wxVERTICAL );
		auto *pTitle = NWx::Child<wxStaticText>( pPage, wxID_ANY, "Icon Rendering" );
		pTitle->SetFont( pTitle->GetFont().Bold() );
		pPageSizer->Add( pTitle, 0, wxBOTTOM, FromDIP( 18 ) );
		auto *pDimensions = new wxFlexGridSizer( 2, FromDIP( 12 ), FromDIP( 16 ) );
		pDimensions->AddGrowableCol( 1 );
		pWidth = NWx::Child<wxSpinCtrl>( pPage, wxID_ANY );
		pHeight = NWx::Child<wxSpinCtrl>( pPage, wxID_ANY );
		for ( auto *pControl : { pWidth, pHeight } )
		{
			pControl->SetRange( 1, NIconRendering::TARGET_SIZE );
			pControl->SetMinSize( FromDIP( wxSize( 110, -1 ) ) );
		}
		pWidth->SetValue( options.nWidth );
		pHeight->SetValue( options.nHeight );
		pDimensions->Add( NWx::Child<wxStaticText>( pPage, wxID_ANY, "Icon width (pixels)" ), 0, wxALIGN_CENTER_VERTICAL );
		pDimensions->Add( pWidth, 0, wxEXPAND );
		pDimensions->Add( NWx::Child<wxStaticText>( pPage, wxID_ANY, "Icon height (pixels)" ), 0, wxALIGN_CENTER_VERTICAL );
		pDimensions->Add( pHeight, 0, wxEXPAND );
		pPageSizer->Add( pDimensions, 0, wxEXPAND | wxBOTTOM, FromDIP( 12 ) );
		pPageSizer->Add( NWx::Child<wxStaticText>( pPage, wxID_ANY, "Default: 42 x 42. Range: 1-512 pixels." ),
		                 0,
		                 wxBOTTOM,
		                 FromDIP( 28 ) );
		auto *pRender = NWx::Child<wxButton>( pPage, wxID_ANY, "Re-Render All Mech Units" );
		pRender->Enable( Singleton<ICommandHandlerContainer>()->Get( CHID_ICON_RENDERING ) != nullptr );
		pSeparatePak = NWx::Child<wxCheckBox>( pPage, wxID_ANY, "Save a separate .pak" );
		pSeparatePak->SetValue( options.bSeparatePak );
		pSeparatePak->SetToolTip( "Save MechUnitIcons.pak in the active Data or mod export folder." );
		auto *pBatch = new wxBoxSizer( wxHORIZONTAL );
		pBatch->Add( pRender, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP( 16 ) );
		pBatch->Add( pSeparatePak, 0, wxALIGN_CENTER_VERTICAL );
		pPageSizer->Add( pBatch, 0, wxBOTTOM, FromDIP( 10 ) );
		auto *pHint = NWx::Child<wxStaticText>(
				pPage,
				wxID_ANY,
				"Saves these settings and renders all unit icons.\nProgress and the output path appear in the Log Window." );
		pPageSizer->Add( pHint, 0, wxEXPAND );
		pPage->SetSizer( pPageSizer );
		pRender->Bind( wxEVT_BUTTON,
		               [this, pRender]( wxCommandEvent & )
		               {
										 if ( Apply() )
										 {
											 pRender->Disable();
											 Singleton<ICommandHandlerContainer>()->HandleCommand(
													 CHID_ICON_RENDERING, ID_ICON_RENDER_ALL, 0 );
											 pRender->Enable();
										 }
									 } );

		auto *pDefaultsPage = NWx::Child<wxPanel>( pBook );
		pBook->AddPage( pDefaultsPage, "Defaults" );
		auto *pDefaultsSizer = new wxBoxSizer( wxVERTICAL );
		auto *pDefaultsTitle = NWx::Child<wxStaticText>( pDefaultsPage, wxID_ANY, "Defaults" );
		pDefaultsTitle->SetFont( pDefaultsTitle->GetFont().Bold() );
		pDefaultsSizer->Add( pDefaultsTitle, 0, wxBOTTOM, FromDIP( 18 ) );
		pDefaultsSizer->Add( NWx::Child<wxStaticText>( pDefaultsPage, wxID_ANY, "Reinforcement point deploy template" ),
		                     0,
		                     wxBOTTOM,
		                     FromDIP( 8 ) );
		const auto defaults = NEditorDefaults::LoadOptions();
		pDeployTemplate = NWx::Child<wxTextCtrl>( pDefaultsPage,
		                                          wxID_ANY,
		                                          wxString::FromUTF8( defaults.szDeployTemplate ),
		                                          wxDefaultPosition,
		                                          wxDefaultSize,
		                                          wxTE_READONLY );
		pDeployTemplate->SetName( "DefaultDeployTemplate" );
		pDeployTemplate->SetToolTip( wxString::FromUTF8( defaults.szDeployTemplate ) );
		pDefaultsSizer->Add( pDeployTemplate, 0, wxEXPAND | wxBOTTOM, FromDIP( 10 ) );
		auto *pTemplateButtons = new wxBoxSizer( wxHORIZONTAL );
		auto *pBrowse = NWx::Child<wxButton>( pDefaultsPage, wxID_ANY, "Browse..." );
		auto *pReset = NWx::Child<wxButton>( pDefaultsPage, wxID_ANY, "Use Common Template" );
		pTemplateButtons->Add( pBrowse, 0, wxRIGHT, FromDIP( 10 ) );
		pTemplateButtons->Add( pReset );
		pDefaultsSizer->Add( pTemplateButtons, 0, wxBOTTOM, FromDIP( 18 ) );
		pDefaultsSizer->Add( NWx::Child<wxStaticText>(
				pDefaultsPage,
				wxID_ANY,
				"Assigned automatically to new reinforcement points.\nExisting points keep their templates." ) );
		pDefaultsPage->SetSizer( pDefaultsSizer );
		pBrowse->Bind( wxEVT_BUTTON,
		               [this]( wxCommandEvent & )
		               {
										 CDBID dbid;
										 std::string szType = "DeployTemplate";
										 // Restrict the picker to deploy templates and disallow an empty selection.
										 if ( Singleton<IMainFrameContainer>()->Get()->BrowseForObject( &dbid, &szType, false, false ) )
										 {
											 pDeployTemplate->SetValue( wxString::FromUTF8( dbid.ToString() ) );
											 pDeployTemplate->SetToolTip( pDeployTemplate->GetValue() );
										 }
									 } );
		pReset->Bind( wxEVT_BUTTON,
		              [this]( wxCommandEvent & )
		              {
										pDeployTemplate->SetValue( wxString::FromUTF8( NEditorDefaults::COMMON_DEPLOY_TEMPLATE ) );
										pDeployTemplate->SetToolTip( pDeployTemplate->GetValue() );
									} );

		// A proportional sidebar leaves room for more categories without
		// squeezing the controls when the dialog is resized or DPI changes.
		pBody->Add( pTabs, 1, wxEXPAND | wxRIGHT, FromDIP( 24 ) );
		pBody->Add( pBook, 3, wxEXPAND );
		pRoot->Add( pBody, 1, wxEXPAND | wxALL, FromDIP( 20 ) );
		pRoot->Add( NWx::Child<wxStaticLine>( this ), 0, wxEXPAND );
		pRoot->Add( CreateButtonSizer( wxOK | wxCANCEL | wxAPPLY ), 0, wxEXPAND | wxALL, FromDIP( 12 ) );
		SetSizer( pRoot );
		SetMinSize( FromDIP( wxSize( 720, 390 ) ) );
		SetSize( GetMinSize() );
		CentreOnParent();
		Bind( wxEVT_BUTTON, [this]( wxCommandEvent & ) { Apply(); }, wxID_APPLY );
		Bind(
				wxEVT_BUTTON,
				[this]( wxCommandEvent & )
				{
					if ( Apply() )
					{
						EndModal( wxID_OK );
					}
				},
				wxID_OK );
	}
};
} // namespace

void NOptionsDialog::Show( wxWindow *pParent )
{
	COptionsDialog dialog( pParent );
	dialog.ShowModal();
}
