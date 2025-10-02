#pragma once

#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "MapEditorLib/Tools_Geometry.h"
#include "EditorScene.h"
#include "Tools_SceneDraw.h"

#include <zconf.h>

typedef vector<NDb::SVSOPoint> CVSOPointList;
//
class CVSOManager
{
public:
	static const float DEFAULT_LEFT_BANK_HEIGHT;
	static const float DEFAULT_RIGHT_BANK_HEIGHT;
	static const float DEFAULT_WIDTH;
	static const float DEFAULT_HEIGHT;
	static const float DEFAULT_STEP;
	static const float DEFAULT_OPACITY;
	//
	static const float CONTROL_POINT_RADIUS;
	static const int CONTROL_POINT_PARTS;
	static const float CENTER_POINT_RADIUS;
	static const int CENTER_POINT_PARTS;
	static const float NORMALE_POINT_RADIUS;
	static const int NORMALE_POINT_PARTS;
	static const DWORD CONTROL_POINT_COLOR;
	static const DWORD CENTER_POINT_COLOR;
	static const DWORD NORMALE_POINT_COLOR;
	static const DWORD CONTROL_LINE_COLOR;
	static const DWORD CENTER_LINE_COLOR;
	static const DWORD NORMALE_LINE_COLOR;
	static const float DEFAULT_POINT_RADIUS;
	static const DWORD DEFAULT_POINT_PARTS;
	static const DWORD DEFAULT_POINT_COLOR;
	static const DWORD DEFAULT_LINE_COLOR;
	static const float OPACITY_DELIMITER;
	//
private:
	CVSOManager() {}
	//
	struct SVSOCircle : public CCircle
	{
		EClassifyRotation classifyRotation;
		CVec2 vCreationPoint;

		bool CreateFromDirection( const CVec2 &vBegin, const CVec2 &vEnd, float _fRadius, EClassifyRotation _classifyRotation, bool bBegin = true );
		bool GetTangentPoint( const CVec2 &v, CVec2 *pTangentPoint ) const;
		bool GetPointsSequence( const CVec2 &v, int nSegmentsCount, list<CVec2> *pPointsSequence ) const;
	};
	//
public:
	struct SBackupKeyPoints
	{
	private:
		struct SKeyPoint
		{
			float fWidth;
			float fHeight;
			float fOpacity;
			//			
			SKeyPoint() : fWidth( DEFAULT_WIDTH ), fHeight( DEFAULT_HEIGHT ), fOpacity( DEFAULT_OPACITY ) {}
			SKeyPoint( const SKeyPoint &rKeyPoint ) : fWidth( rKeyPoint.fWidth ), fHeight( rKeyPoint.fHeight ), fOpacity( rKeyPoint.fOpacity ) {}
			SKeyPoint& operator=( const SKeyPoint &rKeyPoint )
			{
				if( &rKeyPoint != this )
				{
					fWidth = rKeyPoint.fWidth;
					fHeight = rKeyPoint.fHeight;
					fOpacity = rKeyPoint.fOpacity;
				}
				return *this;
			}	
		};
		typedef list<SKeyPoint> CKeyPointList;

		CKeyPointList keyPointList;

	public:
		void SaveKeyPoints( const CVSOPointList &rVSOPointList );
		void LoadKeyPoints( CVSOPointList *pVSOPointList );
		void AddKeyPoint( int nKeyPointIndex, float fWidth = DEFAULT_WIDTH, float fHeight = DEFAULT_HEIGHT, float fOpacity = DEFAULT_OPACITY );
		void RemoveKeyPoint( int nKeyPointIndex );

