#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex.htm

struct SECPTNStrategy {

};

struct SECPersistentTreeNode {

};

struct SECPTNFactory {

};

class SECWorkspaceManagerEx : public CObject {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__secworkspacemanagerex.htm
    // Construction/Initialization
    SECWorkspaceManagerEx();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__setregistrymode.htm
    // Set the workspace manager to registry mode
    virtual void SetRegistryMode(const CString& strAppSubKey,BOOL bEnable=TRUE, HKEY hKeyApp=HKEY_CURRENT_USER, BOOL bSectionKey=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__setregistrymode.htm
    // Set the workspace manager to registry mode
    virtual void SetRegistryMode(BOOL bEnable, BOOL bSectionKey = FALSE);

    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__closeworkspace.htm
    // Close the currently active workspace.
    virtual BOOL CloseWorkspace();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__saveworkspace.htm
    // Save the currently active workspace state.
    virtual BOOL SaveWorkspace();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__saveworkspace.htm
    // Save the currently active workspace state under a new name.
    virtual BOOL SaveWorkspace(CString strWorkspaceName);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__openworkspace.htm
    virtual BOOL OpenWorkspace(BOOL bLastActiveWorkspace=FALSE, BOOL bSuppressErrorDlg=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__openworkspace.htm
    // Load the workspace state of the workspace name passed in.
    virtual BOOL OpenWorkspace(CString strWorkspaceName, BOOL bSuppressErrorDlg=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__getactiveworkspace.htm
    // Get the name of the currently active workspace.
    BOOL GetActiveWorkspace(CString& strWorkspaceName);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__setmaxworkspacemrusize.htm
    // Set the maximum number of workspaces in the MRU list
    void SetMaxWorkspaceMRUSize(UINT nMaxSize);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__getmaxworkspacemrusize.htm
    // Get the maximum number of workspaces in the MRU list
    int GetMaxWorkspaceMRUSize();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__setworkspacefileextension.htm
    // Set the workspace default extension
    void SetWorkspaceFileExtension(const CString& strExt);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__getworkspacefileextension.htm
    // Get the workspace default extension
    void GetWorkspaceFileExtension(CString& strExt);

    // Overridable
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__getpersistancestrategy.htm
    // Get the persistance strategy (file, registry, etc.)
    virtual SECPTNStrategy* GetPersistanceStrategy(const CString& strWorkspace);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__openworkspacedockstate.htm
    // Load the controlbar dockstate for current OpenWorkspace operation.
    virtual BOOL OpenWorkspaceDockState(SECPersistentTreeNode* pRoot);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__openworkspaceframewnds.htm
    // Load the frame window state for current OpenWorkspace operation.
    virtual BOOL OpenWorkspaceFrameWnds(SECPersistentTreeNode* pRoot, CFrameWnd** ppActiveChildFrame);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__saveworkspacedockstate.htm
    // Save the controlbar dockstate for current SaveWorkspace operation.
    virtual BOOL SaveWorkspaceDockState(SECPTNFactory& nodeFactory, SECPersistentTreeNode* pRoot);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__saveworkspaceframewnds.htm
    // Save the frame window state for current SaveWorkspace operation.
    virtual BOOL SaveWorkspaceFrameWnds(SECPTNFactory& nodeFactory, SECPersistentTreeNode* pRoot);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__promptworkspacename.htm
    // Display prompt dialog for workspace Open or SaveAs
    virtual BOOL PromptWorkspaceName(CString& strWorkspaceName,BOOL bSaveAs);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__openworkspacecustomdata.htm
    // Overridable to load your own custom workspace data.
    virtual void OpenWorkspaceCustomData(SECPersistentTreeNode* pRoot, const CString& strWorkspaceName);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__saveworkspacecustomdata.htm
    // Overridable to load your own custom workspace data.
    virtual void SaveWorkspaceCustomData(SECPTNFactory& nodeFactory, SECPersistentTreeNode* pRoot, const CString& strWorkspaceName);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__closeallchildframes.htm
    // Close all the child frame windows.
    virtual BOOL CloseAllChildFrames();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__closecontrolbars.htm
    // Close all the controlbars.
    virtual BOOL CloseControlBars(CPtrList* pListBars,CFrameWnd* pFrame);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__openadditionalviewperframe.htm
    // Override to load multiple views per single frame window.
    virtual BOOL OpenAdditionalViewPerFrame(const CDocument* pDoc, const CFrameWnd* pFrame, const SECPersistentTreeNode* pAddViewNode);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkspacemanagerex__saveadditionalviewperframe.htm
    // Override to save multiple views per single frame window.
    virtual SECPersistentTreeNode* SaveAdditionalViewPerFrame( CDocument* pDoc, CView* pView, CFrameWnd* pFrame, SECPTNFactory& nodeFactory, SECPersistentTreeNode* pParent, int nMultViewFrameUniqueID);
};
