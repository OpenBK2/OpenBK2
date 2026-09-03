#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "sbarcore.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secstatusbar.htm
// The docs say SECStatusBar does nothing more than rederive from SECControlBar,
// which is where this started, and as a base that class cannot answer any of
// the four questions below: it has no panes.
//
// MFC's CStatusBar has all four, and the original editor's window tree says that
// is what the toolkit was really built on. Its status bar is an
// msctls_statusbar32 with id 59393, which is the common control CStatusBar
// creates and AFX_IDW_STATUS_BAR, the id CStatusBar::Create defaults to; every
// bar the toolkit derives from SECControlBar instead shows up in the same dump
// as an AfxControlBar70 or an Afx: registered class.
//
// So this rederives from CStatusBar and inherits CommandToIndex, SetPaneInfo and
// SetPaneText unchanged. Only SetIndicators needs an override, for the reason
// given over its body.
class SECStatusBar : public CStatusBar {
public:
    BOOL SetIndicators(const UINT *, int);
};