		//very special methods for RMG
		void InsertToFront( float fWidth, float fHeight, float fOpacity );
		void InsertToBack( float fWidth, float fHeight, float fOpacity );
		void SetFrontOpacity( float fOpacity );
		void SetBackOpacity( float fOpacity );
		void Clear();
	};
	//
	struct SVSOSelection
	{
		enum ESelectionType
		{
			ST_UNKNOWN		= 0,
			ST_CONTROL		= 1,
			ST_CENTER			= 2,
			ST_NORMALE		= 3,
			ST_OPNORMALE	= 4,
			ST_COUNT			= 5,
		};
		ESelectionType eSelectionType;
		int nIndex;
		CVec3 vDifference;
		float fOpacity;
		//
		SVSOSelection()
			:	eSelectionType( ST_UNKNOWN ),
			nIndex( 0 ),
			vDifference( VNULL3 ),
			fOpacity( 0.0f ) {}
		SVSOSelection( const SVSOSelection &rVSOSelection )
			:	eSelectionType( rVSOSelection.eSelectionType ),
			nIndex( rVSOSelection.nIndex ),
			vDifference( rVSOSelection.vDifference ),
			fOpacity( rVSOSelection.fOpacity ) {}
		SVSOSelection& operator=( const SVSOSelection &rVSOSelection )
		{
			if( &rVSOSelection != this )
			{
				eSelectionType = rVSOSelection.eSelectionType;
				nIndex = rVSOSelection.nIndex;
				vDifference = rVSOSelection.vDifference;
				fOpacity = rVSOSelection.fOpacity;
			}
			return *this;
		}
		inline void Invalidate() { eSelectionType = ST_UNKNOWN; }
		inline bool IsValid() { return ( eSelectionType != ST_UNKNOWN ); }
		inline bool IsControlPointType() { return ( eSelectionType == ST_CONTROL ); }
		inline bool IsCenterPointType() { return ( eSelectionType == ST_CENTER ); }
		inline bool IsNormalePointType() { return ( ( eSelectionType == ST_NORMALE ) || ( eSelectionType == ST_OPNORMALE ) ); }
	};

	struct SVSOSelectionParam
	{
		SVSOSelection::ESelectionType eSelectionType;
		float fRadius;

		SVSOSelectionParam()
			:	eSelectionType( SVSOSelection::ST_UNKNOWN ),
			fRadius( 0.0f ) {}
		SVSOSelectionParam( SVSOSelection::ESelectionType _eSelectionType, float _fRadus )
			:	eSelectionType( _eSelectionType ),
			fRadius( _fRadus ) {}
		SVSOSelectionParam( const SVSOSelectionParam &rVSOSelectionParam )
			:	eSelectionType( rVSOSelectionParam.eSelectionType ),
			fRadius( rVSOSelectionParam.fRadius ){}
		SVSOSelectionParam& operator=( const SVSOSelectionParam &rVSOSelectionParam )
		{
			if( &rVSOSelectionParam != this )
			{
				eSelectionType = rVSOSelectionParam.eSelectionType;
				fRadius = rVSOSelectionParam.fRadius;
			}
			return *this;
		}
	};
	typedef list<SVSOSelectionParam> CVSOSelectionParamList;

private:
	static void SmoothCurve( const vector<int> &rKeyPointList, vector<float> *pPointList, bool bCircle, bool bComplete );
	static void SmoothCurveWidth( CVSOPointList *pVSOPointList, bool bCircle, bool bComplete );
	static void SmoothCurveHeight( CVSOPointList *pVSOPointList, bool bCircle, bool bComplete );
	static void SmoothCurveOpacity( CVSOPointList *pVSOPointList, bool bCircle, bool bComplete );

	static int SliceSpline( const class CAnalyticBSpline2 &rSpline,
													list<NDb::SVSOPoint> *pPoints,
													float *pfRest,
													const float fStep );
	static void SampleCurve( const vector<CVec3> &rControlPointList,
													 CVSOPointList *pVSOPointList,
													 float fStep, 
													 float fWidth,
													 float fHeight,
													 float fOpacity,
													 bool bCircle,
													 bool bComplete );

	static bool GetPointsSequence( const SVSOCircle &rCircleBegin, const SVSOCircle &rCircleEnd, int nSegmentsCountBegin, int nSegmentsCountEnd, list<CVec2> *pPointsSequence );
	static bool GetPointsSequence( const CVec2 &vBegin0, const CVec2 &vEnd0, float fRadius0, int nSegmentsCount0, bool bBegin0,
																 const CVec2 &vBegin1, const CVec2 &vEnd1, float fRadius1, int nSegmentsCount1, bool bBegin1,
																 list<CVec2> *pPointsSequence );

