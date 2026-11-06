#include "MapEditor/stdafx.h"

#include "MapEditor/CommandHandlerContainer.h"
#include "MapEditor/OptionsDialogWx.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/DefaultsOptions.h"
#include "MapEditorLib/IconRenderingOptions.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/ResourceDefines.h"

#include <chrono>
#include <filesystem>
#include <functional>

#include <gtest/gtest.h>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/simplebook.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>

namespace
{
class CTestUserData : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CTestUserData );

public:
	SUserData data;

	SUserData *Get() override
	{
		return &data;
	}

	void Load() override {}

	void Save() override {}
};

class CRenderHandler : public ICommandHandler
{
public:
	int nRenders = 0;
	uintptr_t nBatch = 0;
	NIconRendering::SOptions renderedOptions;

	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override
	{
		if ( nCommandID == ID_ICON_RENDER_ALL )
		{
			++nRenders;
			nBatch = dwData;
			renderedOptions = NIconRendering::LoadOptions();
		}
		return true;
	}

	bool UpdateCommand( unsigned, bool *, bool * ) override
	{
		return false;
	}
};

template <class TControl>
void FindControls( wxWindow *pWindow, std::vector<TControl *> *pControls )
{
	if ( auto *pControl = dynamic_cast<TControl *>( pWindow ) )
	{
		pControls->push_back( pControl );
	}
	for ( wxWindow *pChild : pWindow->GetChildren() )
	{
		FindControls( pChild, pControls );
	}
}

void Click( wxWindow *pWindow, int nID )
{
	wxCommandEvent event( wxEVT_BUTTON, nID );
	pWindow->GetEventHandler()->ProcessEvent( event );
}

void WithDialog( const std::function<void( wxDialog * )> &check )
{
	wxTheApp->CallAfter(
			[&check]()
			{
				for ( wxWindow *pWindow : wxTopLevelWindows )
				{
					auto *pDialog = dynamic_cast<wxDialog *>( pWindow );
					if ( pDialog && pDialog->GetTitle() == "Options" )
					{
						check( pDialog );
						// Even a fatal gtest assertion in check must close the modal dialog.
						pDialog->EndModal( wxID_CANCEL );
						return;
					}
				}
			} );
	NOptionsDialog::Show( nullptr );
}
} // namespace

