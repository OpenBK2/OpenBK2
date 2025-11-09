#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

struct SECControlBarInfo {

};

struct SECControlBarInfoEx {

};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbarmanager.htm?Highlight=SECControlBarManager

class SECControlBarManager: public CObject {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbarmanager__seccontrolbarmanager.htm
    SECControlBarManager() : m_pFrameWnd(nullptr) {}
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbarmanager__seccontrolbarmanager.htm
    SECControlBarManager(CFrameWnd * pFrameWnd) : m_pFrameWnd(pFrameWnd) {}
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbarmanager__getframewnd.htm
    virtual CFrameWnd * GetFrameWnd() const noexcept { return m_pFrameWnd; }
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbarmanager__getbartypeid.htm
    virtual DWORD GetBarTypeID() {
        // Virtual overload provided for override only.
        // No functionality implemented in this base class.
        // It will differentiate between a tool bar and a menu bar in the custom classes.
        return 0;
    }
    virtual SECControlBarInfoEx* CreateControlBarInfoEx(SECControlBarInfo*) const {
        // Virtual overload provided for override only.
        // No functionality implemented in this base class.
        // It will create an appropriate extended information pointer (pInfoEx)
        // for the bar info provided so that the bar may be persistant.
        return nullptr;
    }

    CFrameWnd * m_pFrameWnd = nullptr;
};
