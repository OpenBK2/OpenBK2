#pragma once

#include <algorithm>
// No Stingray: the Objective Toolkit headers this used to include were for
// the MFC frame, panes, trees, tab windows and shortcut bar, all gone.
//
// They also brought in <afxpriv.h>, here, before any wx header. WxMfcOwnerDialog.h
// needs it, and included after wx's headers it does not compile: afxconv.h
// fails converting LPOLESTR to LPTSTR. So it is included here, as it was.
#include <afxpriv.h>
