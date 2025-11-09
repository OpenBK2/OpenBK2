#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

enum {
    // Vertical progress control
    SEC_EX_PROGRESS_VERT,
    // Horizontal displays right to left
    SEC_EX_PROGRESS_RIGHT_TO_LEFT,
    // Vertical displays top to bottom
    SEC_EX_PROGRESS_TOP_TO_BOTTOM,
    // Show percentage complete text
    SEC_EX_PROGRESS_SHOWPERCENT,
    // Show custom text inside bar
    SEC_EX_PROGRESS_SHOWTEXT,
    // Left justify shown text/percent
    SEC_EX_PROGRESS_TEXT_ALIGN_LEFT,
    // Right justify shown text/percent
    SEC_EX_PROGRESS_TEXT_ALIGN_RIGHT,
    // Center shown text/percent,
    SEC_EX_PROGRESS_TEXT_ALIGN_CENTER,
    // Has look and feel of CProgressCtrl
    SEC_EX_PROGRESS_COMMCTRL32,
    SEC_EX_PROGRESS_DEFAULTS = SEC_EX_PROGRESS_SHOWPERCENT | SEC_EX_PROGRESS_TEXT_ALIGN_CENTER,
};

enum {
    SEC_PROGRESS_DEF_FGND_COLOR = 0x000000FF, // dark blue
    SEC_PROGRESS_DEF_BGND_COLOR = 0x00FFFFFF, // white
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl.htm

class SECProgressCtrl : public CProgressCtrl {
public:
    // Creation/Initialization
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__secprogressctrl.htm
    // Constructs a SECProgress control object.
    SECProgressCtrl();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__create.htm
    // Dynamically create a progress control.
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID,DWORD dwExStyle=SEC_EX_PROGRESS_DEFAULTS);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__attachprogress.htm
    // Attach to an existing progress control.
    virtual BOOL AttachProgress(int nCtlID,CWnd* pParentWnd);

    // Attributes
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setrange.htm
    // Set progress value range for appropriate completion percentage.
    virtual void SetRange(ULONG ulLower,ULONG ulUpper);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setpos.htm
    // Set current progress position.
    virtual ULONG SetPos(ULONG ulPos,BOOL bYield=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__offsetpos.htm
    // Increment current progress position.
    virtual ULONG OffsetPos(ULONG ulPos,BOOL bYield=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setstep.htm
    // Set step size for "StepIt" function call.
    virtual ULONG SetStep(ULONG ulStep);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setcolors.htm
    // Set progress control foreground (bar) and background colors.
    virtual void SetColors(COLORREF fgnd=SEC_PROGRESS_DEF_FGND_COLOR, COLORREF bgnd=SEC_PROGRESS_DEF_BGND_COLOR);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setfont.htm
    // Set the font for progress text.
    virtual void SetFont(CFont* pFont=(CFont *)NULL,BOOL bRedraw=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setwindowtext.htm
    // Set the current progress text.
    virtual void SetWindowText(LPCTSTR lpszNewText=(const TCHAR *)NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__getwindowtext.htm
    // Get the current progress text.
    virtual void GetWindowText(CString& strText);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__setexstyle.htm
    // Set the progress extended styles from the SEC_EX_PROGRESS_* flags.
    void SetExStyle(DWORD dwExNewStyle);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__getexstyle.htm
    // Get the current progress extended styles.
    DWORD GetExStyle();

    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__stepit.htm
    // Step the progress up one increment of the step size (set by SetStep).
    virtual ULONG StepIt(BOOL bYield=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__resetprogress.htm
    // Reset the progress indicator to origin.
    virtual void ResetProgress();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__stepontimer.htm
    // AutoStep the timer at a regular interval.
    virtual BOOL StepOnTimer(UINT nInterval);

    // Overridable
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__oninitprogress.htm
    // Progress control is initializing.
    virtual BOOL OnInitProgress();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__onpaintbarfill.htm
    // Paint the filled progress bar image.
    virtual void OnPaintBarFill(CDC* pDC,CRect rectFill);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__onpaintbarempty.htm
    // Paint the empty "unused" portion of the progress bar.
    virtual void OnPaintBarEmpty(CDC* pDC,CRect rectEmpty);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__onpaintbartext.htm
    // Paint the progress text, if any.
    virtual void OnPaintBarText(CDC* pDC,float fPctComplete,CRect rectEmpty,CRect rectFilled);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__ondisplaystr.htm
    // Override this to alter a progress string before display.
    virtual void OnDisplayStr(CString& strToDisplay);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__dopaint.htm
    // Paints the progress control.
    virtual void DoPaint(CPaintDC* pdc);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__calcprogressrects.htm
    // Calculates the filled/empty rectangles for the progress control.
    virtual BOOL CalcProgressRects(float fPct,CRect& rectFilled,CRect& rectEmpty);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__paintprogressbarandtext.htm
    // Paints the progress bar and text.
    virtual BOOL PaintProgressBarAndText(float fPctComplete,CRect rectFilled, CRect rectEmpty,CDC* pdc);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secprogressctrl__calcpercentcomplete.htm
    // Calculates the percent complete.
    virtual float CalcPercentComplete();
};
