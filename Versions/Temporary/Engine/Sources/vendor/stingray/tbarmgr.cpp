#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


namespace {

// The editor names every resource with MAKEINTRESOURCE, so what arrives here is
// an integer id smuggled in a pointer, not a string. The trace shows 0x271a,
// 0x2711 and 0x2710 going past, which are 10010, 10001 and 10000. Reading one
// of those as a string is what IS_INTRESOURCE exists to prevent.
UINT ResourceIDFromName(LPCTSTR lpszName) {
    if (lpszName == nullptr) {
        return 0;
    }
    if (IS_INTRESOURCE(lpszName)) {
        return static_cast<UINT>(reinterpret_cast<UINT_PTR>(lpszName));
    }
    // A genuine string name. Nothing in this editor uses one, and the bitmaps
    // here are paired to definitions by id, so there is nothing useful to do
    // with it but say so.
    spdlog::warn("SECToolBarManager: toolbar resource named by string \"{}\", which is not handled", SafeString( lpszName ));
    return 0;
}

// Which edge a bar docked to a given dock bar sits on. CToolBar needs one of
// these in its own style; the dwAlignment the editor passes is CBRS_ALIGN_ANY,
// which says what docking is permitted, not where the bar goes.
DWORD BarStyleForDockBar(UINT nDockBarID) {
    switch (nDockBarID) {
        case AFX_IDW_DOCKBAR_BOTTOM: return CBRS_BOTTOM;
        case AFX_IDW_DOCKBAR_LEFT:   return CBRS_LEFT;
        case AFX_IDW_DOCKBAR_RIGHT:  return CBRS_RIGHT;
        default:                     return CBRS_TOP;
    }
}

}  // namespace

SECToolBarManager::SECToolBarManager() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

// The frame was being dropped on the floor. SECControlBarManager keeps it in
// m_pFrameWnd and GetFrameWnd hands it back, and everything below needs it in
// order to have something to create a bar on.
SECToolBarManager::SECToolBarManager(CFrameWnd* pFrameWnd, CFrameWnd* pOwnerFrame) {
    spdlog::debug("{} this={} pFrameWnd={} pOwnerFrame={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFrameWnd), spdlog::fmt_lib::ptr(pOwnerFrame));
    m_pFrameWnd = pFrameWnd;
}

// The bars are ours: they were allocated here, and CControlBar defaults
// m_bAutoDelete to FALSE so nothing else will free them. By the time this runs
// the frame window is gone and their windows with it, which a CWnd destructor
// copes with.
SECToolBarManager::~SECToolBarManager() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    for (ToolBarDef &def : m_defs) {
        delete def.pBar;
        def.pBar = nullptr;
    }
}

SECToolBarManager::ToolBarDef* SECToolBarManager::FindDef(UINT nID) {
    for (ToolBarDef &def : m_defs) {
        if (def.nID == nID) {
            return &def;
        }
    }
    return nullptr;
}

// Keep the definition. Everything needed to build the bar is here except a
// frame that is ready to take it, which is why this only records.
void SECToolBarManager::DefineDefaultToolBar(UINT nID, const CString& strTitle, UINT nBtnCount, UINT* lpBtnIDs, DWORD dwAlignment, UINT nDockBarID, UINT nDockNextToID, BOOL bDocked, BOOL bVisible) {
    spdlog::debug("{} this={} nID={} strTitle={} nBtnCount={} lpBtnIDs={} dwAlignment={} nDockBarID={} nDockNextToID={} bDocked={} bVisible={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
    nID, strTitle.GetString(), nBtnCount, spdlog::fmt_lib::ptr(lpBtnIDs), dwAlignment, nDockBarID, nDockNextToID, bDocked, bVisible);

    if (ToolBarDef *pExisting = FindDef(nID)) {
        spdlog::warn("SECToolBarManager::DefineDefaultToolBar: toolbar {} already defined as \"{}\", ignoring", nID, pExisting->strTitle.GetString());
        return;
    }

    ToolBarDef def;
    def.nID = nID;
    def.strTitle = strTitle;
    if (lpBtnIDs != nullptr) {
        def.btnIDs.assign(lpBtnIDs, lpBtnIDs + nBtnCount);
    }
    def.dwAlignment = dwAlignment;
    def.nDockBarID = nDockBarID;
    def.nDockNextToID = nDockNextToID;
    def.bDocked = bDocked;
    def.bVisible = bVisible;

    // The toolkit builds one shared bitmap out of every resource it is given and
    // has each bar index into it. Here each bar loads its own, so a definition
    // has to be told which one, and the only thing relating them is order: the
    // editor loads a bitmap and defines the toolbar that uses it, in step, both
    // for its own six and for the three the editor modules add later.
    if (m_defs.size() < m_bitmaps.size()) {
        def.nBitmapID = m_bitmaps[m_defs.size()];
    } else {
        spdlog::warn("SECToolBarManager::DefineDefaultToolBar: toolbar {} \"{}\" has no bitmap; {} defined against {} loaded", nID, strTitle.GetString(), m_defs.size() + 1, m_bitmaps.size());
    }

    m_defs.push_back(std::move(def));

    // A definition arriving after the bars were built is one of the editor
    // modules', so build it now rather than leaving it until the next load.
    if (m_bBarsCreated) {
        CreateBars();
    }
}

