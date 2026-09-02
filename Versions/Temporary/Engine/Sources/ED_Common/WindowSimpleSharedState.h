#pragma once

#include "MapEditorLib/DefaultInputState.h"
struct IWindow;
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
	bool CheckInsertChild( const std::string & szTypeName, const CDBID &rDBID );
	
	bool InsertChildInstanceToDB( const std::string & szSharedTypeName, const CDBID &rSharedDBID, CDBID *pInstanceDBID );
	IWindow * InsertChildInstanceToUI( const std::string & szSharedTypeName, const CDBID &rDBID );
	bool GenerateChildInstance( const std::string &szTypeName, const CDBID &rDBID, std::string *szInstanceFullName, CDBID *pInstanceDBID );
	bool MakeInstanceName( const std::string & szInstanceTypeName, const std::string & szSharedShortName, std::string *pShortName, std::string *pFullName, std::string *pObjName );
	bool MakeSharedName( const std::string & szSharedTypeName, const CDBID &rDBID, std::string *pSharedShortName, std::string *pSharedFullName );
	bool GetEditorObjName( std::string *pObjName );
	void UpdatePropertyControl( bool bHardUpdate = false );
	void ResetSelection();

	void OnKeyDelete();
	void OnKeyEnter();
	void OnKeyBack();
	void OnKeyTab();
	void OnKeyArrows( int dx, int dy );

	static bool FindInstanceTypeNameByShared( const std::string & szSharedName, std::string *szInstanceName );
	static bool IsPushableType( const std::string & szTypeName );

	void MakeUIScreenWithElement( const NDb::SUIDesc *pElement );
public:
	//Life-cycle
	CWindowSimpleSharedState( CWindowSimpleSharedEditor *_pEditor );

	//IInputState
	void Enter();
	void Leave();
	void PostDraw( class CPaintDC *pPaintDC );
	void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	void OnMouseMove	( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonUp	( unsigned nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint );

	// methods
	void UndoChange( const std::string & szTypeName, const CDBID &rDBID, const std::string & szName, const CVariant & oldValue );
	void UndoInsert( const std::string & szTypeName, const CDBID &rDBID, const std::string & szName );
	void UndoRemove( const std::string & szTypeName, const CDBID &rDBID, const std::string & szName );

	// members
protected:
	class CWindowSimpleSharedEditor *pEditor;
	std::string szEditorTypeName;
	CDBID editorDBID;
	CPtr<IWindow> pScreen;
	CPtr<IWindow> pMainWindow;
	CPtr<IWindow> pPickedWindow;
	CTPoint<int> rLastPoint;
	CTPoint<int> rStartPoint;
	bool bDragging;
};



