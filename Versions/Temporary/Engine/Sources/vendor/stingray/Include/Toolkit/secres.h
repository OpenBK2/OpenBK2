#ifndef __SECRES_H__
#define __SECRES_H__

// Resource ids for Stingray Objective Toolkit.
//
// MapEditor.rc, ED_B2_M1.rc and ED_B2.rc all open with #include
// "toolkit\secres.h", so the include has to resolve before any of them can be
// compiled at all.
//
// The real header shipped with the commercial library and is not in this tree.
// The ids below were read back out of the retail editor, which had the toolkit
// statically linked and so carries its resources: scripts/port/resdlg.py walks
// RT_DIALOG and RT_STRING in a binary and prints them, and the templates in
// secres.rc beside this file came from the same run. Keeping the toolkit's own
// numbers rather than inventing new ones costs nothing and means a template
// recovered later drops in as it is.
//
//     python scripts/port/resdlg.py <retail B2_MapEditor.exe> --list
//     python scripts/port/resdlg.py <retail B2_MapEditor.exe> --dump 20509
//
// Only what is implemented is listed. Add ids here as they are needed, the same
// way the class stubs beside this file grew.

// Dialogs.
#define IDD_SEC_COMMANDS_PAGE           20509   // SECToolBarCmdPage
#define IDD_SEC_NEW_TOOLBAR             20511   // the New... prompt
#define IDD_SEC_TOOLBARS_PAGE           20513   // SECToolBarsPage

// Strings.
#define IDS_TOOLBAR_CUSTOMIZE           20538   // "Customize", the sheet caption

// Controls, shared between the two pages: 43094 is the categories list on one
// page and the toolbar list on the other, which is why the numbering looks
// interleaved.
#define IDC_SEC_BUTTONS_BOX             43093   // group box the button faces go in
#define IDC_SEC_LIST                    43094   // categories / toolbars
#define IDC_SEC_DESCRIPTION             43095
#define IDC_SEC_NEW_NAME                43096   // edit on the New Toolbar dialog
#define IDC_SEC_NAME_LABEL              43097
#define IDC_SEC_NEW                     43098
#define IDC_SEC_CUSTOMIZE               43099
#define IDC_SEC_RESET                   43100
#define IDC_SEC_TOOLTIPS                43101
#define IDC_SEC_NAME                    43102
#define IDC_SEC_LARGE_BTNS              43103
#define IDC_SEC_COOL_LOOK               43104

// The grid of button faces the Commands page draws inside IDC_SEC_BUTTONS_BOX.
// The toolkit created that window without a template id; this one is ours, and
// is outside the toolkit's block on purpose.
#define IDC_SEC_BUTTON_GRID             43120

#endif // __SECRES_H__
