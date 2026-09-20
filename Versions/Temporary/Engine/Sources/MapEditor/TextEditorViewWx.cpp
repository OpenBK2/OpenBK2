#include "stdafx.h"

#include "TextEditorView.h"


#include "MapEditorLib/SimulatedKey.h"
#include "MapEditorLib/WxWidget.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPlacement.h"
#include "MapEditorLib/WxToolDialog.h"
#include "port/vkcodes.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>

#include <cctype>

// The text editors in wx, on wxStyledTextCtrl.
//
// wxSTC is the Scintilla the wx build carries -- 5.x, with Lexilla -- where the
// MFC editors use the 2005 one this tree vendors. The message set is the same,
// which is why most of what follows reads as a transliteration of CLuaEditor.
// One thing is not the same and changes the order things happen in:
//
//   **Keyword lists belong to the lexer now.** The old Scintilla kept them in
//   the control, so CLuaEditor could hand over its keyword sets first and pick
//   the Lua lexer afterwards. Lexilla keeps them inside the lexer instance, and
//   choosing a lexer makes a new one. Done in the old order every keyword colour
//   is lost, silently. So the lexer comes first here, then the words.
//
// The rest of what the script editor does that is not drawing -- the Lua syntax
// check behind the error box, the keyword lists and the colours -- is shared
// with the MFC editor through TextEditorView.h.
//
// Two things the MFC script editor does are deliberately not here, because they
// do nothing there either: WM_ME_TEXTCHANGED is posted to a parent that never
// handles it, and the call tip on mouse-over is behind an `if ( false )`.

namespace
{
	wxColour FromColorRef( uint32_t nColor )
	{
		return wxColour( nColor & 0xFF, ( nColor >> 8 ) & 0xFF, ( nColor >> 16 ) & 0xFF );
	}


	std::string ToUtf8( const wxString &rText )
	{
		return std::string( rText.utf8_str() );
	}


	// The narrow text the callers hand over is the process code page, which is
	// UTF-8 in this build. A file written in some older code page is not valid
	// UTF-8, and FromUTF8 answers an empty string for it -- which OK would then
	// hand back as the file's new contents. So anything that is not UTF-8 is
	// read as the local code page instead of being dropped.
	wxString FromNarrow( const std::string &rszText )
	{
		const wxString text = wxString::FromUTF8( rszText.c_str(), rszText.size() );
		if ( text.empty() && !rszText.empty() )
		{
			return wxString( rszText.c_str(), wxConvLocal, rszText.size() );
		}
		return text;
	}


	// What Find and Replace ask the editor for -- CFindNext's three calls.
	class IFindTarget
	{
	public:
		virtual ~IFindTarget() {}
		virtual void FindNext( const std::string &rszText, bool bWholeWord, bool bMatchCase ) = 0;
		virtual void ReplaceSelection( const std::string &rszWith ) = 0;
		virtual void ReplaceAll( const std::string &rszText, const std::string &rszWith,
														 bool bWholeWord, bool bMatchCase ) = 0;
	};


	enum
	{
		ID_FIND_NEXT = wxID_HIGHEST + 1,
		ID_REPLACE,
		ID_REPLACE_ALL,
	};


	// "Find", IDD_FINDTEXT. Modeless and owned by the script editor. Cancel and
	// the close box hide it, as CFindTextDlg::OnCancel does, so what was typed is
	// still there next time.
	class CFindWxDialog : public wxDialog
	{
		wxTextCtrl *pText = nullptr;
		wxCheckBox *pWholeWord = nullptr;
		wxCheckBox *pMatchCase = nullptr;
		IFindTarget *pTarget = nullptr;

