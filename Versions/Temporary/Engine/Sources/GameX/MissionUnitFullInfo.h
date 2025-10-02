#pragma once

#include "Stats_B2_M1/RPGStats.h"
#include "UISpecificB2/UISpecificB2.h"
#include "UnitFullInfoHelper.h"

struct SObjectStatus;
class CMapObj;
namespace NDb
{
	struct SAnimB2;
};

class CMissionUnitFullInfo : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CMissionUnitFullInfo );
	
	enum EType
	{
		ET_OBJECT,
		ET_REINF,
		ET_SUPER_WEAPON,
	};
	
	struct SWeaponItem
	{
		ZDATA
		CPtr<IWindow> pItem;
		ZSKIP //CPtr<ITextView> pName;
		CPtr<ITextView> pDamage;
		CPtr<ITextView> pPenetration;
		CPtr<ITextView> pAmmo;
		CPtr<ITextView> pCount;
		CPtr<IButton> pIconBtn;
		CPtr<IWindow> pIcon;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pItem); f.Add(4,&pDamage); f.Add(5,&pPenetration); f.Add(6,&pAmmo); f.Add(7,&pCount); f.Add(8,&pIconBtn); f.Add(9,&pIcon); return 0; }
	};
	
	struct SMember
	{
		ZDATA
		CDBPtr<NDb::SHPObjectRPGStats> pStats;
		CPtr<CMapObj> pMO;
		int nPassengerCount;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pStats); f.Add(3,&pMO); f.Add(4,&nPassengerCount); return 0; }
		
		bool operator==( const SMember &member ) const
		{
			return pStats == member.pStats && pMO == member.pMO;
		}
	};
	
	struct SViewMember
	{
		ZDATA
		CPtr<IButton> pBtn;
		CPtr<IWindow> pIconWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBtn); f.Add(3,&pIconWnd); return 0; }
	};
	
	struct SHP
	{
		ZDATA
		float fFraction;
		int nHP;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&fFraction); f.Add(3,&nHP); return 0; }
	};
	
	ZDATA
	CPtr<ITextView> pName;
	
	ZSKIP //CPtr<IWindow> pArmorMask;
	ZSKIP //CPtr<ITextView> pArmorFront;
	ZSKIP //CPtr<ITextView> pArmorBack;
	ZSKIP //CPtr<ITextView> pArmorSide;
	ZSKIP //CPtr<ITextView> pArmorTop;
	ZSKIP //CPtr<IWindow> pHPUnit;
	ZSKIP //CPtr<IWindow> pHPSquad;
	ZSKIP //CPtr<IWindow> pHPView;
	
	vector<SWeaponItem> weaponItems;
	ZSKIP //SSupplyItem supplyCalls;
	ZSKIP //SSupplyItem supplyCount;
	ZSKIP //SSupplyItem supplyTime;
	
	ZSKIP //vector< CPtr<IWindow> > memberButtons;
	
	vector<int> armors;
	vector<NUnitFullInfo::SWeapon> weapons;
	
	EType eType;
	CPtr<IWindow> pWeaponsWnd;
	CPtr<IWindow> pSuppliesWnd;
	CPtr<IWindow> pMembersWnd;
	CPtr<IWindow> pFlagWnd;
	CPtr<IWindow3DControl> p3DCtrl;
	CPtr<ITextView> pWeaponFullName;
	CPtr<ITextView> pWeaponFullDamage;
	CPtr<ITextView> pWeaponFullPenetration;
	CPtr<ITextView> pWeaponFullAmmo;
	vector<IWindow3DControl::SObject> objects;
	NDb::ESeason eSeason;
	
	CDBPtr<NDb::SHPObjectRPGStats> pBaseStats;
	CPtr<CMapObj> pBaseMO;
	CDBPtr<NDb::SHPObjectRPGStats> pSelStats;
	CPtr<CMapObj> pSelMO;
	int nSelWeapon;
	int nSelMember;
	int nCurSupplies;
	int nMaxSupplies;
	CDBPtr<NDb::STexture> pKeyObjectTexture;

	CPtr<IWindow> pArmorsWnd;
	CPtr<ITextView> pArmorFrontView;
	CPtr<ITextView> pArmorSide1View;
	CPtr<ITextView> pArmorSide2View;
	CPtr<ITextView> pArmorBackView;
	CPtr<ITextView> pArmorTopView;
	
	CPtr<ITextView> pSuppliesCountView;
	wstring wszLocalizedName;
	
	vector<SMember> members;
	vector<SViewMember> viewMembers;
	ZSKIP //vector<SHP> hps;
	CPtr<ITextView> pHPView;
	ZSKIP //CPtr<IMultiTextureProgressBar> pHPBarUnit;
	CPtr<ITextView> pSuppliesCountInfinite;
	CPtr<IWindow> pHitbarPanel;

	vector<SHP> hps;
	
	CPtr<IWindow> pFuelPanel;
	CPtr<IProgressBar> pFuelBar;
	float fFuel;
	CPtr<IProgressBar> pHPBarUnit;
	int nPlayer;
	
	wstring wszTooltipMemberInTransport;
	wstring wszTooltipMemberInBuilding;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pName); f.Add(11,&weaponItems); f.Add(16,&armors); f.Add(17,&weapons); f.Add(18,&eType); f.Add(19,&pWeaponsWnd); f.Add(20,&pSuppliesWnd); f.Add(21,&pMembersWnd); f.Add(22,&pFlagWnd); f.Add(23,&p3DCtrl); f.Add(24,&pWeaponFullName); f.Add(25,&pWeaponFullDamage); f.Add(26,&pWeaponFullPenetration); f.Add(27,&pWeaponFullAmmo); f.Add(28,&objects); f.Add(29,&eSeason); f.Add(30,&pBaseStats); f.Add(31,&pBaseMO); f.Add(32,&pSelStats); f.Add(33,&pSelMO); f.Add(34,&nSelWeapon); f.Add(35,&nSelMember); f.Add(36,&nCurSupplies); f.Add(37,&nMaxSupplies); f.Add(38,&pKeyObjectTexture); f.Add(39,&pArmorsWnd); f.Add(40,&pArmorFrontView); f.Add(41,&pArmorSide1View); f.Add(42,&pArmorSide2View); f.Add(43,&pArmorBackView); f.Add(44,&pArmorTopView); f.Add(45,&pSuppliesCountView); f.Add(46,&wszLocalizedName); f.Add(47,&members); f.Add(48,&viewMembers); f.Add(50,&pHPView); f.Add(52,&pSuppliesCountInfinite); f.Add(53,&pHitbarPanel); f.Add(54,&hps); f.Add(55,&pFuelPanel); f.Add(56,&pFuelBar); f.Add(57,&fFuel); f.Add(58,&pHPBarUnit); f.Add(59,&nPlayer); f.Add(60,&wszTooltipMemberInTransport); f.Add(61,&wszTooltipMemberInBuilding); return 0; }
