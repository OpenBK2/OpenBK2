#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <vector>

#include "secres.h"

class SECToolBarManager;

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarsheet.htm

class SECToolBarSheet: public CPropertySheet {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarsheet__sectoolbarsheet.htm
    SECToolBarSheet(UINT nIDCaption = IDS_TOOLBAR_CUSTOMIZE, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

protected:
    BOOL OnInitDialog() override;
};

//! The grid of button faces the Commands page draws inside its "Buttons" group
//! box. The toolkit had no template id for this window; it is created here.
//!
//! One cell per command: its toolbar image where it has one, its text where it
//! does not, which is the case for everything that only ever appears on a menu.
//! Clicking a cell tells the page, which puts the command's prompt in the
//! description box. The toolkit also let a cell be dragged onto a toolbar --
//! that is not implemented, because every bar here loads its own bitmap and a
//! button moved between two of them would index into the wrong one.
class SECCmdButtonGrid : public CWnd {
public:
    //! One command offered in the grid.
    struct Cell {
        UINT nID = 0;
        CString strText;        //!< the menu text, for commands with no image
        HIMAGELIST hImages = nullptr;
        int nImage = -1;
    };

    //! Not named Create: CWnd::Create is virtual and this is a different
    //! thing with a different signature, so hiding it would only mislead.
    BOOL CreateGrid(CWnd* pParent, const CRect& rect, UINT nID);
    void SetCells(std::vector<Cell> cells);
    //! The selected command, or zero.
    UINT GetSelectedID() const;

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

    //! Cell geometry, in pixels. The width is a minimum: as many columns as
    //! fit at it, then the client width shared out between them, so a row is
    //! filled rather than leaving a strip of empty box at the right. Wide
    //! enough for a 16x15 toolbar face plus a command name beside it.
    static const int nMinCellWidth = 100;
    static const int nCellHeight = 22;

    int Columns() const;
    int CellWidth() const;
    CRect CellRect(int nIndex) const;
    void UpdateScrollBar();

    std::vector<Cell> m_cells;
    int m_nSel = -1;
    int m_nScroll = 0;          //!< first visible row
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage.htm

class SECToolBarCmdPage : public CPropertyPage {
public:
    enum { IDD = IDD_SEC_COMMANDS_PAGE };
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__sectoolbarcmdpage.htm
    // Constructs an SECToolBarCmdPage object.
    SECToolBarCmdPage();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__sectoolbarcmdpage.htm
    // Constructs an SECToolBarCmdPage object.
    SECToolBarCmdPage(UINT nIDTemplate, UINT nIDCaption = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__sectoolbarcmdpage.htm
    // Constructs an SECToolBarCmdPage object.
    SECToolBarCmdPage(LPCTSTR lpszTemplate, UINT nIDCaption = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__setmanager.htm
    // Defines the toolbar manager.
    void SetManager(SECToolBarManager* pManager);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__definebtngroup.htm
    // Defines a button group, consisting of a title and an array of button IDs
    void DefineBtnGroup(LPCTSTR lpszTitle, int nBtnCount, UINT* lpBtnIDs);
    void DefineMenuGroup(LPCTSTR lpszTitle);

    //! One named group of commands, as the editor defines them: a toolbar's
    //! buttons, or a menu offered whole.
    struct CmdGroup {
        CString strTitle;
        std::vector<UINT> btnIDs;   //!< empty for a menu group
        bool bMenu = false;
    };

protected:
    BOOL OnInitDialog() override;

    //! A different category picked: refill the grid from that group.
    afx_msg void OnSelChangeCategory();
    //! A cell clicked: show that command's prompt.
    afx_msg void OnGridSelect();

    DECLARE_MESSAGE_MAP()

    //! Fill m_cells for one group. A menu group's commands come from the
    //! manager's menu resources rather than from a list handed in.
    std::vector<SECCmdButtonGrid::Cell> CellsForGroup(const CmdGroup& group) const;

    //! Everything the editor described before opening the dialog: the manager,
    //! one button group per toolbar, the custom toolbars from the profile, and
    //! the menus as a group of their own. CMainFrame defines nine toolbars and
    //! a menu group on every Tools -> Customize.
    SECToolBarManager* m_pManager = nullptr;
    std::vector<CmdGroup> m_groups;

    CListBox m_wndCategories;
    SECCmdButtonGrid m_wndGrid;
};
