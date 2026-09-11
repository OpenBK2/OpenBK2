#include "stdafx.h"

#include "ModelView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include <fmt/format.h>

#include "ModelState.h"
#include "ResourceDefines.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/WxColourDialog.h"
#include "MapEditorLib/WxEditParameter.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

#include <cstdio>
#include <string>

// The model palette, in wx: the twelfth palette, the last of the five that
// speak edit parameters, and the first that is not a tab.
//
// It fills the model editor's docking pane: the preview light, the background
// colour and FOV of the viewport, a terrain to stand the model on and how to
// draw it, a ring or a line of copies of the model playing its animations, and
// whether to draw its AI geometry. Every control reports one flag to
// CModelState, which reads that field back out of the palette.
//
// It answers the edit-parameter pair and nothing else, so CModelCommands is
// inherited whole and there is no HandleCommand here. Everything below is the
// controls and what they report.
//
// Three things carried over that are worth knowing:
//
//   * **The four edit boxes are debounced**, as the script camera palette's
//     are: typing starts a 500 ms timer that each keystroke restarts, and the
//     state hears about the value once the typing stops.
//
//   * **The "..." colour buttons use the editor's shared colour picker**,
//     NWxColourDialog::Pick, with the user's saved custom colours and modal
//     over the frame. The original also tests `GetStyle() & ES_READONLY` on
//     this dialog before using the colour -- an edit control's style bit asked
//     of a dialog, copied from somewhere it meant something, and never set on
//     IDD_TAB_MODEL_TOOL, so the test always passes. It is not reproduced.
//
//   * **Speed's - and + buttons are enabled from the struct the state sent**,
//     not from the combo box, and the + test compares an int with a size_t the
//     way the original does: an index of -1 becomes enormous and disables +.
//
// Radio groups: Circle and Line are one group and Transparent Red and Solid
// Blue another, with combo boxes between them in creation order. wx walks past
// siblings that are not radio buttons when it looks for a group's members, and
// a group ends only at the next wxRB_GROUP, so the combo boxes do not split
// them.

namespace
{
	typedef CModelState::SEditParameters SEditParams;

	// GetSceneColorTimerInterval() and its three siblings, which the original
	// spells as member functions returning the same literal.
	const int EDIT_DEBOUNCE = 500;

	// The template's label column: every value control starts 60 dialog units
	// in. Converted with the palette's own font, so it scales the way the
	// template did.
	const int LABEL_COLUMN_DLU = 60;


	class CModelWxWindow : public CWxHostWindow, public CModelCommands
	{
		// One per debounced edit box. Carries the flag its box reports; a
		// one-shot wxTimer with a Notify() does not point at any wx window, so
		// it cannot outlive one.
		class CDebounceTimer : public wxTimer
		{
			CModelWxWindow *pOwner;
			unsigned nFlag;

		public:
			CDebounceTimer( CModelWxWindow *_pOwner, unsigned _nFlag ) : pOwner( _pOwner ), nFlag( _nFlag ) {}
			virtual void Notify() { pOwner->Send( nFlag ); }
		};

		wxScrolledWindow *pRoot = nullptr;
		int nLabelWidth = 0;

		wxChoice *pLight = nullptr;
		wxTextCtrl *pSceneColour = nullptr;
		wxTextCtrl *pFOV = nullptr;
		wxCheckBox *pTerrain = nullptr;
		wxChoice *pTerrainSize = nullptr;
		wxTextCtrl *pTerrainColour = nullptr;
		wxTextCtrl *pTerrainOpacity = nullptr;
		wxCheckBox *pTerrainDoubleSided = nullptr;
		wxCheckBox *pTerrainGrid = nullptr;
		wxCheckBox *pAnim = nullptr;
		wxChoice *pAnimCount = nullptr;
		wxChoice *pAnimSpeed = nullptr;
		wxButton *pSpeedDown = nullptr;
		wxButton *pSpeedUp = nullptr;
		wxRadioButton *pAnimCircle = nullptr;
		wxChoice *pAnimRadius = nullptr;
		wxRadioButton *pAnimLine = nullptr;
		wxChoice *pAnimDistance = nullptr;
		wxCheckBox *pAIGeometry = nullptr;
		wxRadioButton *pAITransparent = nullptr;
		wxRadioButton *pAISolid = nullptr;

