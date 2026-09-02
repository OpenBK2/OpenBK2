#include "Toolkit/swsmgrex.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECWorkspaceManagerEx::SECWorkspaceManagerEx() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECWorkspaceManagerEx::SetRegistryMode(const CString& strAppSubKey, BOOL bEnable, HKEY hKeyApp, BOOL bSectionKey) {
    spdlog::debug("{} this={} strAppSubKey={} bEnable={} hKeyApp={} bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
        strAppSubKey.GetString(), bEnable, spdlog::fmt_lib::ptr(hKeyApp), bSectionKey);
}

void SECWorkspaceManagerEx::SetRegistryMode(BOOL bEnable, BOOL bSectionKey) {
    spdlog::debug("{} this={} bEnable={} bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bSectionKey);
}

BOOL SECWorkspaceManagerEx::CloseWorkspace() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::SaveWorkspace() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::SaveWorkspace(CString strWorkspaceName) {
    spdlog::debug("{} this={} strWorkspaceName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strWorkspaceName.GetString());
    return FALSE;
}

BOOL SECWorkspaceManagerEx::OpenWorkspace(BOOL bLastActiveWorkspace, BOOL bSuppressErrorDlg) {
    spdlog::debug("{} this={} bLastActiveWorkspace={} bSuppressErrorDlg={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bLastActiveWorkspace, bSuppressErrorDlg);
    return FALSE;
}

BOOL SECWorkspaceManagerEx::OpenWorkspace(CString strWorkspaceName, BOOL bSuppressErrorDlg) {
    spdlog::debug("{} this={} strWorkspaceName={} bSuppressErrorDlg={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strWorkspaceName.GetString(), bSuppressErrorDlg);
    return FALSE;
}

BOOL SECWorkspaceManagerEx::GetActiveWorkspace(CString& strWorkspaceName) {
    spdlog::debug("{} this={} strWorkspaceName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strWorkspaceName.GetString());
    return FALSE;
}

void SECWorkspaceManagerEx::SetMaxWorkspaceMRUSize(UINT nMaxSize) {
    spdlog::debug("{} this={} nMaxSize={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nMaxSize);
}

int SECWorkspaceManagerEx::GetMaxWorkspaceMRUSize() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECWorkspaceManagerEx::SetWorkspaceFileExtension(const CString& strExt) {
    spdlog::debug("{} this={} strExt={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strExt.GetString());
}

void SECWorkspaceManagerEx::GetWorkspaceFileExtension(CString& strExt) {
    spdlog::debug("{} this={} strExt={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strExt.GetString());
}

SECPTNStrategy* SECWorkspaceManagerEx::GetPersistanceStrategy(const CString& strWorkspace) {
    spdlog::debug("{} this={} strWorkspace={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strWorkspace.GetString());
    return nullptr;
}

BOOL SECWorkspaceManagerEx::OpenWorkspaceDockState(SECPersistentTreeNode* pRoot) {
    spdlog::debug("{} this={} pRoot={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pRoot));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::OpenWorkspaceFrameWnds(SECPersistentTreeNode* pRoot, CFrameWnd** ppActiveChildFrame) {
    spdlog::debug("{} this={} pRoot={} ppActiveChildFrame={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pRoot), spdlog::fmt_lib::ptr(ppActiveChildFrame));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::SaveWorkspaceDockState(SECPTNFactory& nodeFactory, SECPersistentTreeNode* pRoot) {
    spdlog::debug("{} this={} pRoot={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pRoot));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::SaveWorkspaceFrameWnds(SECPTNFactory& nodeFactory, SECPersistentTreeNode* pRoot) {
    spdlog::debug("{} this={} pRoot={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pRoot));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::PromptWorkspaceName(CString& strWorkspaceName,BOOL bSaveAs) {
    spdlog::debug("{} this={} strWorkspaceName={} bSaveAs={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strWorkspaceName.GetString(), bSaveAs);
    return FALSE;
}

void SECWorkspaceManagerEx::OpenWorkspaceCustomData(SECPersistentTreeNode* pRoot, const CString& strWorkspaceName) {
    spdlog::debug("{} this={} pRoot={} strWorkspaceName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pRoot), strWorkspaceName.GetString());
}

void SECWorkspaceManagerEx::SaveWorkspaceCustomData(SECPTNFactory& nodeFactory, SECPersistentTreeNode* pRoot, const CString& strWorkspaceName) {
    spdlog::debug("{} this={} pRoot={} strWorkspaceName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pRoot), strWorkspaceName.GetString());
}

BOOL SECWorkspaceManagerEx::CloseAllChildFrames() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECWorkspaceManagerEx::CloseControlBars(CPtrList* pListBars,CFrameWnd* pFrame) {
    spdlog::debug("{} this={} pListBars={} pFrame={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pListBars), spdlog::fmt_lib::ptr(pFrame));

    return FALSE;
}

BOOL SECWorkspaceManagerEx::OpenAdditionalViewPerFrame(const CDocument* pDoc, const CFrameWnd* pFrame, const SECPersistentTreeNode* pAddViewNode) {
    spdlog::debug("{} this={} pDoc={} pFrame={} pAddViewNode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDoc), spdlog::fmt_lib::ptr(pFrame), spdlog::fmt_lib::ptr(pAddViewNode));
    return FALSE;
}
SECPersistentTreeNode* SECWorkspaceManagerEx::SaveAdditionalViewPerFrame( CDocument* pDoc, CView* pView, CFrameWnd* pFrame, SECPTNFactory& nodeFactory, SECPersistentTreeNode* pParent, int nMultViewFrameUniqueID) {
    spdlog::debug("{} this={} pDoc={} pView={} pFrame={} pParent={} nMultViewFrameUniqueID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDoc), spdlog::fmt_lib::ptr(pView), spdlog::fmt_lib::ptr(pFrame), spdlog::fmt_lib::ptr(pParent), nMultViewFrameUniqueID);
    return nullptr;
}
