#pragma once

#include <algorithm>
// No Stingray: the Objective Toolkit headers this used to include were for
// the MFC frame, panes, trees, tab windows and shortcut bar, all gone. The
// <afxpriv.h> that stood in for one of their includes went with MFC, and with
// WxMfcOwnerDialog.h, which was what needed it.
