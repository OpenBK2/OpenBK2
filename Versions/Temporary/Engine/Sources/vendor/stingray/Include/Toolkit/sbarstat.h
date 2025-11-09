#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "sbarcore.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secstatusbar.htm
// SECStatusBar does nothing more than rederive from SECControlBar

class SECStatusBar : public SECControlBar {
public:
    BOOL SetIndicators(const UINT *, int);
    int CommandToIndex(UINT);
    void SetPaneInfo(int, UINT, UINT, UINT);
    void SetPaneText(int, const CString &);
};
