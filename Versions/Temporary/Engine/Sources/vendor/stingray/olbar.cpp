#include "Toolkit/olbar.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECShortcutBar::SECShortcutBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECShortcutBar::~SECShortcutBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECShortcutBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID) {
    spdlog::debug("{} this={} dwStyle={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, nID);
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    RECT rect{0, 0, 0, 0};
    return CWnd::Create(lpszClassName, "SECShortcutBar", dwStyle, rect, pParentWnd, nID, nullptr);
}

void SECShortcutBar::SetBarClass(CRuntimeClass* const pBarClass) {
    spdlog::debug("{} this={} pBarClass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBarClass));
}

CRuntimeClass* SECShortcutBar::GetBarClass() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SetListBarClass( CRuntimeClass* const pBarClass ) {
    spdlog::debug("{} this={} pBarClass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBarClass));
}

CRuntimeClass* SECShortcutBar::GetListBarClass() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SetListCtrlClass( CRuntimeClass* const pBarClass ) {
    spdlog::debug("{} this={} pBarClass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBarClass));
}

CRuntimeClass* SECShortcutBar::GetListCtrlClass() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SetFontPointSize( const int& iFontPointSize )  {
    spdlog::debug("{} this={} iFontPointSize={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iFontPointSize);
}

int SECShortcutBar::GetFontPointSize() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::SetFontName( const CString& sFontName ) {
    spdlog::debug("{} this={} sFontName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), sFontName.GetString());
}

const CString& SECShortcutBar::GetFontName() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return "";
}

void SECShortcutBar::SetAnimationSpeed( const int& iAnimationSpeed ) {
    spdlog::debug("{} this={} iAnimationSpeed={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iAnimationSpeed);
}

int SECShortcutBar::GetAnimationSpeed() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::SetAnimationStep( const int& iAnimationStep ) {
    spdlog::debug("{} this={} iAnimationStep={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iAnimationStep);
}

int SECShortcutBar::GetAnimationStep() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::SetBarMenu( HMENU hMenu, int iIndex ){
    spdlog::debug("{} this={} hMenu={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hMenu), iIndex);
}

void SECShortcutBar::SetBarMenu( CMenu* pSubMenu, int iIndex, int iLevel ){
    spdlog::debug("{} this={} pSubMenu={} iIndex={} iLevel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSubMenu), iIndex, iLevel);
}

void SECShortcutBar::SetBarFont( CFont* pFont, int iIndex ) {
    spdlog::debug("{} this={} pFont={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFont), iIndex);
}

void SECShortcutBar::SetBarFont( HFONT hFont, int iIndex ) {
    spdlog::debug("{} this={} hFont={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hFont), iIndex);
}

void SECShortcutBar::SetBackFillColor( COLORREF color ) {
    spdlog::debug("{} this={} color={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color);
}

void SECShortcutBar::SetBackFillColor( CBrush* pBackFillBrush ) {
    spdlog::debug("{} this={} pBackFillBrush={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBackFillBrush));
}

void SECShortcutBar::SetFocusRectColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetFocusRectColor( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

void SECShortcutBar::SetTextColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetTextColor(int iIndex) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

void SECShortcutBar::SetBkColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetBkColor(int iIndex) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

void SECShortcutBar::SetPaneBkColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetPaneBkColor(int iIndex) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

int SECShortcutBar::GetBarCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

CWnd* SECShortcutBar::GetBarWnd( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return nullptr;
}

void SECShortcutBar::SetAlignStyle( DWORD dwAlign ) {
    spdlog::debug("{} this={} dwAlign={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwAlign);
}

DWORD SECShortcutBar::GetAlignStyle() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::ModifyBarStyle( DWORD dwRemove, DWORD dwAdd, BOOL bRecalcRedraw ) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bRecalcRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRecalcRedraw);
}

DWORD SECShortcutBar::GetBarStyle() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

SECBar& SECShortcutBar::GetActiveBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    static SECBar bar;
    return bar;
}

BOOL SECShortcutBar::HasActiveBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

int SECShortcutBar::GetActiveIndex() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SECShortcutBar::IsVertAlign() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECShortcutBar::IsHorzAlign() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECShortcutBar::IsStyleSet( DWORD dwStyle ) const {
    spdlog::debug("{} this={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle);
    return FALSE;
}

SECIterator<SECBar*>* SECShortcutBar::CreateBarIterator() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

SECBar& SECShortcutBar::GetBar( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    static SECBar bar;
    return bar;
}

SECBar* SECShortcutBar::GetBarPtr( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return nullptr;
}

int SECShortcutBar::HitBar( const CPoint& pt ) {
    spdlog::debug("{} this={} pt.x={} pt.y={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), pt.x, pt.y);
    return 0;
}

void SECShortcutBar::SetFlatStyleMode( BOOL bEnabled ) {
    spdlog::debug("{} this={} bEnabled={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnabled);
}


auto SECShortcutBar::AddBar(CWnd *pWnd, LPCTSTR lpszLabel, BOOL bRecalc) -> SECBar * {
    spdlog::debug("{} this={} pWnd={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), lpszLabel, bRecalc);
    return nullptr;
}

SECBar* SECShortcutBar::AddBar(CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext, BOOL bRecalc, UINT nID) {
    spdlog::debug("{} this={} pViewClass={} lpszLabel={} pContext={} bRecalc={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pViewClass), lpszLabel, spdlog::fmt_lib::ptr(pContext), bRecalc, nID);
    return nullptr;
}

SECListBar* SECShortcutBar::AddBar( LPCTSTR lpszLabel, BOOL bRecalc ) {
    spdlog::debug("{} this={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszLabel, bRecalc);
    return nullptr;
}

SECBar* SECShortcutBar::InsertBar( int iIndex, CWnd* pWnd, LPCTSTR lpszLabel, BOOL bRecalc) {
    spdlog::debug("{} this={} iIndex={} pWnd={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, spdlog::fmt_lib::ptr(pWnd), lpszLabel, bRecalc);
    return nullptr;
}

SECBar* SECShortcutBar::InsertBar( int iIndex, CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext, BOOL bRecalc, UINT uID ) {
    spdlog::debug("{} this={} iIndex={} pViewClass={} lpszLabel={} pContext={} bRecalc={} uID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, spdlog::fmt_lib::ptr(pViewClass), lpszLabel, spdlog::fmt_lib::ptr(pContext), bRecalc, uID);
    return nullptr;
}

SECListBar* SECShortcutBar::InsertBar( int iIndex, LPCTSTR lpszLabel, BOOL bRecalc) {
    spdlog::debug("{} this={} iIndex={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, lpszLabel, bRecalc);
    return nullptr;
}

void SECShortcutBar::RemoveBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
}

void SECShortcutBar::RenameBar( int iIndex, LPCTSTR lpszLabel ) {
    spdlog::debug("{} this={} iIndex={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, lpszLabel);
}

void SECShortcutBar::ActivateBar(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
}

void SECShortcutBar::DisableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
}

void SECShortcutBar::EnableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
}

void SECShortcutBar::OnStyleChange( DWORD dwRemovedStyles, DWORD dwAddedStyles ) {
    spdlog::debug("{} this={} dwRemovedStyles={} dwAddedStyles={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemovedStyles, dwAddedStyles);
}

BOOL SECShortcutBar::OnChangeBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return FALSE;
}

BOOL SECShortcutBar::OnRemoveBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return FALSE;
}

BOOL SECShortcutBar::OnDisableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return FALSE;
}

BOOL SECShortcutBar::OnEnableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return FALSE;
}

BOOL SECShortcutBar::OnCreatePaneWnd( CWnd* pwnd ) {
    spdlog::debug("{} this={} pwnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pwnd));
    return FALSE;
}

BOOL SECShortcutBar::OnCreateBar( SECBar* pbar ) {
    spdlog::debug("{} this={} pbar={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pbar));
    return FALSE;
}

void SECShortcutBar::ConstructGDIObjects() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECShortcutBar::DestructGDIObjects() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECBar* SECShortcutBar::CreateNewBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

SECListBar* SECShortcutBar::CreateNewListBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SelectPane( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
}

void RWSetDotNetStyle(bool enable) {
    spdlog::debug("{} enable={}", BOOST_CURRENT_FUNCTION, enable);
}
