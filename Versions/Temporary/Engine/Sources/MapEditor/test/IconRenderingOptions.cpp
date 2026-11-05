#include "MapEditor/stdafx.h"

#include "MapEditor/CommandHandlerContainer.h"
#include "MapEditor/OptionsDialogWx.h"
#include "MapEditorLib/CommandHandlerDefines.h"
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
#include <wx/spinctrl.h>

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
	NIconRendering::SOptions renderedOptions;

	bool HandleCommand( unsigned nCommandID, uintptr_t ) override
	{
		if ( nCommandID == ID_ICON_RENDER_ALL )
		{
			++nRenders;
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

	WithDialog(
			[]( wxDialog *pDialog )
			{
				std::vector<wxSpinCtrl *> controls;
				FindControls( pDialog, &controls );
				ASSERT_EQ( 2u, controls.size() );
				EXPECT_EQ( 42, controls[0]->GetValue() );
				EXPECT_EQ( 42, controls[1]->GetValue() );
				std::vector<wxCheckBox *> checks;
				FindControls( pDialog, &checks );
				ASSERT_EQ( 1u, checks.size() );
				EXPECT_EQ( "Save a separate .pak", checks[0]->GetLabel().ToStdString() );
				EXPECT_TRUE( checks[0]->GetValue() );
				checks[0]->SetValue( false );
				controls[0]->SetValue( 21 );
				controls[1]->SetValue( 84 );
			} );
	EXPECT_EQ( 42, NIconRendering::LoadOptions().nWidth );
	EXPECT_EQ( 42, NIconRendering::LoadOptions().nHeight );
	EXPECT_TRUE( NIconRendering::LoadOptions().bSeparatePak );

	WithDialog(
			[]( wxDialog *pDialog )
			{
				std::vector<wxSpinCtrl *> controls;
				FindControls( pDialog, &controls );
				ASSERT_EQ( 2u, controls.size() );
				controls[0]->SetValue( 84 );
				controls[1]->SetValue( 126 );
				std::vector<wxCheckBox *> checks;
				FindControls( pDialog, &checks );
				ASSERT_EQ( 1u, checks.size() );
				Click( pDialog, wxID_APPLY );
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
				ASSERT_EQ( 1u, checks.size() );
				EXPECT_FALSE( checks[0]->GetValue() );
				checks[0]->SetValue( true );
				Click( pDialog, wxID_APPLY );
				std::vector<wxListBox *> tabs;
				FindControls( pDialog, &tabs );
				ASSERT_EQ( 1u, tabs.size() );
				EXPECT_EQ( "Icon Rendering", tabs[0]->GetStringSelection().ToStdString() );
			} );
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
