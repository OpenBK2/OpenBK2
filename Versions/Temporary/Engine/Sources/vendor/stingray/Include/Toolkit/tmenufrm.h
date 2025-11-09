#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "tbarcust.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar.htm
// SECMenuBar uses SECCustomToolBar as a base class
class SECMenuBar : public SECCustomToolBar {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__secmenubar.htm
    // Constructor
    SECMenuBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__.7esecmenubar.htm
    // Destructor
    virtual ~SECMenuBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__loadmenu.htm
    // Loads the menu specified
    BOOL LoadMenu(UINT nIDResource);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__switchmenu.htm
    // Switches to the menu specified
    BOOL SwitchMenu(UINT nIDResource);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__enablebitflag.htm
    // Enables the bit flag
    virtual void EnableBitFlag(DWORD dwBit, BOOL bUpdate = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__onbarstylechange.htm
    // Notifies you when the style is changing
    virtual void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__getmenufont.htm
    // Returns font used for the menu
    virtual HFONT GetMenuFont() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__resetmenus.htm
    // Resets the menu
    virtual void ResetMenus(BOOL bNoUpdate = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__setmenuinfo.htm
    // Defines the menu resources to use.
    virtual BOOL SetMenuInfo(int nCount, UINT nIDMenu, ...);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmenubar__getcurmenuid.htm
    // Returns the current menu ID
    UINT GetCurMenuID() const;
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdimenubar.htm
// SECMDIMenuBar is a SECMenuBar derivative

class SECMDIMenuBar : public SECMenuBar {
public:
};