	public:
		CFindWxDialog( wxWindow *pParent, IFindTarget *_pTarget )
			: wxDialog( pParent, wxID_ANY, "Find" ), pTarget( _pTarget )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxHORIZONTAL );
			wxBoxSizer *pLeft = new wxBoxSizer( wxVERTICAL );
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Find what:" ),
								 wxSizerFlags().CentreVertical().Border( wxRIGHT, 6 ) );
			pText = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(), wxDefaultPosition,
																			wxSize( ConvertDialogToPixels( wxSize( 108, 0 ) ).x, -1 ) );
			pRow->Add( pText, wxSizerFlags( 1 ).CentreVertical() );
			pLeft->Add( pRow, wxSizerFlags().Expand() );
			pWholeWord = NWx::Child<wxCheckBox>( this, wxID_ANY, "Match whole word only" );
			pMatchCase = NWx::Child<wxCheckBox>( this, wxID_ANY, "Match case" );
			pLeft->Add( pWholeWord, wxSizerFlags().Border( wxTOP, 6 ) );
			pLeft->Add( pMatchCase, wxSizerFlags().Border( wxTOP, 4 ) );
			pSizer->Add( pLeft, wxSizerFlags( 1 ).Expand().Border( wxALL, 8 ) );

			wxBoxSizer *pButtons = new wxBoxSizer( wxVERTICAL );
			wxButton *const pFindNext = NWx::Child<wxButton>( this, ID_FIND_NEXT, "Find Next" );
			pFindNext->SetDefault();
			pButtons->Add( pFindNext, wxSizerFlags().Expand() );
			pButtons->Add( NWx::Child<wxButton>( this, wxID_CANCEL, "Cancel" ),
										 wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pSizer->Add( pButtons, wxSizerFlags().Border( wxTOP | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );

			Bind( wxEVT_BUTTON, &CFindWxDialog::OnFindNext, this, ID_FIND_NEXT );
			Bind( wxEVT_BUTTON, &CFindWxDialog::OnHide, this, wxID_CANCEL );
			Bind( wxEVT_CLOSE_WINDOW, &CFindWxDialog::OnCloseBox, this );
		}

	private:
		void OnFindNext( wxCommandEvent & )
		{
			pTarget->FindNext( ToUtf8( pText->GetValue() ), pWholeWord->GetValue(), pMatchCase->GetValue() );
		}

		void OnHide( wxCommandEvent & ) { Hide(); }
		void OnCloseBox( wxCloseEvent & ) { Hide(); }
	};


	// "Replace", IDD_REPLACETEXT. Modeless like Find, and hidden rather than
	// destroyed: CReplaceTextDlg's Close is CDialog::OnCancel, and ending a
	// modeless dialog only hides it.
	class CReplaceWxDialog : public wxDialog
	{
		wxTextCtrl *pFind = nullptr;
		wxTextCtrl *pWith = nullptr;
		wxCheckBox *pWholeWord = nullptr;
		wxCheckBox *pMatchCase = nullptr;
		IFindTarget *pTarget = nullptr;

	public:
		CReplaceWxDialog( wxWindow *pParent, IFindTarget *_pTarget )
			: wxDialog( pParent, wxID_ANY, "Replace" ), pTarget( _pTarget )
		{
			wxBoxSizer *pSizer = new wxBoxSizer( wxHORIZONTAL );
			wxBoxSizer *pLeft = new wxBoxSizer( wxVERTICAL );
			wxFlexGridSizer *pFields = new wxFlexGridSizer( 2, 4, 6 );
			pFields->AddGrowableCol( 1 );
			pFields->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Find what:" ), wxSizerFlags().CentreVertical() );
			pFind = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(), wxDefaultPosition,
																			wxSize( ConvertDialogToPixels( wxSize( 161, 0 ) ).x, -1 ) );
			pFields->Add( pFind, wxSizerFlags().Expand() );
			pFields->Add( NWx::Child<wxStaticText>( this, wxID_ANY, "Replace with:" ), wxSizerFlags().CentreVertical() );
			pWith = NWx::Child<wxTextCtrl>( this, wxID_ANY );
			pFields->Add( pWith, wxSizerFlags().Expand() );
			pLeft->Add( pFields, wxSizerFlags().Expand() );
			pWholeWord = NWx::Child<wxCheckBox>( this, wxID_ANY, "Match whole word only" );
			pMatchCase = NWx::Child<wxCheckBox>( this, wxID_ANY, "Match case" );
			pLeft->Add( pWholeWord, wxSizerFlags().Border( wxTOP, 6 ) );
			pLeft->Add( pMatchCase, wxSizerFlags().Border( wxTOP, 4 ) );
			pSizer->Add( pLeft, wxSizerFlags( 1 ).Expand().Border( wxALL, 8 ) );

			wxBoxSizer *pButtons = new wxBoxSizer( wxVERTICAL );
			wxButton *const pFindNext = NWx::Child<wxButton>( this, ID_FIND_NEXT, "Find Next" );
			pFindNext->SetDefault();
			pButtons->Add( pFindNext, wxSizerFlags().Expand() );
			pButtons->Add( NWx::Child<wxButton>( this, ID_REPLACE, "Replace" ), wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pButtons->Add( NWx::Child<wxButton>( this, ID_REPLACE_ALL, "Replace All" ), wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pButtons->Add( NWx::Child<wxButton>( this, wxID_CANCEL, "Close" ), wxSizerFlags().Expand().Border( wxTOP, 4 ) );
			pSizer->Add( pButtons, wxSizerFlags().Border( wxTOP | wxRIGHT | wxBOTTOM, 8 ) );
			SetSizerAndFit( pSizer );

			Bind( wxEVT_BUTTON, &CReplaceWxDialog::OnFindNext, this, ID_FIND_NEXT );
			Bind( wxEVT_BUTTON, &CReplaceWxDialog::OnReplace, this, ID_REPLACE );
			Bind( wxEVT_BUTTON, &CReplaceWxDialog::OnReplaceAll, this, ID_REPLACE_ALL );
			Bind( wxEVT_BUTTON, &CReplaceWxDialog::OnHide, this, wxID_CANCEL );
			Bind( wxEVT_CLOSE_WINDOW, &CReplaceWxDialog::OnCloseBox, this );
		}

	private:
		void OnFindNext( wxCommandEvent & )
		{
			pTarget->FindNext( ToUtf8( pFind->GetValue() ), pWholeWord->GetValue(), pMatchCase->GetValue() );
		}

		// CFindNext::Replace: the selection is replaced and the search moves on.
		void OnReplace( wxCommandEvent & )
		{
			pTarget->ReplaceSelection( ToUtf8( pWith->GetValue() ) );
			pTarget->FindNext( ToUtf8( pFind->GetValue() ), pWholeWord->GetValue(), pMatchCase->GetValue() );
		}

		void OnReplaceAll( wxCommandEvent & )
		{
			pTarget->ReplaceAll( ToUtf8( pFind->GetValue() ), ToUtf8( pWith->GetValue() ),
													 pWholeWord->GetValue(), pMatchCase->GetValue() );
		}

		void OnHide( wxCommandEvent & ) { Hide(); }
		void OnCloseBox( wxCloseEvent & ) { Hide(); }
	};


	// "Script Editor", IDD_SCRIPT_EDITOR: CScriptEditor and the CLuaEditor in it.
	class CScriptEditorWxDialog : public CWxToolDialog, private IFindTarget
	{
		NWxPlacement::CSizedPlacement placement { "CScriptEditor" };
		wxStyledTextCtrl *pEditor = nullptr;
		wxTextCtrl *pErrors = nullptr;
		NTextEditor::SLuaKeywords keywords;

		// Made on first use and owned by this dialog, as CLuaEditor owns its two.
		CFindWxDialog *pFindDialog = nullptr;
		CReplaceWxDialog *pReplaceDialog = nullptr;
		std::string szLastTextToFind;
		bool bLastWholeWord = false;
		bool bLastMatchCase = false;

	public:
		CScriptEditorWxDialog( wxWindow *pParent, const std::string &rszTitle, const std::string &rszText, bool bEnableEdit )
			: CWxToolDialog( pParent, wxID_ANY, "Script Editor", wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
		{
			// The editor over a fixed-height error box, and OK and Cancel in the
			// bottom right -- what CScriptEditor::OnSize keeps them as by hand.
			//
			// The gaps are IDD_SCRIPT_EDITOR's in the pixels MFC lays it out at
			// (7 dlu margins, a 60 dlu error box, 50x14 dlu buttons). Dialog units
			// converted in wx's own font come out a sixth larger, which is why
			// they are not converted here.
			wxBoxSizer *pContent = new wxBoxSizer( wxVERTICAL );
			// The template's Scintilla has no border of its own.
			pEditor = NWx::Child<wxStyledTextCtrl>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );
			pContent->Add( pEditor, wxSizerFlags( 1 ).Expand() );
			// IDC_ERRLOG has no scroll bars. It does not wrap either, and that wx
			// cannot ask for without a horizontal bar -- wxTE_DONTWRAP is
			// WS_HSCROLL -- so a long complaint wraps here instead.
			pErrors = NWx::Child<wxTextCtrl>( this, wxID_ANY, wxString(), wxDefaultPosition,
																				wxSize( -1, FromDIP( 98 ) ),
																				wxTE_MULTILINE | wxTE_READONLY | wxTE_NO_VSCROLL );
			pContent->Add( pErrors, wxSizerFlags().Expand().Border( wxTOP, FromDIP( 10 ) ) );
			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			pButtons->AddStretchSpacer( 1 );
			wxButton *const pOk = NWx::Child<wxButton>( this, wxID_OK, "OK" );
			pOk->SetDefault();
			pButtons->Add( pOk, wxSizerFlags().Border( wxRIGHT, FromDIP( 12 ) ) );
			pButtons->Add( NWx::Child<wxButton>( this, wxID_CANCEL, "Cancel" ) );
			pContent->Add( pButtons, wxSizerFlags().Expand().Border( wxTOP, FromDIP( 9 ) ) );
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pContent, wxSizerFlags( 1 ).Expand().Border( wxALL, FromDIP( 11 ) ) );
			SetSizer( pSizer );
			// GetMinimumXDimension and GetMinimumYDimension, and the template's
			// 434x346 dlu for a first open, as MFC sizes it.
			SetMinSize( wxSize( 400, 300 ) );
			SetClientSize( FromDIP( wxSize( 651, 562 ) ) );

			SetUpMargins();
			SetUpLua();
			SetText( rszText );
			CheckSyntax();
			pEditor->SetReadOnly( !bEnableEdit );

			placement.Restore( this );
			if ( !rszTitle.empty() )
			{
				SetTitle( wxString::FromUTF8( rszTitle.c_str() ) );
			}

			pEditor->Bind( wxEVT_STC_CHANGE, &CScriptEditorWxDialog::OnTextChanged, this );
			pEditor->Bind( wxEVT_STC_CHARADDED, &CScriptEditorWxDialog::OnCharAdded, this );
			pEditor->Bind( wxEVT_KEY_DOWN, &CScriptEditorWxDialog::OnEditorKeyDown, this );
			// The template's first control is OK, so that is where the dialog
			// manager puts the focus, and where this puts it.
			pOk->SetFocus();
		}

		bool WasPlaced() const { return placement.WasPlaced(); }
		void SaveState() { placement.Save( this ); }
		std::string GetEditorText() const { return ToUtf8( pEditor->GetText() ); }

	private:
		// CLuaEditor::InitScintilla and SetEditorMargins.
		void SetUpMargins()
		{
			pEditor->SetMarginWidth( 1, 0 );
			pEditor->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
			pEditor->SetMarginWidth( 0, 6 * pEditor->TextWidth( wxSTC_STYLE_LINENUMBER, "9" ) );
			pEditor->SetMarginType( 1, wxSTC_MARGIN_SYMBOL );
			pEditor->SetMarginWidth( 1, 10 );
			pEditor->SetMarginSensitive( 1, true );
			pEditor->MarkerDefine( 0, wxSTC_MARK_CIRCLE );
			pEditor->MarkerSetForeground( 0, wxColour( 0xff, 0x00, 0x00 ) );
			pEditor->MarkerSetBackground( 0, wxColour( 0xff, 0x00, 0x00 ) );
			pEditor->MarkerDefine( 1, wxSTC_MARK_ARROW );
			// CARET_SLOP, which is the value SCI_SETVISIBLEPOLICY's VISIBLE_SLOP has.
			pEditor->SetVisiblePolicy( 0x01, 1 );
		}

		// CLuaEditor::SetLuaLexer, the keyword sets CScriptEditor::OnInitDialog
		// handed over, and SetAutoComplete -- lexer first; see the top of the file.
		void SetUpLua()
		{
			pEditor->SetLexer( wxSTC_LEX_LUA );
			pEditor->SetKeyWords( 0, NTextEditor::LuaReservedWords() );
			NTextEditor::LoadLuaKeywords( &keywords );
			for ( std::vector<NTextEditor::SKeywordSet>::const_iterator it = keywords.sets.begin();
						it != keywords.sets.end(); ++it )
			{
				pEditor->SetKeyWords( it->nSet, wxString::FromUTF8( it->szWords.c_str() ) );
			}

			// The dictionary colours, which SetKeywordColor gave the WORD styles.
			// Before the style table, not after, because that is the order the MFC
			// editor ran them in -- CheckSyntax( true ) calls SetLuaLexer once the
			// keywords are in -- and the table's WORD2 colour lands on top of set 2's.
			for ( std::vector<NTextEditor::SKeywordSet>::const_iterator it = keywords.sets.begin();
						it != keywords.sets.end(); ++it )
			{
				if ( it->bHasColor )
				{
					pEditor->StyleSetForeground( wxSTC_LUA_WORD2 - 1 + it->nSet, FromColorRef( it->nColor ) );
				}
			}

			pEditor->SetIndentationGuides( 1 );
			pEditor->SetTabWidth( 4 );
			pEditor->SetUseTabs( true );
			pEditor->SetEOLMode( wxSTC_EOL_CRLF );

			const std::vector<NTextEditor::SLuaStyle> &rStyles = NTextEditor::LuaStyles();
			for ( std::vector<NTextEditor::SLuaStyle>::const_iterator it = rStyles.begin(); it != rStyles.end(); ++it )
			{
				if ( it->pszFace != nullptr )
				{
					pEditor->StyleSetFaceName( it->nStyle, it->pszFace );
				}
				if ( it->nSize > 0 )
				{
					pEditor->StyleSetSize( it->nStyle, it->nSize );
				}
				if ( it->nFore >= 0 )
				{
					pEditor->StyleSetForeground( it->nStyle, FromColorRef( it->nFore ) );
				}
				if ( it->nBack >= 0 )
				{
					pEditor->StyleSetBackground( it->nStyle, FromColorRef( it->nBack ) );
				}
				if ( it->bEolFilled )
				{
					pEditor->StyleSetEOLFilled( it->nStyle, true );
				}
			}
			// Lexilla styles a comment that opens with "---" as a doc comment,
			// style 3. The 2005 lexer never used style 3 and painted the same line
			// as the line comment, style 2 -- the scripts are full of "-----"
			// rulers -- so style 3 takes style 2's look and they read the same in
			// both editors.
			pEditor->StyleSetFaceName( wxSTC_LUA_COMMENTDOC, pEditor->StyleGetFaceName( wxSTC_LUA_COMMENTLINE ) );
			pEditor->StyleSetSize( wxSTC_LUA_COMMENTDOC, pEditor->StyleGetSize( wxSTC_LUA_COMMENTLINE ) );
			pEditor->StyleSetForeground( wxSTC_LUA_COMMENTDOC, pEditor->StyleGetForeground( wxSTC_LUA_COMMENTLINE ) );
			pEditor->AutoCompSetCancelAtStart( false );
			pEditor->AutoCompSetFillUps( " (" );
		}

		void SetText( const std::string &rszText )
		{
			pEditor->ClearAll();
			pEditor->AddText( FromNarrow( rszText ) );
		}

		// CScriptEditor::CheckSyntax, on every change.
		void CheckSyntax()
		{
			pErrors->ChangeValue( wxString::FromUTF8( NTextEditor::CheckLuaSyntax( GetEditorText() ).c_str() ) );
		}

		void OnTextChanged( wxStyledTextEvent &rEvent )
		{
			CheckSyntax();
			rEvent.Skip();
		}

		// CScriptEditor::OnCnCharAdded.
		void OnCharAdded( wxStyledTextEvent &rEvent )
		{
			const int nChar = rEvent.GetKey();
			if ( nChar == '\r' || nChar == '\n' )
			{
				NewLineIndent();
			}
			else if ( nChar > 0 && nChar < 128 && isalpha( nChar ) )
			{
				AutoComplete();
			}
		}

		void OnEditorKeyDown( wxKeyEvent &rEvent )
		{
			// Windows reports AltGr as Ctrl+Alt. Let it reach the text control
			// so layout-specific characters (e.g. Croatian AltGr+F = [) can be typed.
			if ( !HandleShortcut( rEvent.GetKeyCode(), rEvent.ControlDown() && !rEvent.AltDown() ) )
			{
				rEvent.Skip();
			}
		}

		// CLuaEditor::HandleShortcut: Ctrl+F finds, Ctrl+H replaces, and F3 finds
		// the last text again once there is a Find dialog to have typed it into.
		// nKey is a wx key code. True when the key was consumed; F3 is not.
		bool HandleShortcut( int nKey, bool bControl )
		{
			if ( pFindDialog != nullptr && nKey == WXK_F3 )
			{
				if ( szLastTextToFind.empty() )
				{
					OpenFind();
				}
				else
				{
					FindNext( szLastTextToFind, bLastWholeWord, bLastMatchCase );
				}
			}
			if ( bControl )
			{
				if ( nKey == 'F' )
				{
					OpenFind();
					return true;
				}
				if ( nKey == 'H' )
				{
					OpenReplace();
					return true;
				}
			}
			return false;
		}

#ifdef __WXMSW__
		// NSimulatedKey, as CScriptEditor::OnSimulatedKey handles it. The message
		// carries a Win32 virtual key; letters are the same code in wx, and the
		// function keys are moved onto wx's.
		virtual WXLRESULT MSWWindowProc( WXUINT nMessage, WXWPARAM wParam, WXLPARAM lParam )
		{
			if ( nMessage != NSimulatedKey::Message() )
			{
				return CWxToolDialog::MSWWindowProc( nMessage, wParam, lParam );
			}
			switch ( wParam )
			{
				case NSimulatedKey::OP_KEY:
				{
					const unsigned nVirtualKey = NSimulatedKey::KeyOf( lParam );
					const int nKey = ( nVirtualKey >= VK_F1 && nVirtualKey <= VK_F24 )
														 ? WXK_F1 + static_cast<int>( nVirtualKey - VK_F1 )
														 : static_cast<int>( nVirtualKey );
					HandleShortcut( nKey, NSimulatedKey::HasModifier( lParam, NSimulatedKey::MODIFIER_CONTROL ) );
					return 1;
				}
				case NSimulatedKey::OP_SELECTION_START:
					return pEditor->GetSelectionStart();
				case NSimulatedKey::OP_SELECTION_END:
					return pEditor->GetSelectionEnd();
				default:
					return 0;
			}
		}
#endif

		// CLuaEditor::AutoComplete: offer the list when the word at the caret
		// starts some keyword.
		void AutoComplete()
		{
			if ( pEditor->AutoCompActive() )
			{
				return;
			}
			const int nCurrentPos = pEditor->GetCurrentPos();
			const int nStart = pEditor->WordStartPosition( nCurrentPos, true );
			const int nEnd = pEditor->WordEndPosition( nCurrentPos, true );
			const int nCount = nEnd - nStart;
			if ( nCount <= 0 )
			{
				return;
			}
			const std::string szWord = ToUtf8( pEditor->GetTextRange( nStart, nEnd ) );
			for ( std::vector<std::string>::const_iterator it = keywords.completionWords.begin();
						it != keywords.completionWords.end(); ++it )
			{
				if ( it->compare( 0, szWord.size(), szWord ) == 0 )
				{
					pEditor->AutoCompShow( nCount, wxString::FromUTF8( keywords.szCompletionList.c_str() ) );
					break;
				}
			}
		}

		// CLuaEditor::NewLineIndent: a new, still empty line takes the leading
		// blanks of the line above.
		void NewLineIndent()
		{
			const int nCurrentLine = pEditor->LineFromPosition( pEditor->GetCurrentPos() );
			if ( nCurrentLine <= 0 || pEditor->GetLineLength( nCurrentLine ) > 2 )
			{
				return;
			}
			// The MFC editor's line buffer was 1000 bytes and a longer line above
			// was left alone.
			if ( pEditor->GetLineLength( nCurrentLine - 1 ) >= 1000 )
			{
				return;
			}
			const std::string szPrevious = ToUtf8( pEditor->GetLine( nCurrentLine - 1 ) );
			size_t nIndent = 0;
			while ( nIndent < szPrevious.size() && ( szPrevious[nIndent] == ' ' || szPrevious[nIndent] == '\t' ) )
			{
				++nIndent;
			}
			pEditor->SetSelection( pEditor->PositionFromLine( nCurrentLine ),
														 pEditor->GetLineEndPosition( nCurrentLine ) );
			pEditor->ReplaceSelection( wxString::FromUTF8( szPrevious.substr( 0, nIndent ).c_str() ) );
		}

		void OpenFind()
		{
			if ( pFindDialog == nullptr )
			{
				pFindDialog = NWx::Child<CFindWxDialog>( this, static_cast<IFindTarget*>( this ) );
			}
			pFindDialog->Show();
			pFindDialog->Raise();
		}

		void OpenReplace()
		{
			if ( pReplaceDialog == nullptr )
			{
				pReplaceDialog = NWx::Child<CReplaceWxDialog>( this, static_cast<IFindTarget*>( this ) );
			}
			pReplaceDialog->Show();
			pReplaceDialog->Raise();
		}

		// IFindTarget -- CLuaEditor::FindNext, Replace and ReplaceAll. Positions
		// are bytes, as Scintilla's are, so lengths are taken in UTF-8.
		virtual void FindNext( const std::string &rszText, bool bWholeWord, bool bMatchCase )
		{
			szLastTextToFind = rszText;
			bLastMatchCase = bMatchCase;
			bLastWholeWord = bWholeWord;
			if ( rszText.empty() )
			{
				return;
			}
			const int nFlags = ( bMatchCase ? wxSTC_FIND_MATCHCASE : 0 ) | ( bWholeWord ? wxSTC_FIND_WHOLEWORD : 0 );
			const wxString text = wxString::FromUTF8( rszText.c_str() );
			const int nCurrentPos = pEditor->GetCurrentPos();
			pEditor->SetTargetStart( nCurrentPos );
			pEditor->SetTargetEnd( pEditor->GetTextLength() );
			pEditor->SetSearchFlags( nFlags );
			int nPos = pEditor->SearchInTarget( text );
			// Not found after the caret: once more from the top.
			if ( nPos == -1 && nCurrentPos > 0 )
			{
				pEditor->SetTargetStart( 0 );
				nPos = pEditor->SearchInTarget( text );
			}
			if ( nPos >= 0 )
			{
				pEditor->SetSelection( nPos, nPos + static_cast<int>( rszText.size() ) );
			}
		}

		virtual void ReplaceSelection( const std::string &rszWith )
		{
			if ( pEditor->GetSelectionStart() == pEditor->GetSelectionEnd() )
			{
				return;
			}
			pEditor->ReplaceSelection( wxString::FromUTF8( rszWith.c_str() ) );
		}

		virtual void ReplaceAll( const std::string &rszText, const std::string &rszWith,
														 bool bWholeWord, bool bMatchCase )
		{
			szLastTextToFind = rszText;
			bLastMatchCase = bMatchCase;
			bLastWholeWord = bWholeWord;
			if ( rszText.empty() )
			{
				return;
			}
			const int nFlags = ( bMatchCase ? wxSTC_FIND_MATCHCASE : 0 ) | ( bWholeWord ? wxSTC_FIND_WHOLEWORD : 0 );
			const wxString text = wxString::FromUTF8( rszText.c_str() );
			const wxString with = wxString::FromUTF8( rszWith.c_str() );
			pEditor->SetTargetStart( 0 );
			pEditor->SetTargetEnd( pEditor->GetTextLength() );
			pEditor->SetSearchFlags( nFlags );
			int nPos = -1;
			while ( ( nPos = pEditor->SearchInTarget( text ) ) >= 0 )
			{
				pEditor->SetTargetStart( nPos );
				pEditor->SetTargetEnd( nPos + static_cast<int>( rszText.size() ) );
				pEditor->ReplaceTarget( with );
				pEditor->SetTargetStart( nPos + static_cast<int>( rszWith.size() ) );
				pEditor->SetTargetEnd( pEditor->GetTextLength() );
				pEditor->SetSearchFlags( nFlags );
			}
		}
	};


	// "Text Editor", IDD_TEXT_EDITOR: CTextEditorDialog and the
	// CScintillaEditorWindow in it.
	class CTextEditorWxDialog : public CWxToolDialog
	{
		NWxPlacement::CSizedPlacement placement;
		wxStyledTextCtrl *pEditor = nullptr;

	public:
		// CTextEditorDialog::GetXMLFilePath: the prefix and the editor type's
		// mnemonic, which is "lua" for Lua and empty for everything else.
		static std::string PlacementName( const std::string &rszEditor )
		{
			return std::string( "CTextEditorDialog" ) + ( ( rszEditor == "lua" ) ? "lua" : "" );
		}

		CTextEditorWxDialog( wxWindow *pParent, const std::string &rszTitle, const std::string &rszEditor,
												 const std::string &rszText, bool bEnableEdit )
			: CWxToolDialog( pParent, wxID_ANY, "Text Editor", wxDefaultPosition, wxDefaultSize,
											 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
				placement( PlacementName( rszEditor ) )
		{
			// IDD_TEXT_EDITOR's gaps in the pixels MFC lays it out at, for the
			// reason the script editor gives.
			wxBoxSizer *pContent = new wxBoxSizer( wxVERTICAL );
			pEditor = NWx::Child<wxStyledTextCtrl>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN );
			pContent->Add( pEditor, wxSizerFlags( 1 ).Expand() );
			// IDC_TE_STATUSBAR. CScintillaEditorWindow::UpdateStatusStringWindow has
			// an empty body, so it has only ever been an empty sunken line.
			pContent->Add( NWx::Child<wxStaticText>( this, wxID_ANY, wxString(), wxDefaultPosition,
																							 wxSize( -1, FromDIP( 16 ) ),
																							 wxST_NO_AUTORESIZE | wxBORDER_SUNKEN ),
										 wxSizerFlags().Expand().Border( wxTOP, FromDIP( 9 ) ) );
			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			pButtons->AddStretchSpacer( 1 );
			wxButton *const pOk = NWx::Child<wxButton>( this, wxID_OK, "OK" );
			pOk->SetDefault();
			pButtons->Add( pOk, wxSizerFlags().Border( wxRIGHT, FromDIP( 12 ) ) );
			pButtons->Add( NWx::Child<wxButton>( this, wxID_CANCEL, "Cancel" ) );
			pContent->Add( pButtons, wxSizerFlags().Expand().Border( wxTOP, FromDIP( 11 ) ) );
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pContent, wxSizerFlags( 1 ).Expand().Border( wxALL, FromDIP( 11 ) ) );
			SetSizer( pSizer );
			SetMinSize( wxSize( 250, 180 ) );
			SetClientSize( FromDIP( wxSize( 651, 562 ) ) );

			// CScintillaEditorWindow::CreateEx: every margin hidden, CRLF, word wrap.
			pEditor->SetMarginWidth( 0, 0 );
			pEditor->SetMarginWidth( 1, 0 );
			pEditor->SetMarginWidth( 2, 0 );
			pEditor->SetEOLMode( wxSTC_EOL_CRLF );
			pEditor->SetWrapMode( wxSTC_WRAP_WORD );
			// And its SetText: the text goes in writable, with no undo history,
			// and the read-only state is set after.
			pEditor->SetReadOnly( false );
			pEditor->ClearAll();
			pEditor->AddText( FromNarrow( rszText ) );
			pEditor->EmptyUndoBuffer();
			pEditor->SetReadOnly( !bEnableEdit );

			placement.Restore( this );
			if ( !rszTitle.empty() )
			{
				SetTitle( wxString::FromUTF8( rszTitle.c_str() ) );
			}
			// OK is the template's first control that takes the focus.
			pOk->SetFocus();
		}

		bool WasPlaced() const { return placement.WasPlaced(); }
		void SaveState() { placement.Save( this ); }
		std::string GetEditorText() const { return ToUtf8( pEditor->GetText() ); }
	};
}