		CDebounceTimer sceneColourTimer;
		CDebounceTimer fovTimer;
		CDebounceTimer terrainColourTimer;
		CDebounceTimer terrainOpacityTimer;

		// True from construction until the controls are built, as in the
		// original, and raised again around anything that fills them.
		bool bCreateControls = true;

	public:
		CModelWxWindow()
			: sceneColourTimer( this, MODEL_EP_COLOR ),
				fovTimer( this, MODEL_EP_FOV ),
				terrainColourTimer( this, MODEL_EP_TERRAIN_COLOR ),
				terrainOpacityTimer( this, MODEL_EP_TERRAIN_COLOR_OPACITY )
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_MODEL_WINDOW, this );
		}

		virtual ~CModelWxWindow()
		{
			StopTimers();
			Singleton<ICommandHandlerContainer>()->Remove( CHID_MODEL_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			// A column of fixed rows, taller than a short pane: scrolled, as
			// every palette is, with no stretchy row to take any slack.
			pRoot = CreateScrolledRoot();
			nLabelWidth = pRoot->ConvertDialogToPixels( wxSize( LABEL_COLUMN_DLU, 0 ) ).x;

			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			// Light: [..........] [...] -- the one row whose control starts at
			// its label rather than at the label column, as in the template.
			{
				wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
				pRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Light:" ), wxSizerFlags().Centre() );
				pLight = NWx::Child<wxChoice>( pRoot, wxID_ANY );
				pRow->Add( pLight, wxSizerFlags( 1 ).Centre().Border( wxLEFT, 4 ) );
				wxButton *pLightButton = SmallButton( "..." );
				pRow->Add( pLightButton, wxSizerFlags().Centre() );
				pLightButton->Bind( wxEVT_BUTTON, &CModelWxWindow::OnLightButton, this );
				pSizer->Add( pRow, wxSizerFlags().Expand() );
			}
			pSceneColour = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			wxButton *pSceneColourButton = SmallButton( "..." );
			pSizer->Add( Row( "Background Color:", pSceneColour, pSceneColourButton ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pFOV = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			pSizer->Add( Row( "FOV (1...179):", pFOV, Label( "dg." ) ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			// IDC_MODEL_DELIMITER_0, _1 and _2 are SS_ETCHEDHORZ statics.
			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ), wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pTerrain = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Draw Terrain:" );
			pSizer->Add( pTerrain, wxSizerFlags().Expand() );
			pTerrainSize = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			pSizer->Add( Row( "Size (8...64):", pTerrainSize ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pTerrainColour = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			wxButton *pTerrainColourButton = SmallButton( "..." );
			pSizer->Add( Row( "Color:", pTerrainColour, pTerrainColourButton ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pTerrainOpacity = NWx::Child<wxTextCtrl>( pRoot, wxID_ANY );
			pSizer->Add( Row( "Alpha (0...255):", pTerrainOpacity, Label( "pt." ) ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pTerrainDoubleSided = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Double-Sided Terrain" );
			pSizer->Add( pTerrainDoubleSided, wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pTerrainGrid = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Draw Terrain grid" );
			pSizer->Add( pTerrainGrid, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ), wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pAnim = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Draw Animations:" );
			pSizer->Add( pAnim, wxSizerFlags().Expand() );
			pAnimCount = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			pSizer->Add( Row( "Count (0..64):", pAnimCount ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			{
				// Speed: [combo] [-] [+] -- the buttons share the control column
				// with the combo box, as in the template, rather than sitting in
				// a column of their own.
				pAnimSpeed = NWx::Child<wxChoice>( pRoot, wxID_ANY );
				pSpeedDown = SmallButton( "-" );
				pSpeedUp = SmallButton( "+" );
				wxBoxSizer *pSpeedControls = new wxBoxSizer( wxHORIZONTAL );
				pSpeedControls->Add( pAnimSpeed, wxSizerFlags( 1 ).Centre() );
				pSpeedControls->Add( pSpeedDown, wxSizerFlags().Centre() );
				pSpeedControls->Add( pSpeedUp, wxSizerFlags().Centre() );
				wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
				pRow->Add( ColumnLabel( "Speed (-10...+10):" ), wxSizerFlags().Centre() );
				pRow->Add( pSpeedControls, wxSizerFlags( 1 ).Centre() );
				pSizer->Add( pRow, wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			}
			pAnimCircle = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Circle:", wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
			pSizer->Add( pAnimCircle, wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pAnimRadius = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			pSizer->Add( Row( "Radius (2...32):", pAnimRadius ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );
			pAnimLine = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Line:" );
			pSizer->Add( pAnimLine, wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pAnimDistance = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			pSizer->Add( Row( "Distance (2...16):", pAnimDistance ), wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ), wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pAIGeometry = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Draw AI Geometry:" );
			pSizer->Add( pAIGeometry, wxSizerFlags().Expand() );
			pAITransparent = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Transparent Red", wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
			pSizer->Add( pAITransparent, wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pAISolid = NWx::Child<wxRadioButton>( pRoot, wxID_ANY, "Solid Blue" );
			pSizer->Add( pAISolid, wxSizerFlags().Expand().Border( wxTOP, 2 ) );

			pRoot->SetSizer( pSizer );
			pRoot->FitInside();

			// The message map's entries, one to one.
			pSceneColourButton->Bind( wxEVT_BUTTON, &CModelWxWindow::OnSceneColourButton, this );
			pTerrainColourButton->Bind( wxEVT_BUTTON, &CModelWxWindow::OnTerrainColourButton, this );
			pSpeedDown->Bind( wxEVT_BUTTON, &CModelWxWindow::OnSpeedDown, this );
			pSpeedUp->Bind( wxEVT_BUTTON, &CModelWxWindow::OnSpeedUp, this );
			BindReport( pLight, MODEL_EP_LIGHT_INDEX );
			BindReport( pTerrainSize, MODEL_EP_TERRAIN_SIZE_INDEX );
			BindReport( pAnimCount, MODEL_EP_ANIM_COUNT_INDEX );
			BindReport( pAnimSpeed, MODEL_EP_ANIM_SPEED_INDEX );
			BindReport( pAnimRadius, MODEL_EP_ANIM_RADIUS_INDEX );
			BindReport( pAnimDistance, MODEL_EP_ANIM_DISTANCE_INDEX );
			BindReport( pTerrain, MODEL_EP_TERRAIN );
			BindReport( pTerrainDoubleSided, MODEL_EP_TERRAIN_DOUBLESIDED );
			BindReport( pTerrainGrid, MODEL_EP_TERRAIN_GRID );
			BindReport( pAnim, MODEL_EP_ANIM );
			BindReport( pAnimCircle, MODEL_EP_ANIM_TYPE );
			BindReport( pAnimLine, MODEL_EP_ANIM_TYPE );
			BindReport( pAIGeometry, MODEL_EP_AI_GEOMETRY );
			BindReport( pAITransparent, MODEL_EP_AI_GEOMETRY_TYPE );
			BindReport( pAISolid, MODEL_EP_AI_GEOMETRY_TYPE );
			BindDebounce( pSceneColour, sceneColourTimer );
			BindDebounce( pFOV, fovTimer );
			BindDebounce( pTerrainColour, terrainColourTimer );
			BindDebounce( pTerrainOpacity, terrainOpacityTimer );

			bCreateControls = false;
			return true;
		}

		//	CModelCommands
		virtual bool GetEditParameters( SEditParams *pEditParameters )
		{
			if ( pEditParameters == 0 )
			{
				return false;
			}
			const unsigned nFlags = pEditParameters->nFlags;
			ReadChoice( *pLight, nFlags, MODEL_EP_LIGHT_COUNT, MODEL_EP_LIGHT_INDEX,
									&( pEditParameters->lightList ), &( pEditParameters->nLightIndex ) );
			ReadChoice( *pTerrainSize, nFlags, MODEL_EP_TERRAIN_SIZE_COUNT, MODEL_EP_TERRAIN_SIZE_INDEX,
									&( pEditParameters->terrainSizeList ), &( pEditParameters->nTerrainSizeIndex ) );
			ReadChoice( *pAnimCount, nFlags, MODEL_EP_ANIM_COUNT_COUNT, MODEL_EP_ANIM_COUNT_INDEX,
									&( pEditParameters->animCountList ), &( pEditParameters->nAnimCountIndex ) );
			ReadChoice( *pAnimSpeed, nFlags, MODEL_EP_ANIM_SPEED_COUNT, MODEL_EP_ANIM_SPEED_INDEX,
									&( pEditParameters->animSpeedList ), &( pEditParameters->nAnimSpeedIndex ) );
			ReadChoice( *pAnimRadius, nFlags, MODEL_EP_ANIM_RADIUS_COUNT, MODEL_EP_ANIM_RADIUS_INDEX,
									&( pEditParameters->animRadiusList ), &( pEditParameters->nAnimRadiusIndex ) );
			ReadChoice( *pAnimDistance, nFlags, MODEL_EP_ANIM_DISTANCE_COUNT, MODEL_EP_ANIM_DISTANCE_INDEX,
									&( pEditParameters->animDistanceList ), &( pEditParameters->nAnimDistanceIndex ) );
			//
			if ( nFlags & MODEL_EP_TERRAIN )
			{
				pEditParameters->bTerrain = pTerrain->GetValue();
			}
			if ( nFlags & MODEL_EP_TERRAIN_DOUBLESIDED )
			{
				pEditParameters->bTerrainDoubleSided = pTerrainDoubleSided->GetValue();
			}
			if ( nFlags & MODEL_EP_TERRAIN_GRID )
			{
				pEditParameters->bTerrainGrid = pTerrainGrid->GetValue();
			}
			if ( nFlags & MODEL_EP_ANIM )
			{
				pEditParameters->bAnim = pAnim->GetValue();
			}
			if ( nFlags & MODEL_EP_ANIM_TYPE )
			{
				// GetCheckedRadioButton's arithmetic gave a large negative with
				// neither down; wx always has one down, the first to start with.
				pEditParameters->eAnimType = pAnimLine->GetValue() ? SEditParams::AT_LINE : SEditParams::AT_CIRCLE;
			}
			if ( nFlags & MODEL_EP_AI_GEOMETRY )
			{
				pEditParameters->bAIGeometry = pAIGeometry->GetValue();
			}
			if ( nFlags & MODEL_EP_AI_GEOMETRY_TYPE )
			{
				pEditParameters->eAIGeometryType = pAISolid->GetValue() ? SEditParams::AIGT_SOLID : SEditParams::AIGT_TRANSPARENT;
			}
			//
			// The edit boxes: parsed, and written into the struct only if they
			// hold a value in range. The caller's value is left otherwise.
			if ( nFlags & MODEL_EP_FOV )
			{
				int nFOV = pEditParameters->nFOV;
				if ( ( sscanf( Text( pFOV ).c_str(), "%d", &nFOV ) == 1 ) && ( nFOV > 0 ) && ( nFOV <= 179 ) )
				{
					pEditParameters->nFOV = nFOV;
				}
			}
			if ( nFlags & MODEL_EP_TERRAIN_COLOR_OPACITY )
			{
				int nOpacity = pEditParameters->nTerrainColorOpacity;
				if ( ( sscanf( Text( pTerrainOpacity ).c_str(), "%d", &nOpacity ) == 1 ) && ( nOpacity >= 0 ) && ( nOpacity < 256 ) )
				{
					pEditParameters->nTerrainColorOpacity = nOpacity;
				}
			}
			if ( nFlags & MODEL_EP_COLOR )
			{
				ReadColour( pSceneColour, &( pEditParameters->vColor ) );
			}
			if ( nFlags & MODEL_EP_TERRAIN_COLOR )
			{
				ReadColour( pTerrainColour, &( pEditParameters->vTerrainColor ) );
			}
			return true;
		}

		virtual bool SetEditParameters( const SEditParams &rEditParameters )
		{
			bCreateControls = true;
			const unsigned nFlags = rEditParameters.nFlags;
			WriteChoice( pLight, nFlags, MODEL_EP_LIGHT_COUNT, MODEL_EP_LIGHT_INDEX,
									 rEditParameters.lightList, rEditParameters.nLightIndex );
			WriteChoice( pTerrainSize, nFlags, MODEL_EP_TERRAIN_SIZE_COUNT, MODEL_EP_TERRAIN_SIZE_INDEX,
									 rEditParameters.terrainSizeList, rEditParameters.nTerrainSizeIndex );
			WriteChoice( pAnimCount, nFlags, MODEL_EP_ANIM_COUNT_COUNT, MODEL_EP_ANIM_COUNT_INDEX,
									 rEditParameters.animCountList, rEditParameters.nAnimCountIndex );
			WriteChoice( pAnimSpeed, nFlags, MODEL_EP_ANIM_SPEED_COUNT, MODEL_EP_ANIM_SPEED_INDEX,
									 rEditParameters.animSpeedList, rEditParameters.nAnimSpeedIndex );
			WriteChoice( pAnimRadius, nFlags, MODEL_EP_ANIM_RADIUS_COUNT, MODEL_EP_ANIM_RADIUS_INDEX,
									 rEditParameters.animRadiusList, rEditParameters.nAnimRadiusIndex );
			WriteChoice( pAnimDistance, nFlags, MODEL_EP_ANIM_DISTANCE_COUNT, MODEL_EP_ANIM_DISTANCE_INDEX,
									 rEditParameters.animDistanceList, rEditParameters.nAnimDistanceIndex );
			//
			if ( nFlags & MODEL_EP_TERRAIN )
			{
				pTerrain->SetValue( rEditParameters.bTerrain );
			}
			if ( nFlags & MODEL_EP_TERRAIN_DOUBLESIDED )
			{
				pTerrainDoubleSided->SetValue( rEditParameters.bTerrainDoubleSided );
			}
			if ( nFlags & MODEL_EP_TERRAIN_GRID )
			{
				pTerrainGrid->SetValue( rEditParameters.bTerrainGrid );
			}
			if ( nFlags & MODEL_EP_ANIM )
			{
				pAnim->SetValue( rEditParameters.bAnim );
			}
			if ( nFlags & MODEL_EP_ANIM_TYPE )
			{
				( ( rEditParameters.eAnimType == SEditParams::AT_LINE ) ? pAnimLine : pAnimCircle )->SetValue( true );
			}
			if ( nFlags & MODEL_EP_AI_GEOMETRY )
			{
				pAIGeometry->SetValue( rEditParameters.bAIGeometry );
			}
			if ( nFlags & MODEL_EP_AI_GEOMETRY_TYPE )
			{
				( ( rEditParameters.eAIGeometryType == SEditParams::AIGT_SOLID ) ? pAISolid : pAITransparent )->SetValue( true );
			}
			//
			// SetValue raises wxEVT_TEXT as SetDlgItemText raises EN_CHANGE, and
			// bCreateControls keeps it from starting a debounce, as there.
			if ( nFlags & MODEL_EP_FOV )
			{
				pFOV->SetValue( std::to_string( rEditParameters.nFOV ) );
			}
			if ( nFlags & MODEL_EP_TERRAIN_COLOR_OPACITY )
			{
				pTerrainOpacity->SetValue( std::to_string( rEditParameters.nTerrainColorOpacity ) );
			}
			if ( nFlags & MODEL_EP_COLOR )
			{
				pSceneColour->SetValue( ColourText( rEditParameters.vColor ) );
			}
			if ( nFlags & MODEL_EP_TERRAIN_COLOR )
			{
				pTerrainColour->SetValue( ColourText( rEditParameters.vTerrainColor ) );
			}
			bCreateControls = false;
			UpdateControls( rEditParameters );
			return true;
		}

		// Unguarded, as the original's timer handlers and colour buttons are:
		// by the time either gets here the question has already been asked.
		void Send( unsigned nFlag )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_GET_EDIT_PARAMETERS, nFlag );
		}

	protected:
		virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
		{
			if ( message == WM_DESTROY )
			{
				// Before the wx side comes down: a debounce firing after the
				// palette is gone would ask the state to read a box that no
				// longer exists.
				StopTimers();
			}
			return CWxHostWindow::WindowProc( message, wParam, lParam );
		}

	private:
		// ------------------------------------------------------------------
		// building
		// ------------------------------------------------------------------

		wxStaticText* Label( const char *pszText )
		{
			return NWx::Child<wxStaticText>( pRoot, wxID_ANY, pszText );
		}

		// A label as wide as the template's label column, so the controls
		// after it line up.
		wxStaticText* ColumnLabel( const char *pszText )
		{
			wxStaticText *pLabel = Label( pszText );
			pLabel->SetMinSize( wxSize( nLabelWidth, -1 ) );
			return pLabel;
		}

		// The template's 18 by 12 buttons: "...", "-" and "+". wxBU_EXACTFIT
		// alone sizes them to their one or three characters -- 12 to 16 pixels
		// wide where the template's are 36, which is a small thing to hit -- so
		// the template's width is their minimum.
		wxButton* SmallButton( const char *pszText )
		{
			wxButton *pButton = NWx::Child<wxButton>( pRoot, wxID_ANY, pszText, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
			pButton->SetMinSize( wxSize( pRoot->ConvertDialogToPixels( wxSize( 18, 0 ) ).x, -1 ) );
			return pButton;
		}

		// Label, control, and something after it or nothing. A control with
		// nothing after it runs to the right edge, as the template's do.
		wxSizer* Row( const char *pszLabel, wxWindow *pControl, wxWindow *pTrailing = nullptr )
		{
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( ColumnLabel( pszLabel ), wxSizerFlags().Centre() );
			pRow->Add( pControl, wxSizerFlags( 1 ).Centre() );
			if ( pTrailing != nullptr )
			{
				pRow->Add( pTrailing, wxSizerFlags().Centre().Border( wxLEFT, 2 ) );
			}
			return pRow;
		}

		// A control whose change is one flag reported to the state.
		void BindReport( wxChoice *pChoice, unsigned nFlag )
		{
			pChoice->Bind( wxEVT_CHOICE, [this, nFlag]( wxCommandEvent& ) { Report( nFlag ); } );
		}

		void BindReport( wxCheckBox *pCheck, unsigned nFlag )
		{
			pCheck->Bind( wxEVT_CHECKBOX, [this, nFlag]( wxCommandEvent& ) { Report( nFlag ); } );
		}

		void BindReport( wxRadioButton *pRadio, unsigned nFlag )
		{
			pRadio->Bind( wxEVT_RADIOBUTTON, [this, nFlag]( wxCommandEvent& ) { Report( nFlag ); } );
		}

		// An edit box whose typing restarts its debounce.
		void BindDebounce( wxTextCtrl *pEdit, CDebounceTimer &rTimer )
		{
			CDebounceTimer *pTimer = &rTimer;
			pEdit->Bind( wxEVT_TEXT, [this, pTimer]( wxCommandEvent& )
			{
				if ( !bCreateControls )
				{
					pTimer->StartOnce( EDIT_DEBOUNCE );
				}
			} );
		}

		void StopTimers()
		{
			sceneColourTimer.Stop();
			fovTimer.Stop();
			terrainColourTimer.Stop();
			terrainOpacityTimer.Stop();
		}

		// ------------------------------------------------------------------
		// reading and writing
		// ------------------------------------------------------------------

		static std::string Text( const wxTextCtrl *pEdit )
		{
			return std::string( pEdit->GetValue().utf8_str() );
		}

		// The shape of every combo box exchange: a COUNT flag for the list and
		// an INDEX flag for the selection, handed to the shared helpers.
		template <class TList>
		static void ReadChoice( const wxChoice &rChoice, unsigned nFlags, unsigned nCountFlag, unsigned nIndexFlag,
														TList *pList, int *pIndex )
		{
			if ( nFlags & ( nCountFlag | nIndexFlag ) )
			{
				NWxEditParameter::ReadChoice( rChoice, pList, pIndex, ( nFlags & nCountFlag ) != 0, ( nFlags & nIndexFlag ) != 0 );
			}
		}

		template <class TList>
		static void WriteChoice( wxChoice *pChoice, unsigned nFlags, unsigned nCountFlag, unsigned nIndexFlag,
														 const TList &rList, int nIndex )
		{
			if ( nFlags & ( nCountFlag | nIndexFlag ) )
			{
				NWxEditParameter::WriteChoice( pChoice, rList, nIndex, ( nFlags & nCountFlag ) != 0, ( nFlags & nIndexFlag ) != 0 );
			}
		}

		// "r, g, b", which is what the palette writes and the state reads.
		static std::string ColourText( const CVec3 &rColour )
		{
			return fmt::format( "{}, {}, {}", (int)( rColour.r ), (int)( rColour.g ), (int)( rColour.b ) );
		}

		// All three components parsed and each in 0..255, or the colour is
		// left as the caller passed it.
		static void ReadColour( const wxTextCtrl *pEdit, CVec3 *pColour )
		{
			int r = 0;
			int g = 0;
			int b = 0;
			if ( ( sscanf( Text( pEdit ).c_str(), "%d,%d,%d", &r, &g, &b ) == 3 ) &&
					 ( ( r >= 0 ) && ( r < 256 ) ) &&
					 ( ( g >= 0 ) && ( g < 256 ) ) &&
					 ( ( b >= 0 ) && ( b < 256 ) ) )
			{
				pColour->r = r;
				pColour->g = g;
				pColour->b = b;
			}
		}

		void UpdateControls( const SEditParams &rEditParameters )
		{
			pSpeedDown->Enable( rEditParameters.nAnimSpeedIndex > 0 );
			// The original's int < size_t comparison, spelled out: an index of
			// -1 converts to the largest size_t and + is disabled, and an empty
			// list makes size() - 1 the largest size_t and + is enabled.
			pSpeedUp->Enable( static_cast<size_t>( rEditParameters.nAnimSpeedIndex ) < ( rEditParameters.animSpeedList.size() - 1 ) );
		}

		// ------------------------------------------------------------------
		// events
		// ------------------------------------------------------------------

		void Report( unsigned nFlag )
		{
			if ( !bCreateControls )
			{
				Send( nFlag );
			}
		}

		void OnLightButton( wxCommandEvent& )
		{
			// Unguarded in the original too.
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_MODEL_SET_LIGHT, 0 );
		}

		void OnSpeedDown( wxCommandEvent& )
		{
			if ( !bCreateControls )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_MODEL_SPEED_DOWN, 0 );
			}
		}

		void OnSpeedUp( wxCommandEvent& )
		{
			if ( !bCreateControls )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MODEL_STATE, ID_MODEL_SPEED_UP, 0 );
			}
		}

		void OnSceneColourButton( wxCommandEvent& ) { PickColour( pSceneColour, MODEL_EP_COLOR ); }
		void OnTerrainColourButton( wxCommandEvent& ) { PickColour( pTerrainColour, MODEL_EP_TERRAIN_COLOR ); }

		// The "..." buttons. The box's colour, or mid grey where it does not
		// parse, is where the picker starts; a colour picked goes into the box
		// without starting its debounce and is reported at once.
		void PickColour( wxTextCtrl *pEdit, unsigned nFlag )
		{
			// The whole of the original is inside `if ( pUserData )`, the
			// scene input reset included; Pick asks the same question.
			if ( Singleton<IUserDataContainer>()->Get() == 0 )
			{
				return;
			}
			int r = 128;
			int g = 128;
			int b = 128;
			sscanf( Text( pEdit ).c_str(), "%d,%d,%d", &r, &g, &b );
			wxColour chosen;
			if ( NWxColourDialog::Pick( pRoot, this, wxColour( (unsigned char)r, (unsigned char)g, (unsigned char)b ), &chosen ) )
			{
				bCreateControls = true;
				pEdit->SetValue( fmt::format( "{}, {}, {}", (int)chosen.Red(), (int)chosen.Green(), (int)chosen.Blue() ) );
				bCreateControls = false;
				Send( nFlag );
			}
			// The scene captured the mouse before the dialog did.
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
	};
}


namespace NModelView
{
	std::unique_ptr<CWnd> CreateWx( CWnd *pParent )
	{
		std::unique_ptr<CModelWxWindow> pWindow( new CModelWxWindow() );
		if ( !pWindow->Build( pParent ) )
		{
			return nullptr;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