TEST( IconRenderingOptions, DialogAppliesPersistsAndDispatchesCurrentDimensions )
{
	const auto previousAssert = wxSetAssertHandler(
			[]( const wxString &file, int line, const wxString &, const wxString &condition, const wxString &message )
			{ ADD_FAILURE_AT( file.utf8_str().data(), line ) << condition.ToStdString() << ": " << message.ToStdString(); } );
	wxApp::SetInstance( new wxApp );
	int argc = 1;
	char name[] = "IconRenderingOptions_test";
	char *argv[] = { name, nullptr };
	if ( !wxEntryStart( argc, argv ) )
	{
		wxSetAssertHandler( previousAssert );
		GTEST_SKIP() << "A display is required (use xvfb-run on Linux).";
	}
	const auto root =
			std::filesystem::temp_directory_path() /
			( "obk2-icon-options-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
	std::filesystem::create_directories( root / "Editor" );
	auto *pUser = new CTestUserData;
	pUser->data.constUserData.szStartFolder = root.generic_u8string() + "/";
	NSingleton::RegisterSingleton( pUser, IUserDataContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CCommandHandlerContainer, ICommandHandlerContainer::tidTypeID );
	CRenderHandler handler;
	Singleton<ICommandHandlerContainer>()->Set( CHID_ICON_RENDERING, &handler );
	EXPECT_EQ( NEditorDefaults::COMMON_DEPLOY_TEMPLATE, NEditorDefaults::LoadOptions().szDeployTemplate );

	WithDialog(
			[]( wxDialog *pDialog )
			{
				auto *pTemplate = dynamic_cast<wxTextCtrl *>( pDialog->FindWindow( "DefaultDeployTemplate" ) );
				ASSERT_NE( nullptr, pTemplate );
				EXPECT_EQ( NEditorDefaults::COMMON_DEPLOY_TEMPLATE, pTemplate->GetValue().ToStdString( wxConvUTF8 ) );
				// A cancelled options dialog must discard the template selection too.
				pTemplate->SetValue( "Reinforcements/cancelled.xdb" );
				std::vector<wxSpinCtrl *> controls;
				FindControls( pDialog, &controls );
				ASSERT_EQ( 2u, controls.size() );
				EXPECT_EQ( 42, controls[0]->GetValue() );
				EXPECT_EQ( 42, controls[1]->GetValue() );
				std::vector<wxCheckBox *> checks;
				FindControls( pDialog, &checks );
				ASSERT_EQ( 5u, checks.size() );
				EXPECT_EQ( "Save a separate .pak", checks[0]->GetLabel().ToStdString() );
				EXPECT_TRUE( checks[0]->GetValue() );
				checks[0]->SetValue( false );
				controls[0]->SetValue( 21 );
				controls[1]->SetValue( 84 );
			} );
	EXPECT_EQ( 42, NIconRendering::LoadOptions().nWidth );
	EXPECT_EQ( 42, NIconRendering::LoadOptions().nHeight );
	EXPECT_TRUE( NIconRendering::LoadOptions().bSeparatePak );
	EXPECT_EQ( NEditorDefaults::COMMON_DEPLOY_TEMPLATE, NEditorDefaults::LoadOptions().szDeployTemplate );

	WithDialog(
			[]( wxDialog *pDialog )
			{
				auto *pTemplate = dynamic_cast<wxTextCtrl *>( pDialog->FindWindow( "DefaultDeployTemplate" ) );
				ASSERT_NE( nullptr, pTemplate );
				pTemplate->SetValue( wxString::FromUTF8( u8"Reinforcements/\u00dcberfall.xdb" ) );
				std::vector<wxSpinCtrl *> controls;
				FindControls( pDialog, &controls );
				ASSERT_EQ( 2u, controls.size() );
				controls[0]->SetValue( 84 );
				controls[1]->SetValue( 126 );
				std::vector<wxCheckBox *> checks;
				FindControls( pDialog, &checks );
				ASSERT_EQ( 5u, checks.size() );
				Click( pDialog, wxID_APPLY );
				EXPECT_EQ( u8"Reinforcements/\u00dcberfall.xdb", NEditorDefaults::LoadOptions().szDeployTemplate );
				EXPECT_EQ( 84, NIconRendering::LoadOptions().nWidth );
				EXPECT_EQ( 126, NIconRendering::LoadOptions().nHeight );
				// Re-render must also apply edits made since the last Apply.
				controls[0]->SetValue( 168 );
				checks[0]->SetValue( false );
				std::vector<wxButton *> buttons;
				FindControls( pDialog, &buttons );
				bool bFound = false;
				for ( wxButton *pButton : buttons )
				{
					if ( pButton->GetLabel() == "Re-Render All Mech Units" )
					{
						bFound = true;
						EXPECT_TRUE( pButton->IsEnabled() );
						Click( pButton, pButton->GetId() );
					}
				}
				EXPECT_TRUE( bFound );
			} );
	EXPECT_EQ( 1, handler.nRenders );
	EXPECT_EQ( 168, handler.renderedOptions.nWidth );
	EXPECT_EQ( 126, handler.renderedOptions.nHeight );
	EXPECT_FALSE( handler.renderedOptions.bSeparatePak );
	EXPECT_EQ( NIconRendering::MECH_UNITS, handler.nBatch );
	// Each button must dispatch its own type and persist independent pak flags.
	WithDialog(
			[&handler]( wxDialog *pDialog )
			{
				std::vector<wxCheckBox *> checks;
				FindControls( pDialog, &checks );
				ASSERT_EQ( 5u, checks.size() );
				for ( int i = 1; i < NIconRendering::BATCH_COUNT; ++i )
				{
					EXPECT_TRUE( checks[i]->GetValue() );
					checks[i]->SetValue( i % 2 == 0 );
				}
				std::vector<wxButton *> buttons;
				FindControls( pDialog, &buttons );
				const char *labels[] = {
					"Re-Render All Buildings", "Re-Render All Bridges", "Re-Render All Fences", "Re-Render All Objects" };
				for ( int i = 0; i < 4; ++i )
				{
					wxButton *pRender = nullptr;
					for ( auto *pButton : buttons )
					{
						if ( pButton->GetLabel() == labels[i] )
						{
							pRender = pButton;
						}
					}
					ASSERT_NE( nullptr, pRender );
					EXPECT_TRUE( pRender->IsEnabled() );
					EXPECT_TRUE( pRender->IsShownOnScreen() );
					Click( pRender, pRender->GetId() );
					EXPECT_EQ( i + 1, handler.nBatch );
					EXPECT_EQ( i + 2, handler.nRenders );
					EXPECT_EQ( 168, handler.renderedOptions.nWidth );
					EXPECT_EQ( 126, handler.renderedOptions.nHeight );
					EXPECT_FALSE( handler.renderedOptions.bSeparatePak );
					EXPECT_FALSE( handler.renderedOptions.bBuildingPak );
					EXPECT_TRUE( handler.renderedOptions.bBridgePak );
					EXPECT_FALSE( handler.renderedOptions.bFencePak );
					EXPECT_TRUE( handler.renderedOptions.bObjectPak );
				}
			} );
	WithDialog(
			[]( wxDialog *pDialog )
			{
				std::vector<wxSpinCtrl *> controls;
				FindControls( pDialog, &controls );
				ASSERT_EQ( 2u, controls.size() );
				EXPECT_EQ( 168, controls[0]->GetValue() );
				EXPECT_EQ( 126, controls[1]->GetValue() );
				std::vector<wxCheckBox *> checks;
				FindControls( pDialog, &checks );
				ASSERT_EQ( 5u, checks.size() );
				EXPECT_FALSE( checks[0]->GetValue() );
				EXPECT_FALSE( checks[1]->GetValue() );
				EXPECT_TRUE( checks[2]->GetValue() );
				EXPECT_FALSE( checks[3]->GetValue() );
				EXPECT_TRUE( checks[4]->GetValue() );
				checks[0]->SetValue( true );
				Click( pDialog, wxID_APPLY );
				std::vector<wxListBox *> tabs;
				FindControls( pDialog, &tabs );
				ASSERT_EQ( 1u, tabs.size() );
				EXPECT_EQ( "Icon Rendering", tabs[0]->GetStringSelection().ToStdString() );
				ASSERT_EQ( 2u, tabs[0]->GetCount() );
				EXPECT_EQ( "Defaults", tabs[0]->GetString( 1 ).ToStdString() );
				tabs[0]->SetSelection( 1 );
				wxCommandEvent select( wxEVT_LISTBOX, tabs[0]->GetId() );
				select.SetInt( 1 );
				tabs[0]->GetEventHandler()->ProcessEvent( select );
				std::vector<wxSimplebook *> books;
				FindControls( pDialog, &books );
				ASSERT_EQ( 1u, books.size() );
				EXPECT_EQ( 1, books[0]->GetSelection() );
				auto *pTemplate = dynamic_cast<wxTextCtrl *>( pDialog->FindWindow( "DefaultDeployTemplate" ) );
				ASSERT_NE( nullptr, pTemplate );
				EXPECT_TRUE( pTemplate->IsShownOnScreen() );
				EXPECT_EQ( u8"Reinforcements/\u00dcberfall.xdb", pTemplate->GetValue().ToStdString( wxConvUTF8 ) );
				std::vector<wxButton *> buttons;
				FindControls( pDialog, &buttons );
				wxButton *pReset = nullptr;
				for ( wxButton *pButton : buttons )
				{
					if ( pButton->GetLabel() == "Use Common Template" )
					{
						pReset = pButton;
					}
				}
				ASSERT_NE( nullptr, pReset );
				Click( pReset, pReset->GetId() );
				EXPECT_EQ( NEditorDefaults::COMMON_DEPLOY_TEMPLATE, pTemplate->GetValue().ToStdString( wxConvUTF8 ) );
				Click( pDialog, wxID_APPLY );
			} );
	EXPECT_EQ( NEditorDefaults::COMMON_DEPLOY_TEMPLATE, NEditorDefaults::LoadOptions().szDeployTemplate );
	EXPECT_FALSE( NEditorDefaults::SaveOptions( { "" } ) );
	EXPECT_TRUE( NIconRendering::LoadOptions().bSeparatePak );
	EXPECT_FALSE( NIconRendering::SaveOptions( { 0, 42 } ) );
	EXPECT_EQ( 168, NIconRendering::LoadOptions().nWidth );

	NSingleton::UnRegisterSingleton( ICommandHandlerContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IUserDataContainer::tidTypeID );
	std::error_code error;
	std::filesystem::remove_all( root, error );
	wxTheApp->ProcessPendingEvents();
	wxEntryCleanup();
	wxSetAssertHandler( previousAssert );
}
