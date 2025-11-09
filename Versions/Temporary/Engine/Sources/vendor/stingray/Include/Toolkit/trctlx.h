#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "trcore.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectreectrl.htm

class SECTreeCtrl: public SEC_TREECLASS {
public:
    // Construction/Initialization
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectreectrl__sectreectrl.htm
    // Constructor
    SECTreeCtrl();

    // Dynamic Layout
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectreectrl__enabledynamiclayout.htm
    // // Enables dynamic moving and resizing or the tree control.
    void EnableDynamicLayout(CMFCDynamicLayout::MoveSettings moveSettings, CMFCDynamicLayout::SizeSettings sizeSettings);
};
