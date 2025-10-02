
#pragma once
#include "resource.h"
#include "MapEditorLib/ResizeDialog.h"
#include "ELK_Types.h"


class CStatisticDialog : public CResizeDialog
{
protected:
	enum IMAGES
	{
		IMAGE_ROOT_NOT_TRANSLATED								= 0,
		IMAGE_ROOT_OUTDATED											= 1,
		IMAGE_ROOT_TRANSLATED										= 2,
		IMAGE_ROOT_APPROVED											= 3,
		IMAGE_ROOT_NOT_TRANSLATED_EXPANDED			= 4,
		IMAGE_ROOT_OUTDATED_EXPANDED						= 5,
		IMAGE_ROOT_TRANSLATED_EXPANDED					= 6,
		IMAGE_ROOT_APPROVED_EXPANDED						= 7,

		IMAGE_FOLDER_NOT_TRANSLATED							= 8,
		IMAGE_FOLDER_OUTDATED										= 9,
		IMAGE_FOLDER_TRANSLATED									= 10,
		IMAGE_FOLDER_APPROVED										= 11,
		IMAGE_FOLDER_NOT_TRANSLATED_EXPANDED		= 12,
		IMAGE_FOLDER_OUTDATED_EXPANDED					= 13,
		IMAGE_FOLDER_TRANSLATED_EXPANDED				= 14,
		IMAGE_FOLDER_APPROVED_EXPANDED					= 15,

		IMAGE_TEXT_NOT_TRANSLATED								= 16,
		IMAGE_TEXT_OUTDATED											= 17,
		IMAGE_TEXT_TRANSLATED										= 18,
		IMAGE_TEXT_APPROVED											= 19,
		IMAGE_TEXT_NOT_TRANSLATED_EXPANDED			= 20,
		IMAGE_TEXT_OUTDATED_EXPANDED						= 21,
		IMAGE_TEXT_TRANSLATED_EXPANDED					= 22,
		IMAGE_TEXT_APPROVED_EXPANDED						= 23,

		IMAGE_COUNT															= 24,
	};
public:
	CELK *pELK;

	CStatisticDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_STATISTIC };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	CImageList imageListNormal;
	SECTreeCtrl wndTree;
	
	const static int vID[];

	virtual int GetMinimumXDimension() { return 150; }
	virtual int GetMinimumYDimension() { return 100; }
	virtual bool SerializeToRegistry() { return true; }
	virtual void GetRegistryKey( string *pszRegistryKey );
	virtual bool IsDrawGripper() { return true; }
	
	void InitImageList();
	void CreateControls();
	void FillTree();
	
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
};



