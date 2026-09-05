#include "Toolkit/swinmdi.h"
#include "Toolkit/tbarmgr.h"
#include "dockex.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"

#include <cstdarg>
#include <vector>


BEGIN_MESSAGE_MAP( SECMDIFrameWnd, CMDIFrameWnd )
    ON_WM_INITMENUPOPUP()
    ON_WM_DESTROY()
END_MESSAGE_MAP()


void SECMDIChildWnd::SwapMenu(UINT nID) {
    spdlog::debug("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

void SECMDIFrameWnd::EnableContextListMode(BOOL bEnable) {

    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

namespace
{

// A 32 bit top-down DIB, and the pixels behind it.
HBITMAP MakeDib( int cx, int cy, DWORD **ppBits )
{
    BITMAPINFO info;
    ::ZeroMemory( &info, sizeof( info ) );
    info.bmiHeader.biSize = sizeof( info.bmiHeader );
    info.bmiHeader.biWidth = cx;
    info.bmiHeader.biHeight = -cy;          // negative: top down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *pBits = nullptr;
    const HDC hScreenDC = ::GetDC( nullptr );
    const HBITMAP hBitmap = ::CreateDIBSection( hScreenDC, &info, DIB_RGB_COLORS, &pBits, nullptr, 0 );
    ::ReleaseDC( nullptr, hScreenDC );
    *ppBits = static_cast< DWORD * >( pBits );
    return hBitmap;
}

// One toolbar image, as a bitmap a menu can show.
//
// Menus take a bitmap through MENUITEMINFO::hbmpItem and draw it with
// premultiplied alpha, so the alpha channel has to be right or the image comes
// out invisible. These toolbars are old bitmaps with a mask and no alpha at
// all, and ImageList_Draw does not write one: drawing masked into a zeroed DIB
// leaves every pixel at alpha zero, which is a fully transparent icon. It looks
// like a faint outline, which is exactly what the first attempt at this
// produced.
//
// So the mask is recovered rather than asked for. The image is drawn twice,
// once over black and once over white; a pixel the image actually covers comes
// out the same both times, and a pixel it does not takes the colour underneath.
// That difference is the mask, and it gives alpha 255 or 0 per pixel.
HBITMAP MenuBitmapFromImageList( HIMAGELIST hImageList, int nImage )
{
    int cx = 0;
    int cy = 0;
    if ( !ImageList_GetIconSize( hImageList, &cx, &cy ) || cx <= 0 || cy <= 0 )
    {
        return nullptr;
    }
    DWORD *pOverBlack = nullptr;
    DWORD *pOverWhite = nullptr;
    const HBITMAP hOverBlack = MakeDib( cx, cy, &pOverBlack );
    const HBITMAP hOverWhite = MakeDib( cx, cy, &pOverWhite );
    if ( hOverBlack == nullptr || hOverWhite == nullptr || pOverBlack == nullptr || pOverWhite == nullptr )
    {
        if ( hOverBlack != nullptr ) { ::DeleteObject( hOverBlack ); }
        if ( hOverWhite != nullptr ) { ::DeleteObject( hOverWhite ); }
        return nullptr;
    }
    const int nPixels = cx * cy;
    for ( int i = 0; i < nPixels; ++i )
    {
        pOverBlack[i] = 0x00000000;
        pOverWhite[i] = 0x00FFFFFF;
    }
    const HDC hMemDC = ::CreateCompatibleDC( nullptr );
    HGDIOBJ hOld = ::SelectObject( hMemDC, hOverBlack );
    ::ImageList_Draw( hImageList, nImage, hMemDC, 0, 0, ILD_TRANSPARENT );
    ::SelectObject( hMemDC, hOverWhite );
    ::ImageList_Draw( hImageList, nImage, hMemDC, 0, 0, ILD_TRANSPARENT );
    ::SelectObject( hMemDC, hOld );
    ::DeleteDC( hMemDC );

    // Keep the black-background copy: where the two agree the image covered the
    // pixel and its colour is already right, and premultiplied alpha wants the
    // uncovered pixels left at zero rather than merely marked transparent.
    for ( int i = 0; i < nPixels; ++i )
    {
        const DWORD rgb = pOverBlack[i] & 0x00FFFFFF;
        pOverBlack[i] = ( rgb == ( pOverWhite[i] & 0x00FFFFFF ) ) ? ( 0xFF000000 | rgb ) : 0;
    }
    ::DeleteObject( hOverWhite );
    return hOverBlack;
}

}

// Put each command's toolbar face next to it in the menus.
//
// The toolkit drew its own menus and so could put a button's bitmap beside its
// command. Windows has offered the same thing since Vista -- MENUITEMINFO's
// hbmpItem -- so no drawing is needed here, only finding the right image and
// handing it over.
//
// The work happens per popup, from OnInitMenuPopup, rather than once over the
// whole menu bar. An MDI frame swaps its menu whenever a document opens or
// closes, so decorating "the menu" would be decorating one of several and would
// have to be redone on every swap; decorating the popup that is about to appear
// is right whichever menu it came from.
void SECMDIFrameWnd::EnableBmpMenus() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    m_bBmpMenus = TRUE;
}

// The bitmaps outlive the popup: a menu does not copy what it is given, so
// these are made once per command and kept until the frame goes away.
void SECMDIFrameWnd::DecorateMenuWithBitmaps( CMenu *pPopupMenu ) {
    SECToolBarManager *const pManager = dynamic_cast< SECToolBarManager * >( m_pControlBarManager );
    if ( !m_bBmpMenus || pPopupMenu == nullptr || pManager == nullptr ) {
        return;
    }
    const UINT nItems = pPopupMenu->GetMenuItemCount();
    int nDecorated = 0;
    for ( UINT nItem = 0; nItem < nItems; ++nItem ) {
        const UINT nID = pPopupMenu->GetMenuItemID( nItem );
        // -1 is a submenu and 0 a separator; neither is a command.
        if ( nID == 0 || nID == static_cast< UINT >( -1 ) ) {
            continue;
        }
        std::map< UINT, HBITMAP >::const_iterator itCached = m_menuBitmaps.find( nID );
        HBITMAP hBitmap = nullptr;
        if ( itCached != m_menuBitmaps.end() ) {
            hBitmap = itCached->second;
        } else {
            HIMAGELIST hImageList = nullptr;
            int nImage = 0;
            if ( pManager->GetButtonImage( nID, &hImageList, &nImage ) ) {
                hBitmap = MenuBitmapFromImageList( hImageList, nImage );
            }
            // Remembered even when there is no image, so a command that is on no
            // toolbar is looked up once rather than on every popup.
            m_menuBitmaps[nID] = hBitmap;
        }
        if ( hBitmap != nullptr ) {
            MENUITEMINFO itemInfo;
            ::ZeroMemory( &itemInfo, sizeof( itemInfo ) );
            itemInfo.cbSize = sizeof( itemInfo );
            itemInfo.fMask = MIIM_BITMAP;
            itemInfo.hbmpItem = hBitmap;
            ::SetMenuItemInfo( pPopupMenu->GetSafeHmenu(), nItem, TRUE, &itemInfo );
            ++nDecorated;
        }
    }
    spdlog::debug("SECMDIFrameWnd::DecorateMenuWithBitmaps: {} of {} items given a toolbar face",
                  nDecorated, nItems);
}

void SECMDIFrameWnd::OnInitMenuPopup( CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu ) {
    CMDIFrameWnd::OnInitMenuPopup( pPopupMenu, nIndex, bSysMenu );
    if ( !bSysMenu ) {
        DecorateMenuWithBitmaps( pPopupMenu );
    }
}

void SECMDIFrameWnd::OnDestroy() {
    for ( std::map< UINT, HBITMAP >::const_iterator it = m_menuBitmaps.begin();
          it != m_menuBitmaps.end(); ++it ) {
        if ( it->second != nullptr ) {
            ::DeleteObject( it->second );
        }
    }
    m_menuBitmaps.clear();
    CMDIFrameWnd::OnDestroy();
}

// The other half of the docking, and the one the editor actually calls:
// CMainFrame is an MDI frame, so this runs and SECFrameWnd's copy never does.
//
// The Ex is placement the toolkit adds over MFC's DockControlBar: which row and
// column of the dock bar to land in, and how much of the row to take. This used
// to keep nHeight, as the bar's m_nMRUWidth, and drop the other three, which
// left every bar starting a row of its own and sized square. See dockex.h for
// what the four mean now and how they are honoured.
void SECMDIFrameWnd::DockControlBarEx(CControlBar* pBar, UINT nDockBarID,int nCol, int nRow, float fPctWidth, int nHeight) {
    spdlog::debug("{} this={} pBar={} nDockBarID={} nCol={} nRow={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), nDockBarID, nCol, nRow, fPctWidth, nHeight);
    NDockEx::DockControlBarEx( this, pBar, nDockBarID, nCol, nRow, fPctWidth, nHeight );
}

void SECMDIFrameWnd::ReDockControlBar(CControlBar* pBar, CDockBar* pDockBar, LPCRECT lpRect) {
    spdlog::debug("{} this={} pBar={} pDockBar={} lpRect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), spdlog::fmt_lib::ptr(pDockBar), spdlog::fmt_lib::ptr(lpRect));
}

void SECMDIFrameWnd::FloatControlBarInMDIChild(CControlBar* pBar, CPoint point, DWORD dwStyle) {
    spdlog::debug("{} this={} pBar={} point.x={} point.y={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), point.x, point.y, dwStyle);
}

BOOL SECMDIFrameWnd::EnableCustomCaption(BOOL bEnable, BOOL bRedraw) {

    spdlog::debug("{} this={} bEnable={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bRedraw);
    return FALSE;
}

void SECMDIFrameWnd::ForceCaptionRedraw() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::CreateCaptionAppFont(CFont& font) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::CreateCaptionDocFont(CFont& font) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::DrawCaptionText() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::SetCaptionTextAlign(AlignCaption ac, BOOL bRedraw) {
    spdlog::debug("{} this={} ac={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), static_cast<int>(ac), bRedraw);
}

void SECMDIFrameWnd::SwapMenu(UINT nID) {
    spdlog::debug("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

// Where the frame is told which menus the application has, and the only place
// it is told, so it is where the menu bar learns them.
//
// The menu bar also has to become a window here. CMainFrame::ShowMenu will not
// ask it to switch menus unless ::IsWindow(m_pMenuBar->m_hWnd), and nothing
// else creates it: the toolkit creates its own, and this library had left
// m_pMenuBar an object with no window, so every ShowMenu in the editor was
// skipped and the frame kept the menu LoadFrame gave it. It reports a size of
// zero, so having it costs the frame no room.
BOOL SECMDIFrameWnd::LoadAdditionalMenus(UINT nCount, UINT nIDMenu, ...) {
    spdlog::debug("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    if (m_pMenuBar == nullptr) {
        return FALSE;
    }
    // AFX_IDW_CONTROLBAR_LAST, which is the id the original editor's menu bar
    // carries: its window tree shows it as 59647.
    //
    // Asking GetUniqueBarID for the first id free from AFX_IDW_TOOLBAR was
    // wrong, and only became visibly wrong once the toolbars were real. It
    // answered 59392, because at this point in startup no bar holds that id
    // yet: SECToolBarManager has been given all six definitions but does not
    // build them until later, and 59392 is the first one it will use. The menu
    // bar and the File Toolbar then both had 59392, which is the id
    // GetControlBar, ShowControlBar and LoadBarState address a bar by.
    //
    // The end of the control bar range cannot collide with them, since that is
    // where the toolbar ids count up from.
    if (m_pMenuBar->GetSafeHwnd() == nullptr
        && !m_pMenuBar->CreateEx(0, this, WS_CHILD | CBRS_TOP,
                                 AFX_IDW_CONTROLBAR_LAST,
                                 "SECMenuBar")) {
        spdlog::warn("SECMDIFrameWnd::LoadAdditionalMenus: the menu bar has no window");
        return FALSE;
    }
    std::vector<UINT> menus;
    if (nCount > 0) {
        menus.push_back(nIDMenu);
        va_list args;
        va_start(args, nIDMenu);
        for (UINT i = 1; i < nCount; ++i) {
            menus.push_back(va_arg(args, UINT));
        }
        va_end(args);
    }
    return m_pMenuBar->SetMenus(menus);
}

void SECMDIFrameWnd::EnableOleContainmentMode() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

afx_msg LRESULT SECMDIFrameWnd::OnExtendContextMenu(WPARAM wParam, LPARAM lParam) {

    spdlog::debug("{} this={} wParam={} lParam={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), wParam, lParam);
    return 0;
}

SECWorkspaceManagerEx* SECMDIFrameWnd::InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode, CRuntimeClass* pWSClass, BOOL bSectionKey) {

    spdlog::debug("{} this={} strAppKey={} bRegistryMode={} pWSClass={} bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strAppKey.GetString(), bRegistryMode, spdlog::fmt_lib::ptr(pWSClass), bSectionKey);
    return nullptr;
}
