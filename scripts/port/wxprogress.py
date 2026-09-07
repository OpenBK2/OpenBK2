"""How far the wx migration has got, counted from the source.

A static table in a document goes stale the first time someone lands a commit.
This counts instead, so the answer is always about the tree in front of you.

    python scripts/port/wxprogress.py            # the four scoreboards
    python scripts/port/wxprogress.py --detail   # plus the per-item lists

Four axes, because "how far along are we" means different things depending on
what you are worried about:

  module    CMake targets, and whether each still needs MFC or Stingray at all.
            This is the one that reaches zero last and matters most: a module
            that links neither is a module that could build off Windows.
  file      Translation units naming MFC/ATL/Win32 GUI types. The broadest and
            noisiest measure, and the one that moves most slowly.
  dialog    Dialog classes and .rc templates, against those with a wx
            implementation behind a toolkit-neutral boundary.
  control   Instances of MFC control types declared as members, which is the
            inventory the wx equivalents have to cover.

A thing counts as migrated when there is a wx implementation *behind a neutral
boundary* -- an interface or a function that names no toolkit -- not merely when
wx code exists somewhere near it. Two implementations selectable at run time is
the shape; see MapEditor/LogView.h and MapEditor/SelectTablesView.h.
"""
import argparse
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SOURCES = os.path.join(ROOT, "Versions", "Temporary", "Engine", "Sources")

# The editor's own modules. Scintilla and vendor/stingray are excluded: the
# first is a vendored component with its own migration question (wxSTC), the
# second is the shim being deleted rather than migrated.
MODULES = ["B2_MapEditor", "ED_B2", "ED_B2_M1", "ED_Common", "ED_RTS",
           "MapEditor", "MapEditorLib"]

GUI_MFC = re.compile(
    r"\b(CWnd|CDialog|CDC|CPaintDC|CClientDC|CView|CDocument|CFrameWnd|CWinApp|"
    r"CToolBar|CMenu|CImageList|CListCtrl|CTreeCtrl|CEdit|CComboBox|CButton|"
    r"CStatic|CSliderCtrl|CProgressCtrl|CStatusBar|CControlBar|CPropertyPage|"
    r"CBitmap|CBrush|CPen|CFont|CScrollBar|CFileDialog|CColorDialog|CListBox|"
    r"CCheckListBox|CSplitterWnd|CWinThread|CResizeDialog|afx_msg|"
    r"DECLARE_MESSAGE_MAP|BEGIN_MESSAGE_MAP)\b")
STINGRAY = re.compile(r"\bSEC[A-Z][A-Za-z0-9_]*\b")
CONTROL_MEMBER = re.compile(
    r"^\s+(CButton|CEdit|CComboBox|CListBox|CCheckListBox|CListCtrl|CTreeCtrl|"
    r"CSliderCtrl|CSpinButtonCtrl|CProgressCtrl|CStatic|CTabCtrl|CScrollBar|"
    r"CRichEditCtrl|CHeaderCtrl|CImageList|CBitmapButton|CDateTimeCtrl)\s+[a-zA-Z_]",
    re.M)
DIALOG_CLASS = re.compile(
    r"class\s+(?:[A-Z_0-9]+_EXPORT\s+)?([A-Za-z_0-9]+)\s*:\s*public\s+"
    r"(?:CResizeDialog|CDialog|CPCBaseDialog|CPropertyPage)\b")
RC_DIALOG = re.compile(r"^[A-Za-z_0-9]+\s+DIALOG(?:EX)?\s", re.M)

# A wx implementation exists for these. Kept as an explicit list rather than
# inferred, because "there is a file with wx in the name" is not the same as
# "the boundary is neutral and both sides are selectable".
MIGRATED = {
    "CLogWindow": "MapEditor/LogViewWx.cpp (ILogView)",
    "CSelectTablesDialog": "MapEditor/SelectTablesViewWx.cpp (NSelectTables::Run)",
    "COpenMODDialog": "MapEditor/OpenModViewWx.cpp (NOpenMod::Run)",
    "CSearchObjectDialog": "MapEditor/SearchObjectViewWx.cpp (NSearchObject::Run)",
    "CAboutDialog": "MapEditor/AboutViewWx.cpp (NAbout::Run)",
    "CCreateMODDialog": "MapEditor/CreateModViewWx.cpp (NCreateMod::Run)",
    # A palette rather than a dialog: a child window the tab control owns. It is
    # counted here because it is a CResizeDialog like the rest of them, but the
    # boundary in front of it is a different shape -- see CameraPositionView.h.
    "CCameraPositionWindow": "ED_B2_M1/CameraPositionViewWx.cpp (NCameraPositionView::Create)",
    "CFormationWindow": "ED_B2_M1/FormationViewWx.cpp (NFormationView::Create)",
    "CUnitStartCmdWindow": "ED_B2_M1/UnitStartCmdViewWx.cpp (NUnitStartCmdView::Create)",
    "CReinfPointsWindow": "ED_B2_M1/ReinfPointsViewWx.cpp (NReinfPointsView::Create)",
    "CAIGeneralPointsWindow": "ED_B2_M1/AIGeneralViewWx.cpp (NAIGeneralView::Create)",
    "CFieldWindow": "ED_B2_M1/FieldViewWx.cpp (NFieldView::Create)",
    "CHeightWindowV3": "ED_B2_M1/HeightViewV3Wx.cpp (NHeightViewV3::CreateWx)",
    "CMapObjectWindow": "ED_B2_M1/MapObjectViewWx.cpp (NMapObjectView::CreateWx)",
    "CVSOWindow": "ED_B2_M1/VSOViewWx.cpp (NVSOView::CreateWx)",
    "CScriptAreaWindow": "ED_B2_M1/ScriptAreaViewWx.cpp (NScriptAreaView::CreateWx)",
}