// Turn every definition that has no bar yet into a real docked toolbar.
//
// This cannot happen inside DefineDefaultToolBar, which is where everything it
// needs arrives. CMainFrame::OnCreate defines all six of its toolbars before it
// calls EnableDocking( CBRS_ALIGN_ANY ) on itself, and that is the call that
// creates the frame's dock bars: docking anything before it has nowhere to go.
// So the definitions wait, and LoadState, which the editor calls once the frame
// and every module's controls are up, is what asks for them.
//
// Bars are created visible and then hidden if the definition says so. Docking
// does not show a hidden bar -- CDockBar::DockControlBar re-shows one only if it
// was visible already -- so creating it hidden and expecting the dock to reveal
// it gives a bar that is docked and invisible.
void SECToolBarManager::CreateBars() {
    CFrameWnd *pFrame = GetFrameWnd();
    if (pFrame == nullptr || pFrame->GetSafeHwnd() == nullptr) {
        spdlog::warn("SECToolBarManager::CreateBars: no frame to build {} toolbars on", m_defs.size());
        return;
    }
    m_bBarsCreated = TRUE;

    for (ToolBarDef &def : m_defs) {
        if (def.pBar != nullptr) {
            continue;
        }
        SECCustomToolBar *pBar = new SECCustomToolBar();
        const DWORD dwStyle = WS_CHILD | WS_VISIBLE | BarStyleForDockBar(def.nDockBarID) |
                              CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY;
        if (!pBar->CreateEx(0, pFrame, dwStyle, def.nID, def.strTitle)) {
            spdlog::warn("SECToolBarManager::CreateBars: toolbar {} \"{}\" failed to create", def.nID, def.strTitle.GetString());
            delete pBar;
            continue;
        }

        // The bitmap first, then the buttons: CToolBar numbers the button faces
        // in button order, skipping separators, which is the same order the
        // bitmap's frames were authored in for this toolbar.
        if (def.nBitmapID != 0 && !pBar->LoadBitmap(def.nBitmapID, nullptr, 0)) {
            spdlog::warn("SECToolBarManager::CreateBars: toolbar {} \"{}\" could not load bitmap {}", def.nID, def.strTitle.GetString(), def.nBitmapID);
        }
        if (!def.btnIDs.empty() && !pBar->SetButtons(def.btnIDs.data(), static_cast<int>(def.btnIDs.size()))) {
            spdlog::warn("SECToolBarManager::CreateBars: toolbar {} \"{}\" rejected its {} buttons", def.nID, def.strTitle.GetString(), def.btnIDs.size());
        }

        pBar->EnableDocking(def.dwAlignment);
        if (def.bDocked) {
            pFrame->DockControlBar(pBar, def.nDockBarID);
        }
        pFrame->ShowControlBar(pBar, def.bVisible, TRUE);

        def.pBar = pBar;
        spdlog::debug("SECToolBarManager::CreateBars: toolbar {} \"{}\" created with {} buttons, docked={} visible={}", def.nID, def.strTitle.GetString(), pBar->GetBtnCount(), def.bDocked, def.bVisible);
    }
    pFrame->RecalcLayout();
}

void SECToolBarManager::DefineDefaultToolBar(UINT nID, const CString& strTitle, UINT nToolbarID, UINT& nRetButtonCount, UINT*& pRetButtonArray, DWORD dwAlignment, UINT nDockBarID, UINT nDockNextToID, BOOL bDocked, BOOL bVisible) {
    spdlog::debug("{} this={} nID={} strTitle={} nToolbarID={} nRetButtonCount={} pRetButtonArray={} dwAlignment={} nDockBarID={} nDockNextToID={} bDocked={} bVisible={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
    nID, strTitle.GetString(), nToolbarID, nRetButtonCount, spdlog::fmt_lib::ptr(pRetButtonArray), dwAlignment, nDockBarID, nDockNextToID, bDocked, bVisible);
}

BOOL SECToolBarManager::IsToolBarCommand(CRect& rect) const {
    spdlog::debug("{} this={} rect.left={} rect.top={} rect.right={} rect.bottom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
         rect.left, rect.top, rect.right, rect.bottom);
    return FALSE;
}