	//
public:
	enum EVSOPolygonType
	{
		PT_NORMALE		= 0,
		PT_OPNORMALE	= 1,
		PT_BOTH				= 2,
	};
	// Получить выделеннную точку
	static bool GetVSOSelection( SVSOSelection *pVSOSelection,												// Сюда записывается результат
															 const CVec3 &rvPos,																	// Позиция пересечения луча от мыши с землей
															 const CVec3 &rvOrigin,																// Параметры луча проходящие через курсор мыши ( стартовая точка )
															 const CVec3 &rvDirection,														// Параметры луча проходящие через курсор мыши ( единичный вектор направления )
															 const NDb::SVSOInstance &rVSO,												// VSO, в котором ищем
															 const CVSOSelectionParamList &rVSOSelectionParamList );
	static bool Update( NDb::SVSOInstance *pVSO,
											bool bResampleCurve,
											bool bKeepKeyPoints,
											float fStep,
											float fWidth,
											float fHeight,
											float fOpacity,
											bool bWidth,
											bool bHeight,
											bool bOpacity,
											bool bCircle,
											bool bComplete );
	static bool MoveEdgeControlPointsOut( NDb::SVSOInstance *pVSO, const CTRect<float> &rRect, float fStepOut, bool bBothEdges, bool bFixedStepOut );
	//
	static bool GetBoundingPolygon( list<CVec3> *pBoundingPolygon, const CVSOPointList &rVSOPointList, int nPointIndex, EVSOPolygonType vsoPolygonType, float fRelWidth );
	static bool GetBoundingPolygon( list<CVec3> *pBoundingPolygon, const CVSOPointList &rVSOPointList, EVSOPolygonType vsoPolygonType, float fRelWidth );
	static void GetBoundingPolygon( vector<CVec2> *pBoundingPolygon, const CVSOPointList &rVSOPointList, EVSOPolygonType vsoPolygonType, float fRelWidth );
	static void GetCragBoundingPolygon( vector<CVec2> *pBoundingPolygon, const CVSOPointList &rVSOPointList, EVSOPolygonType vsoPolygonType, float fRelWidth, int nVSOID );
	//
	static bool FindPath( const CVec2 &vBegin0, const CVec2 &vEnd0, bool bBegin0,
												const CVec2 &vBegin1, const CVec2 &vEnd1, bool bBegin1,
												float fRadius, int nSegmentsCount, float fMinEdgeLength, float fDistance, float fDisturbance, 
												list<CVec2> *pPointsSequence, const vector<vector<CVec2> > &rLockedPolygons, list<CVec2> *pUsedPoints,
												int nDepth = 0 );
	//не симметричный метод, первый VSO продолжается на две точки по отношению ко второму с соблюдением ширины
	//на обоих концах проставляется нулевая opacity
	static bool Merge( NDb::SVSOInstance *pVSO0, bool bVSO0Begin, NDb::SVSOInstance *pVSO1, bool bVSO1Begin );
	//вернуть первую не нулевую высоту
	static float GetEdgeHeght( const CVSOPointList &rVSOPointList, bool bBegin, bool bFirst );
	// Необходим для генерации дорог ( используется не правильно )	
	static string GetDescriptionName( const string &rszBeginVSODesc, const string &rszEndVSODesc )
	{
		return rszBeginVSODesc;
	}
	//static bool UpdateZ( NDb::SVSOInstance *pVSO );
	//
	struct SVSODrawParams
	{
		IEditorScene *pScene;
		CSceneDrawTool *pSceneDrawTool;
		NDb::SVSOInstance *pSelectedVSO;
		NDb::SVSOInstance *pVSO;
		CVSOManager::SVSOSelection *pCurrSelection;
		bool bIsClose;
		bool bIsDrawSceneDrawTool;
		enum EWhoDrawsVSO
		{
			BAD_DRAWER,
			ADV_CLIPBOARD,
			VSO_IS_SELECT,
			VSO_IS_EDIT,
			VSO_IS_ADD
		};
		EWhoDrawsVSO eDrawer; 
		bool canEditPointList[SVSOSelection::ST_COUNT];
		//
		SVSODrawParams()
		{
			pScene = 0;
			pSceneDrawTool = 0;
			pSelectedVSO = 0;
			pVSO = 0;
			pCurrSelection = 0;
			bIsClose = false;
			bIsDrawSceneDrawTool = false;
			eDrawer = BAD_DRAWER;
			memset( canEditPointList, 0, sizeof( canEditPointList ) );
		}
		//
		bool SetCanEdit( CVSOManager::SVSOSelection::ESelectionType ePointsType, bool bCanEdit )
		{
			int nIdx = static_cast<int>( ePointsType );
			if ( ( nIdx < 0 ) || ( nIdx >= sizeof( canEditPointList ) / sizeof( canEditPointList[0] ) ) )
			{
				return false;
			}
			canEditPointList[nIdx] = bCanEdit;
			return true;
		}
		//
		bool CanEditPoints( CVSOManager::SVSOSelection::ESelectionType ePointsType )
		{
			int nIdx = static_cast<int>( ePointsType );
			if ( ( nIdx < 0 ) || ( nIdx >= sizeof( canEditPointList ) / sizeof( canEditPointList[0] ) ) )
			{
				return false;
			}
			else 
			{
				return canEditPointList[nIdx];
			}
		}
		//
	};
	static void DrawVSO( SVSODrawParams *pDrawParams );
	//
};


