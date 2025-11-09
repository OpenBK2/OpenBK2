#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "sbarstat.h"

#include "trcore.h"

// https://help.perforce.com/stingray/11/html/otug/5-3.html

void RWSetDotNetStyle(bool);

struct SECBar {};
struct SECListBar{};
template<typename T>
struct SECIterator {};

enum { SEC_DEFAULT_ID };

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_HTML_User_Guide/Content/otugTOC/Shortcut_Bar_Styles.htm

enum {
    // Orients the shortcut bar vertically. This is the default orientation.
    SEC_OBS_VERT,
    // Orients the shortcut bar horizontally. This is not a default style.
    SEC_OBS_HORZ,
    // Draws a button-down look for the bar when it is pressed. This is not the default style.
    SEC_OBS_BUTTONFEEDBACK,
    // Enables the display of a context menu when the user clicks the right mouse button on the shortcut bar.
    // You can associate the displayed menu with the shortcut bar via the SECShortcutBar::SetBarMenu() method.
    // This is not the default style.
    // The context menu associated with this flag does not appear when an SECShortcutListCtrl window is right clicked.
    // SECShortcutListCtrl has its own context menu that is displayed in response to a right click.
    SEC_OBS_CONTEXTMENU,
    // Enables animated scrolling. This is not the default style.
    SEC_OBS_ANIMATESCROLL,
    // Draws the focus rectangle on the contained bars when they are given the focus. This is not the default style.
    SEC_OBS_BUTTONFOCUS,
    // The default style for the shortcut bar when none is specified.
    // This style is the same as specifying (WS_VISIBLE | WS_CHILD | SEC_OBS_VERT).
    SEC_DEFAULT_OUTLOOKBAR = WS_VISIBLE | WS_CHILD | SEC_OBS_VERT,
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar.htm

class SECShortcutBar : public CWnd {
public:
    // Constructors/Destructors

    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__secshortcutbar.htm
    // Constructs the SECShortcutBar object
    SECShortcutBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__.7esecshortcutbar.htm
    // Destructs the SECShortcutBar object
    virtual ~SECShortcutBar();

    // Initializations

    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__create.htm
    // Creates the SECShortcutBar window
    virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle = SEC_DEFAULT_OUTLOOKBAR, UINT nID = AFX_IDW_PANE_FIRST);