private:
	void InitPrivate( IWindow *pInfo, IWindow *pAppearance, EType eType );

	void ClearInfo();
	void Clear3DView();

	bool IsSameWeapons( const struct SObjectStatus &status ) const;
	void MakeWeapons( const struct SObjectStatus &status, bool bSquad );
	void UpdateWeaponsAmmo( const struct SObjectStatus &status );

	void MakeName();
	void MakeMembers( vector<SMember> *pMembers );
	
	void MakeCurrentWeapons();
	void MakeCurrentArmor();
	void MakeCurrentSupplies();
	void MakeCurrentFlag();
	void MakeCurrent3DObjects();
	void MakeCurrentHP();
	void MakeCurrentFuel();
	void MakeCurrent();
	
	void ShowName();
	void ShowMembers();

	void ShowArmor();
	void ShowWeapons();
	void ShowResources();
	void ShowFlag();
	void Show3DObjects();
	void ShowHP();
	void ShowFuel();
	
	void HideSpecific();
	void ShowCurrent();

	void SetBaseID3D( int nID );
	
	void SetMemberTooltip( IWindow *pWnd );
public:
	CMissionUnitFullInfo();
	~CMissionUnitFullInfo();
	
	void InitByReinf( IWindow *pInfo, IWindow *pAppearanceWnd, NDb::ESeason eSeason );
	void InitByMission( IWindow *pInfo, IWindow *pAppearanceWnd, NDb::ESeason eSeason );
	void InitBySuperWeapon( IWindow *pInfo, IWindow *pAppearanceWnd, NDb::ESeason eSeason );
	
	void SetReinfUnit( const NDb::SHPObjectRPGStats *pStats );
	void SetObject( CMapObj *pMO );
	void UpdateObject( CMapObj *pMO );
	void UpdateMembers( bool bCanLeave );

	void OnClickMember( const string &szSender );
	void OnMemberOverOn( const string &szSender );
	void OnMemberOverOff( const string &szSender );
	void OnWeaponOverOn( const string &szSender );
	void OnWeaponOverOff( const string &szSender );
};