namespace NTextEditor
{
	bool RunScript( IWidget *pParent, const std::string &rszTitle, const std::string &rszText,
										bool bEnableEdit, std::string *pszNewText )
	{
		if ( pszNewText == 0 )
		{
			return false;
		}
		CScriptEditorWxDialog dialog( ToWxOwnerWindow( pParent ), rszTitle, rszText, bEnableEdit );
		if ( !dialog.WasPlaced() )
		{
			dialog.CentreOnParent();
		}
		const bool bAccepted = ( dialog.ShowModal() == wxID_OK );
		dialog.SaveState();
		if ( !bAccepted || !bEnableEdit )
		{
			return false;
		}
		( *pszNewText ) = dialog.GetEditorText();
		return true;
	}


	bool RunText( IWidget *pParent, const std::string &rszTitle, const std::string &rszEditor,
									const std::string &rszText, bool bEnableEdit, std::string *pszNewText )
	{
		if ( pszNewText == 0 )
		{
			return false;
		}
		CTextEditorWxDialog dialog( ToWxOwnerWindow( pParent ), rszTitle, rszEditor, rszText, bEnableEdit );
		if ( !dialog.WasPlaced() )
		{
			dialog.CentreOnParent();
		}
		const bool bAccepted = ( dialog.ShowModal() == wxID_OK );
		dialog.SaveState();
		if ( !bAccepted || !bEnableEdit )
		{
			return false;
		}
		( *pszNewText ) = dialog.GetEditorText();
		return true;
	}
}

