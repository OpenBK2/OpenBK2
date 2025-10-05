#pragma once

struct IWindow;
namespace NDb
{
	struct SChapter;
	struct SMapInfo;
	struct STexture;
	struct SCampaign;
}

struct SChapterMapMenuHelper : public CObjectBase
{
	OBJECT_BASIC_METHODS( SChapterMapMenuHelper );
public:
	struct SArrow
	{
		ZDATA
		std::vector<CVec2> points;
		float fWidth;
		CDBPtr<NDb::STexture> pTexture;
		int nDependIndex;
		ZSKIP //CPtr<IWindow> pWnd;
		CTRect<float> rcBounds;
		int nID;
		CVec2 vDelta;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&points); f.Add(3,&fWidth); f.Add(4,&pTexture); f.Add(5,&nDependIndex); f.Add(7,&rcBounds); f.Add(8,&nID); f.Add(9,&vDelta); return 0; }
		
		DWORD GetColor() const;
		DWORD GetDependentColor() const;
	};
	
	struct SMission
	{
		ZDATA
		ZSKIP //int nX;
		ZSKIP //int nY;
		std::vector<SArrow> arrows;
		float fPotentialIncomplete;
		float fPotentialComplete;
		CPtr<IWindow> pWnd;
		ZSKIP //int nID;
		CVec2 vPos;
		bool bShowPotentialComplete;
		int nIndex;
		int nObjectIndex;
		CVec2 vEndOffset;
		ZEND int operator&( IBinSaver &f ) { f.Add(4,&arrows); f.Add(5,&fPotentialIncomplete); f.Add(6,&fPotentialComplete); f.Add(7,&pWnd); f.Add(9,&vPos); f.Add(10,&bShowPotentialComplete); f.Add(11,&nIndex); f.Add(12,&nObjectIndex); f.Add(13,&vEndOffset); return 0; }
	};
	
	ZDATA
	CVec2 vMainStrike;
	std::vector<SMission> missions;
	CVec2 vDetailsCoeff;
	CDBPtr<NDb::SMapInfo> pDetailsMap;
	CDBPtr<NDb::SChapter> pChapter;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vMainStrike); f.Add(3,&missions); f.Add(4,&vDetailsCoeff); f.Add(5,&pDetailsMap); f.Add(6,&pChapter); return 0; }
	
	SChapterMapMenuHelper() {}
	SChapterMapMenuHelper( const NDb::SChapter *pChapter, IWindow *pChapterMap );

	CVec2 Map2Screen( const CVec3 &vMapPos ) const;
	CVec3 Screen2Map( const CVec2 &vScreenPos ) const;

	void UpdateMission( NDb::SMapInfo *pDetailsMap, SMission *pMission );
	void UpdateArrow( NDb::SMapInfo *pDetailsMap, SArrow *pArrow );

	void MoveArrow( SArrow *pArrow, const CVec2 &vDelta );
	void ReReadPotentials();
};


