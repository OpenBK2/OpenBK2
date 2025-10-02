#pragma once

#include "InterfaceScreenBase.h"
#include "../Misc/HashFuncs.h"

namespace NDb
{
	struct SUnitBaseRPGStats;
}

class CInterfaceEncyclopedia : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceEncyclopedia );

	enum EFilterCountries
	{
		EFC_ALL,
		EFC_US,
		EFC_UK,
		EFC_USSR,
		EFC_GERMANY,
		EFC_JAPAN,
		
		EFC_COUNT,
	};
	
	enum EFilterUnitTypes
	{
		EFUT_ALL,
		EFUT_ARTILLERY,
		EFUT_ARMOR,
		EFUT_AIR,
		EFUT_SEA,
		EFUT_TRANSPORT,
		EFUT_MISC,
		
		EFUT_COUNT,
	};
	
	struct SFilterBtn
	{
		ZDATA
		CPtr<IButton> pBtn;
		string szBtnName;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBtn); f.Add(3,&szBtnName); return 0; }
	};

public:	
	class CUnitData : public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( CUnitData );
	public:
		ZDATA
		CDBPtr<NDb::SUnitBaseRPGStats> pStats;
		wstring wszBriefDesc;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pStats); f.Add(3,&wszBriefDesc); return 0; }
		
		bool operator==( const CUnitData &data ) const
		{
			return pStats == data.pStats;
		}
	};

	class CDataViewer : public IDataViewer
	{
		OBJECT_NOCOPY_METHODS( CDataViewer );
	public:
		void MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const;
	};
private:
	
	struct SSelectedUnit
	{
		ZDATA
		CPtr<CUnitData> pData;
		EFilterCountries eCountry;
		EFilterUnitTypes eUnitType;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pData); f.Add(3,&eCountry); f.Add(4,&eUnitType); return 0; }
	};
	
	struct SFilterCache
	{
		ZDATA
		CPtr<IListControl> pUnitListCont;
		vector< CPtr<CUnitData> > filteredUnits;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnitListCont); f.Add(3,&filteredUnits); return 0; }
	};
	
	struct SDataSorter
	{
		bool operator()( const CUnitData *pData1, const CUnitData *pData2 ) const;
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<ITextView> pUnitNameView;
	CPtr<IScrollableContainer> pUnitDescCont;
	CPtr<ITextView> pUnitDescView;
	CPtr<IWindow3DControl> p3DCtrl;
	CPtr<IButton> pStepBackward;
	CPtr<IButton> pStepForward;
	
	vector< SFilterBtn > filterCountries;
	vector< SFilterBtn > filterUnitTypes;
	EFilterCountries eSelCountry;
	EFilterUnitTypes eSelUnitType;
	
	hash_map< CDBPtr<NDb::SUnitBaseRPGStats>, CPtr<CUnitData>, SDBPtrHash > knownUnits;
	vector< SSelectedUnit > selectedUnits; // array for undo purpose
	int nSelUnit;
	
	CObj<CDataViewer> pDataViewer;
	
	vector<SFilterCache> filterCache;
	
	ZSKIP //CPtr<IWindow> pBackground;
	
	bool bEffectEnter;
	int nEffectCounter;
	bool bEffectBorder;
	int nLoading;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pUnitNameView); f.Add(3,&pUnitDescCont); f.Add(4,&pUnitDescView); f.Add(5,&p3DCtrl); f.Add(6,&pStepBackward); f.Add(7,&pStepForward); f.Add(8,&filterCountries); f.Add(9,&filterUnitTypes); f.Add(10,&eSelCountry); f.Add(11,&eSelUnitType); f.Add(12,&knownUnits); f.Add(13,&selectedUnits); f.Add(14,&nSelUnit); f.Add(15,&pDataViewer); f.Add(16,&filterCache); f.Add(18,&bEffectEnter); f.Add(19,&nEffectCounter); f.Add(20,&bEffectBorder); f.Add(21,&nLoading); return 0; }
private:
	void MsgBack( const SGameMessage &msg );
	void MsgStepBackward( const SGameMessage &msg );
	void MsgStepForward( const SGameMessage &msg );
	void MsgUnitSelectionChanged( const SGameMessage &msg );
	void MsgHelpScreen( const SGameMessage &msg );
	
	bool OnFilterBtn( const string &szName );
	bool OnEffectFinished( const string &szName );
	
	void MakeInterior();
	
	void MakeFilterCache( IWindow *pSample );
	void UpdateInfo();
	void UpdateFilters();
	void UpdateSelectedUnitInfo();
	
	void AddSelectedUnit( CUnitData *pData, EFilterCountries eCountry, EFilterUnitTypes eUnitType );
	void SetFiltersBySelection();
	void UpdateListCtrlSelection();
	
	CUnitData* GetUnitData( const NDb::SUnitBaseRPGStats *pStats );
	
	void EffectStart( bool bEnter );
	void EffectFinish();
	void EffectBorderStart( bool bEnter );
	void EffectBorderFinish();
public:
	CInterfaceEncyclopedia();
	~CInterfaceEncyclopedia();

	bool Init();

	bool StepLocal( bool bAppActive );
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
public:
	static bool RunWithUnit( const NDb::SUnitBaseRPGStats *pStats );
};

class CInterfaceEncyclopediaWait : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceEncyclopediaWait );
	
	ZDATA_(CInterfaceScreenBase)
	int nLoading;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&nLoading); return 0; }
public:
	CInterfaceEncyclopediaWait();

	bool Init();
	
	bool StepLocal( bool bAppActive );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

class CICEncyclopedia : public CInterfaceCommandBase<CInterfaceEncyclopedia>
{
	OBJECT_BASIC_METHODS( CICEncyclopedia );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

class CICEncyclopediaWait : public CInterfaceCommandBase<CInterfaceEncyclopediaWait>
{
	OBJECT_BASIC_METHODS( CICEncyclopediaWait );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

