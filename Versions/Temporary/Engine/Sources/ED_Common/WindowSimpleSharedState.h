#if !defined(__WINDOW_SIMPLE_SHARED_STATE__)
#define __WINDOW_SIMPLE_SHARED_STATE__
#pragma once

#include "..\MapEditorLib\DefaultInputState.h"
interface IWindow;
namespace NDb
{
	struct SUIDesc;
}
class CWindowSimpleSharedEditor;
class CVariant;

class CWindowSimpleSharedState : public CDefaultInputState
{
private:
	// flags for ReplaceChild method
	enum {
		RCH_DELTA = 0x0001,		// delta move
		RCH_X     = 0x0002,		// change x only
		RCH_Y     = 0x0004,   // change y only
	};
	//methods
	void InsertChild( const CTPoint<int> &rMousePoint );
	void RemoveChild( IWindow *pWindow );
	void LoadWindow();
	void ReplaceChild( IWindow *pWindow, const CTPoint<int> &rMousePoint, int nFlags );
	bool CheckInsertChild( const string & szTypeName, const CDBID &rDBID );
	
	bool InsertChildInstanceToDB( const string & szSharedTypeName, const CDBID &rSharedDBID, CDBID *pInstanceDBID );
	IWindow * InsertChildInstanceToUI( const string & szSharedTypeName, const CDBID &rDBID );
	bool GenerateChildInstance( const string &szTypeName, const CDBID &rDBID, string *szInstanceFullName, CDBID *pInstanceDBID );
	bool MakeInstanceName( const string & szInstanceTypeName, const string & szSharedShortName, string *pShortName, string *pFullName, string *pObjName );
	bool MakeSharedName( const string & szSharedTypeName, const CDBID &rDBID, string *pSharedShortName, string *pSharedFullName );
	bool GetEditorObjName( string *pObjName );
	void UpdatePropertyControl( bool bHardUpdate = false );
	void ResetSelection();

	void OnKeyDelete();
	void OnKeyEnter();
	void OnKeyBack();
	void OnKeyTab();
	void OnKeyArrows( int dx, int dy );

	static bool FindInstanceTypeNameByShared( const string & szSharedName, string *szInstanceName );
	static bool IsPushableType( const string & szTypeName );

	void MakeUIScreenWithElement( const NDb::SUIDesc *pElement );
public:
	//Life-cycle
	CWindowSimpleSharedState( CWindowSimpleSharedEditor *_pEditor );

	//IInputState
	void Enter();
	void Leave();
	void PostDraw( class CPaintDC *pPaintDC );
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnMouseMove	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp	( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );

	// methods
	void UndoChange( const string & szTypeName, const CDBID &rDBID, const string & szName, const CVariant & oldValue );
	void UndoInsert( const string & szTypeName, const CDBID &rDBID, const string & szName );
	void UndoRemove( const string & szTypeName, const CDBID &rDBID, const string & szName );

	// members
protected:
	class CWindowSimpleSharedEditor *pEditor;
	string szEditorTypeName;
	CDBID editorDBID;
	CPtr<IWindow> pScreen;
	CPtr<IWindow> pMainWindow;
	CPtr<IWindow> pPickedWindow;
	CTPoint<int> rLastPoint;
	CTPoint<int> rStartPoint;
	bool bDragging;
};

#endif // !defined(__WINDOW_SIMPLE_SHARED_STATE__)


