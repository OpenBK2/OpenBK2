#pragma once
#include "window.h"
#include "System/FreeIDs.h"

struct SWindowScrollBar;

class CWindowScrollBar;
class CWindowSimple;

struct IScrollableContrainerSorter
{

};

class CWindowScrollableContainerBase : public CWindow, public IScrollableContainer, public ISliderNotify, public IClickNotify
{
	//{dynamic data
	struct SElement
	{
		string szName;
		string szNegativeSelection;
		bool bSelectable;
		int nPos;																// pChild's position inside control
		SElement() {  }
		SElement( CWindow *_pChild, const int _bSelectable, const int _nPos, CWindow *pNegativeSelecion )
			: szName( _pChild->GetName() ), bSelectable( _bSelectable ), nPos( _nPos ) 
		{ 
			if ( pNegativeSelecion )
				szNegativeSelection = pNegativeSelecion->GetName();
		}
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &szName );
			saver.Add( 2, &bSelectable );
			saver.Add( 3, &nPos );
			saver.Add( 4, &szNegativeSelection );
			return 0;
		}
	};
	//CRAP{ may be ineficient, it's easy to optimize thou
	struct SSortPred
	{
		CPtr<IWindowSorter> pSorter;
		CWindow *pContainer;
		SSortPred( IWindowSorter *_pSorter, CWindow *_pContainer ) : pSorter( _pSorter ), pContainer( _pContainer ) {  }
		bool operator()( const SElement &e1, const SElement &e2 ) const
		{
			return pSorter->Compare( pContainer->GetChild( e1.szName, false ), pContainer->GetChild( e2.szName, false ) );
		}
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &pSorter );
			return 0;
		}
	};
	//CRAP}

	CObj<IWindow> pSelected;
	CObj<IWindow> pPreSelected;
	CObj<CWindow> pSelection;
	CObj<CWindow> pPreSelection;					// shows under mouse cursor

	CDBPtr<NDb::SWindowScrollableContainerBaseShared> pShared;
	CFreeIds elementIDs;
	int nPosSize;															// total size of elements
	CObj<CWindowScrollBar> pScrollBar;	
	CObj<CWindowSimple> pBorder;  // for clipping
	CObj<CWindowSimple> pContainer;  // contains elements
	typedef vector<SElement> CElements;
	CElements elements;
	//}

	void UpdateItemCoordinates( CElements::iterator from );
	void UpdateScrollBar();
	CElements::iterator GetBefore( CElements::iterator after );
	CElements::iterator GetAfter( IWindow *pElement );

	void AddElement( CWindow *pElement, const bool _bSelectable, const int _nPosSize, CWindow *pNegativeSelection );
	void RemoveElement( CWindow *pElement );
	void UpdateSelectionPosition( IWindow *pSelection, IWindow *pSelected );
	void SelectWithSelection( IWindow *pElement, IWindow * _pSelection );
	void PreSelect( IWindow *pElement );
protected:
	CWindow* GetElement( const string &szName );
	void Resort( IWindowSorter *pSorter );
	void RemoveItems();
	bool IsHorisontal() const;
public:
	CWindowScrollableContainerBase() : nPosSize( 0 ) {  }
	virtual void SliderPosition( const float fPosition, class CWindow *pWho );

	void Segment();
	virtual void InsertAfter( IWindow *pElement, IWindow *pInsert, const bool bSelectable );
	virtual void Remove( IWindow *pRemove );
	virtual void PushBack( IWindow *pElement, const bool bSelectable );
	// call it after remove or insertafterr
	virtual void Update();
	virtual void Reposition( const CTRect<float> &parentRect );
	virtual void Init();
	virtual void Select( IWindow *pElement );
	void AfterLoad();
	bool OnMouseMove( const CVec2 &vPos, const int nButton );

	
	virtual int operator&( IBinSaver &saver );
	virtual void Visit( struct IUIVisitor *pVisitor );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	
	virtual void Clicked( struct IWindow *pWho, int nButton );
	void DoubleClicked( struct IWindow *pWho, int nButton );
	virtual IWindow *GetSelectedItem() const { return pSelected; }
	virtual IWindow *GetItem( const string &szName );
	virtual IWindow *GetItem( const int nItem );
	virtual int GetItemNumber( IWindow *pElement );
	virtual void EnsureElementVisible( IWindow *pElement );

	int GetBaseHeight() const;

	void ResetScroller();
	void SetDiscreteScroll( int nVisibleSlots );
};