    // Accessor Functions
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbarclass.htm
    // Set the CRuntimeClass used for SECBar's
    void SetBarClass(CRuntimeClass* const pBarClass);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbarclass.htm
    // Get the CRuntimeClass used for SECBar's
    CRuntimeClass* GetBarClass() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setlistbarclass.htm
    // Set the CRuntimeClass used for SECListBar's
    void SetListBarClass( CRuntimeClass* const pBarClass );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getlistbarclass.htm
    // Get the CRuntimeClass used for SECBar's
    CRuntimeClass* GetListBarClass() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setlistctrlclass.htm
    // Set the CRuntimeClass used for SECShortcutListCtrl's
    void SetListCtrlClass( CRuntimeClass* const pBarClass );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getlistctrlclass.htm
    // Get the CRuntimeClass used for SECShortcutListCtrl's
    CRuntimeClass* GetListCtrlClass() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setfontpointsize.htm
    // Set the font point size to use for SECBar
    void SetFontPointSize( const int& iFontPointSize );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setfontpointsize.htm
    // Get the font point size to use for SECBar
    int GetFontPointSize() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setfontname.htm
    // Set the font name to use for SECBar
    void SetFontName( const CString& sFontName );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getfontname.htm
    // Get the font name to use for SECBar
    const CString& GetFontName() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setanimationspeed.htm
    // Set the Animation speed to use (in ms)
    void SetAnimationSpeed( const int& iAnimationSpeed );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getanimationspeed.htm
    // Get the Animation speed to use (in ms)
    int GetAnimationSpeed() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setanimationstep.htm
    // Set the Animation Step (# of frames in animation)
    void SetAnimationStep( const int& iAnimationStep );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getanimationstep.htm
    // Get the Animation Step (# of frames in animation)
    int GetAnimationStep() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbarmenu.htm
    // Set menu for SECBar to use
    void SetBarMenu( HMENU hMenu, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbarmenu.htm
    // Set menu for SECBar to use
    void SetBarMenu( CMenu* pSubMenu, int iIndex = -1, int iLevel = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbarfont.htm
    // Set font for SECBar to use
    void SetBarFont( CFont* pFont, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbarfont.htm
    // Set font for SECBar to use
    void SetBarFont( HFONT hFont, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbackfillcolor.htm
    // Set color to use as filler in animated scrolls
    void SetBackFillColor( COLORREF color );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbackfillcolor.htm
    // Set brush to use as filler in animated scrolls
    void SetBackFillColor( CBrush* pBackFillBrush );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setfocusrectcolor.htm
    // Set color of the focus rect for SECBar's
    void SetFocusRectColor( COLORREF color, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getfocusrectcolor.htm
    // Get color of the focus rect for SECBar's
    COLORREF GetFocusRectColor( int iIndex = -1 ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__settextcolor.htm
    // Set color of the bar text for SECBar's
    void SetTextColor( COLORREF color, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__gettextcolor.htm
    // Get color of the bar text for SECBar's
    COLORREF GetTextColor(int iIndex = -1) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setbkcolor.htm
    // Set background color of the bar
    void SetBkColor( COLORREF color, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbkcolor.htm
    // Get background color of the bar
    COLORREF GetBkColor(int iIndex = -1) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setpanebkcolor.htm
    // Set background color of the bar
    void SetPaneBkColor( COLORREF color, int iIndex = -1 );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getpanebkcolor.htm
    // Get background color of the bar
    COLORREF GetPaneBkColor(int iIndex = -1) const;

    // Queries
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbarcount.htm
    // Queries the number of bars in SECShortcutBar
    int GetBarCount() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbarwnd.htm
    // Returns the CWnd associated with the Bar
    CWnd* GetBarWnd( int iIndex ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__setalignstyle.htm
    // Sets the alignment style
    void SetAlignStyle( DWORD dwAlign );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getalignstyle.htm
    // Gets the alignment style
    DWORD GetAlignStyle() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__modifybarstyle.htm
    // Modifies the bar style
    void ModifyBarStyle( DWORD dwRemove, DWORD dwAdd, BOOL bRecalcRedraw = TRUE );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbarstyle.htm
    // Queries the bar style
    DWORD GetBarStyle() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getactivebar.htm
    // Get SECBar associated with active bar
    SECBar& GetActiveBar() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__hasactivebar.htm
    // Queries wether or not we have an active bar
    BOOL HasActiveBar() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getactiveindex.htm
    // Returns the index of the active bar
    int GetActiveIndex() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__isvertalign.htm
    // Queries for the style SEC_OBS_VERT
    BOOL IsVertAlign() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__ishorzalign.htm
    // Queries for the style SEC_OBS_HORZ
    BOOL IsHorzAlign() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__isstyleset.htm
    // Queries for a particular style
    BOOL IsStyleSet( DWORD dwStyle ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__createbariterator.htm
    // Creates an iterator to use for traversal
    SECIterator<SECBar*>* CreateBarIterator() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbar.htm
    // Returns a SECBar at specified index
    SECBar& GetBar( int iIndex ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__getbarptr.htm
    // Returns a SECBar* at specified index
    SECBar* GetBarPtr( int iIndex ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__hitbar.htm
    // Returns index of the bar at specified point.
    int HitBar( const CPoint& pt );
    // Specifies whether to use flat style or not.
    void SetFlatStyleMode( BOOL bEnabled = TRUE );

    // Public Overrideables

    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__addbar.htm
    // Adds a new bar at the end
    virtual SECBar* AddBar(CWnd* pWnd,LPCTSTR lpszLabel,BOOL bRecalc=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__addbar.htm
    // Adds a new bar at the end
    virtual SECBar* AddBar(CRuntimeClass* pViewClass,LPCTSTR lpszLabel, CCreateContext* pContext = NULL,BOOL bRecalc = FALSE, UINT nID = SEC_DEFAULT_ID);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__addbar.htm
    // Adds a new SECListBar at the end
    virtual SECListBar* AddBar( LPCTSTR lpszLabel, BOOL bRecalc=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__insertbar.htm
    // Inserts a new bar
    virtual SECBar* InsertBar( int iIndex, CWnd* pWnd, LPCTSTR lpszLabel, BOOL bRecalc=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__insertbar.htm
    // Inserts a new bar
    virtual SECBar* InsertBar( int iIndex, CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext = NULL, BOOL bRecalc = FALSE, UINT uID = SEC_DEFAULT_ID );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__insertbar.htm
    // Inserts a new SECListBar
    virtual SECListBar* InsertBar( int iIndex, LPCTSTR lpszLabel, BOOL bRecalc=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__removebar.htm
    // Removes a bar at specified index
    virtual void RemoveBar( int iIndex );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__renamebar.htm
    // Renames a bar at specified index
    virtual void RenameBar( int iIndex, LPCTSTR lpszLabel );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__activatebar.htm
    // Activates bar at specified index
    virtual void ActivateBar(int nIndex);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__disablebar.htm
    // Disables bar at specified index
    virtual void DisableBar( int iIndex );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__enablebar.htm
    // Enables bar at specified index
    virtual void EnableBar( int iIndex );

protected:
    // Protected Overrideables
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__onstylechange.htm
    // Called when styles changing
    virtual void OnStyleChange( DWORD dwRemovedStyles, DWORD dwAddedStyles );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__onchangebar.htm
    // Called when trying to change bar
    virtual BOOL OnChangeBar( int iIndex );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__onremovebar.htm
    // Called when trying to remove a bar
    virtual BOOL OnRemoveBar( int iIndex );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__ondisablebar.htm
    // Called when trying to disable a bar
    virtual BOOL OnDisableBar( int iIndex );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__onenablebar.htm
    // Called when trying to enable a bar
    virtual BOOL OnEnableBar( int iIndex );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__oncreatepanewnd.htm
    // Called after creating CWnd for bar object
    virtual BOOL OnCreatePaneWnd( CWnd* pwnd );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__oncreatebar.htm
    // Called after creating SECBar object
    virtual BOOL OnCreateBar( SECBar* pbar );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__constructgdiobjects.htm
    // Called to create GDI Objects
    virtual void ConstructGDIObjects();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__destructgdiobjects.htm
    // Called to destroy GDI Objects
    virtual void DestructGDIObjects();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__createnewbar.htm
    // Called to create SECBar object
    virtual SECBar* CreateNewBar() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secshortcutbar__createnewlistbar.htm
    // Called to create SECListBar objects
    virtual SECListBar* CreateNewListBar() const;

public:
    void SelectPane( int iIndex );
};