def read(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError:
        return ""


def walk(module):
    base = os.path.join(SOURCES, module)
    for dirpath, dirnames, filenames in os.walk(base):
        for name in filenames:
            if name.endswith((".cpp", ".h")):
                yield os.path.join(dirpath, name)


def bar(done, total, width=22):
    if total == 0:
        return "-" * width + "   n/a"
    filled = int(round(width * done / float(total)))
    return "#" * filled + "." * (width - filled) + "  %d/%d" % (done, total)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--detail", action="store_true", help="list the items behind the counts")
    args = ap.parse_args()

    per_module = {}
    dialogs = []          # (module, class name)
    controls = 0
    rc_templates = 0
    files_total = 0
    files_gui = 0

    for module in MODULES:
        m_files = m_gui = m_ctrl = 0
        m_mfc = m_sec = False
        for path in walk(module):
            src = read(path)
            m_files += 1
            files_total += 1
            if GUI_MFC.search(src):
                m_gui += 1
                files_gui += 1
                m_mfc = True
            if STINGRAY.search(src):
                m_sec = True
            if path.endswith(".h"):
                for name in DIALOG_CLASS.findall(src):
                    # CResizeDialog is the base every other one derives from,
                    # not a dialog anybody migrates.
                    if name != "CResizeDialog":
                        dialogs.append((module, name))
            n = len(CONTROL_MEMBER.findall(src))
            m_ctrl += n
            controls += n
        # .rc templates belong to the module whose directory holds them
        for dirpath, dirnames, filenames in os.walk(os.path.join(SOURCES, module)):
            for name in filenames:
                if name.endswith(".rc"):
                    rc_templates += len(RC_DIALOG.findall(read(os.path.join(dirpath, name))))
        per_module[module] = dict(files=m_files, gui=m_gui, ctrl=m_ctrl,
                                  mfc=m_mfc, sec=m_sec)

    migrated_dialogs = [d for d in dialogs if d[1] in MIGRATED]

    print("wx migration progress\n")

    print("MODULE  -- targets still needing MFC or Stingray")
    print("  %-14s %-5s %-5s %s" % ("module", "MFC", "SEC", "files with GUI MFC"))
    clean = 0
    for module in MODULES:
        d = per_module[module]
        if not d["mfc"] and not d["sec"]:
            clean += 1
        print("  %-14s %-5s %-5s %d of %d" % (
            module, "yes" if d["mfc"] else "-", "yes" if d["sec"] else "-",
            d["gui"], d["files"]))
    print("  %s\n" % bar(clean, len(MODULES)))

    print("FILE    -- translation units naming MFC/ATL/Win32 GUI types")
    print("  %s\n" % bar(files_total - files_gui, files_total))

    print("DIALOG  -- dialog classes with a wx implementation behind a neutral boundary")
    print("  %s" % bar(len(migrated_dialogs), len(dialogs)))
    print("  .rc templates still to convert: %d\n" % rc_templates)

    print("CONTROL -- MFC control instances declared as members")
    print("  %s\n" % bar(0, controls))

    # Panels are not dialog classes and never appear in the dialog count -- the
    # Log Window's contents derive from CScintillaEditorWindow. Listed here so
    # the scoreboard does not understate what has actually moved.
    print("MIGRATED so far")
    for name in sorted(MIGRATED):
        print("  %-30s %s" % (name, MIGRATED[name]))
    print()

    if args.detail:
        print("migrated:")
        for module, name in sorted(migrated_dialogs):
            print("  %-34s %s" % (name, MIGRATED[name]))
        print("\nremaining dialog classes:")
        for module, name in sorted(d for d in dialogs if d[1] not in MIGRATED):
            print("  %-14s %s" % (module, name))


if __name__ == "__main__":
    sys.exit(main())