SECCustomToolBar* SECToolBarManager::ToolBarUnderRect(const CRect& rect) const {
    spdlog::debug("{} this={} rect.left={} rect.top={} rect.right={} rect.bottom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
        rect.left, rect.top, rect.right, rect.bottom);
    return nullptr;
}

// The bar carrying that default toolbar id, or null if it has none yet.
//
// Null here is what the editor tests for, and every caller in the editor guards
// on it, so it stays the answer before CreateBars has run rather than something
// invented.
SECCustomToolBar* SECToolBarManager::ToolBarFromID(const UINT nToolBarID) const {
    spdlog::debug("{} this={} nToolBarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nToolBarID);
    for (const ToolBarDef &def : m_defs) {
        if (def.nID == nToolBarID) {
            return def.pBar;
        }
    }
    return nullptr;
}

SECCustomToolBar* SECToolBarManager::CreateUserToolBar(LPCTSTR lpszTitle) {
    spdlog::debug("{} this={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTitle ));
    return nullptr;
}

// LoadToolBarResource starts the list over, AddToolBarResource appends to it.
// The large-button bitmap is ignored throughout: large buttons are not offered
// here, so keeping a second bitmap for them would only be storage.
//
// These return TRUE where they used to return FALSE, which the editor does look
// at: CMainFrame::OnCreate wraps both in VERIFY. TRUE is now the honest answer,
// since the resource really is recorded and a definition will be paired with it.
BOOL SECToolBarManager::LoadToolBarResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName) {
    spdlog::debug("{} this={} lpszStdBmpName={} lpszLargeBmpName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpszStdBmpName), spdlog::fmt_lib::ptr(lpszLargeBmpName));
    m_bitmaps.clear();
    return LoadToolBarResource(ResourceIDFromName(lpszStdBmpName), 0);
}

BOOL SECToolBarManager::LoadToolBarResource(UINT nIDStdBmp, UINT nIDLargeBmp) {
    spdlog::debug("{} this={} nIDStdBmp={} nIDLargeBmp={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDStdBmp, nIDLargeBmp);
    m_bitmaps.clear();
    if (nIDStdBmp == 0) {
        return FALSE;
    }
    m_bitmaps.push_back(nIDStdBmp);
    return TRUE;
}

// Still a stub, and the one overload nothing calls: with no resource named
// there is nothing to load.
BOOL SECToolBarManager::LoadToolBarResource() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECToolBarManager::AddToolBarResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName) {
    spdlog::debug("{} this={} lpszStdBmpName={} lpszLargeBmpName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpszStdBmpName), spdlog::fmt_lib::ptr(lpszLargeBmpName));
    return AddToolBarResource(ResourceIDFromName(lpszStdBmpName), 0);
}

BOOL SECToolBarManager::AddToolBarResource(UINT nIDStdBmp, UINT nIDLargeBmp) {
    spdlog::debug("{} this={} nIDStdBmp={} nIDLargeBmp={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDStdBmp, nIDLargeBmp);
    if (nIDStdBmp == 0) {
        return FALSE;
    }
    m_bitmaps.push_back(nIDStdBmp);
    return TRUE;
}

BOOL SECToolBarManager::AddBitmapResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName, const UINT* lpIDArray, UINT nIDCount) {
    spdlog::debug("{} this={} lpszStdBmpName={} lpszLargeBmpName={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszStdBmpName ), SafeString( lpszLargeBmpName ), spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

BOOL SECToolBarManager::AddBitmapResource(UINT nIDstdBmpName, UINT nIDLargeBmpName, const UINT* lpIDArray, UINT nIDCount) {
    spdlog::debug("{} this={} nIDstdBmpName={} nIDLargeBmpName={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDstdBmpName, nIDLargeBmpName, spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

BOOL SECToolBarManager::AddBitmap(HBITMAP hBmpSmall, HBITMAP hBmpLarge, const UINT* lpIDArray, UINT nIDCount) {
    spdlog::debug("{} this={} hBmpSmall={} hBmpLarge={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hBmpSmall), spdlog::fmt_lib::ptr(hBmpLarge), spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

int SECToolBarManager::ExecViewToolBarsDlg() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECToolBarManager::InformBtns(UINT nID, UINT nCode, void* pData) {
    spdlog::debug("{} this={} nID={} nCode={} pData={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID, nCode, spdlog::fmt_lib::ptr(pData));
}

// These four pairs were a set of stubs that answered: every Enable did nothing
// and every query said FALSE, so the manager reported tooltips off immediately
// after being told to turn them on. The flags are now kept and reported.
//
// Tooltips and flyby help are also applied, because a control bar takes them
// from its own style: CBRS_TOOLTIPS and CBRS_FLYBY, which CreateBars already
// sets on every bar it makes, and which are toggled here on bars that exist.
void SECToolBarManager::EnableToolTips(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    m_bToolTips = bEnable;
    for (ToolBarDef &def : m_defs) {
        if (def.pBar != nullptr && def.pBar->GetSafeHwnd() != nullptr) {
            def.pBar->SetBarStyle(bEnable ? (def.pBar->GetBarStyle() | CBRS_TOOLTIPS)
                                          : (def.pBar->GetBarStyle() & ~CBRS_TOOLTIPS));
        }
    }
}

void SECToolBarManager::EnableFlyBy(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    m_bFlyBy = bEnable;
    for (ToolBarDef &def : m_defs) {
        if (def.pBar != nullptr && def.pBar->GetSafeHwnd() != nullptr) {
            def.pBar->SetBarStyle(bEnable ? (def.pBar->GetBarStyle() | CBRS_FLYBY)
                                          : (def.pBar->GetBarStyle() & ~CBRS_FLYBY));
        }
    }
}

BOOL SECToolBarManager::ToolTipsEnabled() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bToolTips;
}

BOOL SECToolBarManager::FlyByEnabled() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bFlyBy;
}

// The flag is kept and reported, but nothing acts on it: large buttons mean a
// second, bigger bitmap per toolbar, and the large bitmap handed to
// LoadToolBarResource is discarded here. Says so rather than pretending.
void SECToolBarManager::EnableLargeBtns(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    m_bLargeBtns = bEnable;
    if (bEnable) {
        spdlog::warn("SECToolBarManager::EnableLargeBtns: large buttons are not drawn; the large bitmaps are not kept");
    }
}

BOOL SECToolBarManager::LargeBtnsEnabled() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bLargeBtns;
}

// The cool look is the toolkit's own painting: flat buttons, a gripper, a close
// box. None of that is drawn here. TBSTYLE_FLAT, which CreateBars gives every
// bar, is the common control's nearest equivalent and is on regardless, so this
// records the request and reports it back honestly.
void SECToolBarManager::EnableCoolLook(BOOL bEnable, DWORD dwExCoolLookStyles) {
    spdlog::debug("{} this={} bEnable={} dwExCoolLookStyles={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, dwExCoolLookStyles);
    m_bCoolLook = bEnable;
    m_dwCoolLookStyles = dwExCoolLookStyles;
}

BOOL SECToolBarManager::CoolLookEnabled() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bCoolLook;
}

// Kept so GetButtonMap can answer with it. The map pairs a button with a second
// command for its drop-down half, which needs the customize machinery to mean
// anything, so nothing here reads it yet.
void SECToolBarManager::SetButtonMap(const SECBtnMapEntry* pMap) {
    spdlog::debug("{} this={} pMap={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pMap));
    m_pButtonMap = pMap;
}

const SECBtnMapEntry* SECToolBarManager::GetButtonMap() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_pButtonMap;
}

// Build the bars in the arrangement the definitions asked for, which is all the
// default dock state is. CMainFrame has this call commented out and reaches the
// same place through LoadState.
void SECToolBarManager::SetDefaultDockState() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    CreateBars();
}

BOOL SECToolBarManager::SetMenuInfo(int nCount, UINT nIDMenu, ...) {
    spdlog::debug("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    return FALSE;
}

// Where the toolbars actually get built.
//
// The toolkit restores each customizable bar's saved buttons and position from
// the registry here. This does not save any of that, so there is nothing to read
// back, and what it does instead is the part that has to happen either way:
// create the bars in their defined arrangement. The editor calls this once the
// frame is docking-ready and every module has added its own toolbars, which is
// exactly when they can be built.
//
// A bar's docked position is separately restored by the frame's own
// LoadBarState, which CMainFrame::OnCreate calls just *after* this and which
// finds these bars by the ids CreateBars gave them. That order matters and this
// comment used to have it backwards: with LoadBarState first, the bars it wants
// have not been created, it cannot find them, and the editor's check discards
// the whole saved layout rather than restoring any of it.
void SECToolBarManager::LoadState(const CString & state) {
    spdlog::debug("{} this={} state={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), state.GetString());
    CreateBars();
}

// Still a stub, and honestly empty. What the toolkit writes here is which
// buttons the user rearranged onto which bar, and no arrangement is possible in
// this library, so the definitions are the whole state and they come from the
// editor's own tables every run. The bars' docked positions are saved by the
// frame's SaveBarState, not by this.
void SECToolBarManager::SaveState(const CString & state) {
    spdlog::debug("{} this={} state={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), state.GetString());
}
