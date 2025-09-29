/*

	Locator name				Description						
	---------------				-----------------------			
	LDoor						вход/выход для людей			
	LWindow						огневые точки
	----------------------------------------------------------------------------------------------------------------------

	Locator					Field (BuildingRPGStats)							Type
	----------------------- ------------------------------------------------	------------------------------------
	LDoor					entrances											CVec3 entrances[i].Pos
	LWindow					slots												CVec3 slots[i].Pos
	----------------------------------------------------------------------------------------------------------------
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\3dmotor\dbscene.h"
#include "../libdb/ResourceManager.h"
#include "..\MapEditorLib\ExporterFactory.h"
#include "..\MapEditorLib\ManipulatorManager.h"
#include "ExporterMethods.h"
#include "BuildingRPGStatsExporter.h"
#include "../ED_Common/TempAttributesTool.h"
#include "../Misc/StrProc.h"
#include "../MapEditorLib/StringManager.h"
#include "../MapEditorLib/Interface_MOD.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_EXPORTER_IN_DLL( BuildingRPGStats, CBuildingRPGStatsExporter )
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//										CBuildingRPGStatsExporter
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	генерить пыль при разрушении (по периметру с шагом в 1 тайл)
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool Passable( const CArray2D<BYTE> &rPassability, int nX, int nY )
{
	if ( (nX >= 0 && nX < rPassability.GetSizeX()) &&
		 (nY >= 0 && nY < rPassability.GetSizeY()) )
	{
		return (rPassability[nY][nX] == 0);
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum ESmoke
{
	ESD_NONE = 0,
	// направление эффекта (из одной точки может вылетать несколько в разном направлении)
	ESD_0 =		(1<<0),	
	ESD_45 =	(1<<1),	
	ESD_90 =	(1<<2),	
	ESD_135 =	(1<<3),	
	ESD_180 =	(1<<4),	
	ESD_225 =	(1<<5),	
	ESD_270 =	(1<<6),	
	ESD_315 =	(1<<7)	
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool RemoveSmokePoints( IManipulator *pBuildingRPGStatsManipulator )
{
	return pBuildingRPGStatsManipulator->RemoveNode( "smokePoints", NODE_REMOVEALL_INDEX );	
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddSmokePoint( IManipulator *pBuildingRPGStatsManipulator, int nOx, int nOy, float fDirAngle )
{
	CVec3 vAICoord = VNULL3;
	vAICoord.x = nOx * AI_TILE_SIZE;
	vAICoord.y = nOy * AI_TILE_SIZE;

	CVariant v;
	pBuildingRPGStatsManipulator->GetValue( "smokePoints", &v );
	int nElemIdx = v;

	pBuildingRPGStatsManipulator->InsertNode( "smokePoints" );

	char pszDBAPos[64]={0};
	sprintf( pszDBAPos, "smokePoints.[%d].Pos", nElemIdx );
	CManipulatorManager::SetVec3( vAICoord, pBuildingRPGStatsManipulator, pszDBAPos );

	char pszDBADirection[64]={0};
	sprintf( pszDBADirection, "smokePoints.[%d].Direction", nElemIdx );
	pBuildingRPGStatsManipulator->SetValue( pszDBADirection, fDirAngle );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void CreateDestructionDustPoints( IManipulator *pBuildingRPGStatsManipulator )
{
	CArray2D<BYTE> passability;
	GetPassability( &passability, pBuildingRPGStatsManipulator );	

	int nSizeX = passability.GetSizeX();
	int nSizeY = passability.GetSizeY();

	//
	// определение точек периметра и вычисление direction 
	// 
	CArray2D<int> direction;
	direction.SetSizes( nSizeX+1, nSizeY+1 ); 
	for ( int x = 0; x < nSizeX+1; ++x )
	{
		for ( int y = 0; y < nSizeY+1; ++y )
		{
			direction[y][x] = ESD_NONE;
		}
	}

	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			bool bPassable = Passable( passability, x, y );
			if ( bPassable )
				continue;

			if ( Passable( passability, x-1, y ) )
				direction[y][x] |= ESD_180;

			if ( Passable( passability, x+1, y ) )
				direction[y][x] |= ESD_0;

			if ( Passable( passability, x, y-1 ) )
				direction[y][x] |= ESD_90;

			if ( Passable( passability, x, y+1 ) )
				direction[y][x] |= ESD_270;

			if ( Passable( passability, x-1, y-1 ) )
			{
				if ( Passable( passability, x-1, y ) && Passable( passability, x, y-1 ) )
					direction[y][x] |= ESD_135;
			}

			if ( Passable( passability, x-1, y+1 ) )
			{
				if ( Passable( passability, x-1, y ) && Passable( passability, x, y+1 ) )
					direction[y][x] |= ESD_225;
			}

			if ( Passable( passability, x+1, y-1 ) )
			{
				if ( Passable( passability, x, y-1 ) && Passable( passability, x+1, y ) )
					direction[y][x] |= ESD_45;
			}

			if ( Passable( passability, x+1, y+1 ) )
			{
				if ( Passable( passability, x, y+1 ) && Passable( passability, x+1, y ) )
					direction[y][x] |= ESD_315;
			}
		}
	}


	// т.к. точки должны быть с шагом в 1 визуальный тайл, то нужно
	// уменьшить число точек в периметре (в визульном тайле 4 AI-тайла)
	CArray2D<int> visDirection;
	visDirection.SetSizes( nSizeX+1, nSizeY+1 ); 
	for ( int x = 0; x < nSizeX+1; ++x )
	{
		for ( int y = 0; y < nSizeY+1; ++y )
		{
			visDirection[y][x] = ESD_NONE;
		}
	}

	for ( int x = 0; x < nSizeX; x += 2 )
	{
		for ( int y = 0; y < nSizeY; y += 2 )
		{
			// оставляем одну из 4-х точек периметра в визуальном тайле
			for ( int ix = 0; ix < 2; ++ix )
			{
				for ( int iy = 0; iy < 2; ++iy )
				{
					if ( direction[y+iy][x+ix] != ESD_NONE )
					{
						visDirection[y+iy][x+ix] = direction[y+iy][x+ix];
						ix = 2;
						break;
					}
				}
			}
		}
	}

	// генерация точек
	RemoveSmokePoints( pBuildingRPGStatsManipulator );

	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			if ( visDirection[y][x] == ESD_NONE )
				continue;

			// углы
			if ( (visDirection[y][x] & ESD_0) && (visDirection[y][x] & ESD_90) )
				visDirection[y][x] = ESD_45;
			if ( (visDirection[y][x] & ESD_90) && (visDirection[y][x] & ESD_180) )
				visDirection[y][x] = ESD_135;
			if ( (visDirection[y][x] & ESD_180) && (visDirection[y][x] & ESD_270) )
				visDirection[y][x] = ESD_225;
			if ( (visDirection[y][x] & ESD_270) && (visDirection[y][x] & ESD_0) )
				visDirection[y][x] = ESD_315;
			//

			for ( int h = 0; h <= 7; ++h )
			{
				if ( visDirection[y][x] & (1<<h) )
				{
					int nOx = x;
					int nOy = y;
					float fAngle = ( ( 45.0f * h ) / 360.0f ) * 0xFFFF;
					AddSmokePoint( pBuildingRPGStatsManipulator, nOx, nOy, fAngle );
					// оставляем только одно направление вылета из этой точки
					break;
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Complex solution for all entrances and slots
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SLocalDoorInfo
{
	CVec3 vPos;
	int nDir;
};
struct SLocalWindowInfo
{
	string szLocatorName;
	int nFound;							// Entry found (index); -1 if not found
	CVec3 vPos;
	CVec3 vDamageCenter;
	CQuat qRot;
	int nDir;
	CVec2 vWindowScale;
	int nStartTime;					// for animation
	int nEndTime;
	
	bool bSection;					// "window" is section (for M1)
};
typedef list<SLocalDoorInfo> CLocalDoors;
typedef list<SLocalWindowInfo> CLocalWindows;

float QuatToDirection( const CQuat &qRot )
{
	float fDir;
	CVec3 vRot;
	SHMatrix mRot;
	qRot.DecompEulerMatrix( &mRot );
	mRot.RotateVector( &vRot, V3_AXIS_Z );
	float fAngle = atan2( vRot.y, vRot.x ); // -pi...+pi

	// угол возвращается в AI системе отчсчета (отсчитывается против часовой стрелки от (0,1,0))
	if ( fAngle < 0 ) // 0..2pi
		fAngle += FP_2PI;

	if ( fAngle <= FP_PI2 )
	{
		const float F_3PI2 = 3 * FP_PI2;
		fAngle += F_3PI2;
	}
	else if ( fAngle > FP_PI2 )
	{
		fAngle -= FP_PI2;
	}

	// в AI еденицах
	const float F_EPS_ANGLE = 0.01f;
	if ( fabs(fAngle) < F_EPS_ANGLE )
	{
		fDir = 0xFFFF;
	}
	else
	{
		fDir = (fAngle / FP_2PI)*0xFFFF;
	}

	return fDir;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// обновить анимации для объекта
const bool CBuildingRPGStatsExporter::UpdateVisObj( IManipulator* pManipulator, const string &szRefName, const vector<SAnimationInfo> &frames, const int nStage )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pVisObj = CManipulatorManager::CreateManipulatorFromReference( szRefName, pManipulator, 0, 0, 0 );
	if ( !pVisObj )
	{
		return false;
	}
  hash_set<string> models; //имена моделей, которым надо подправить анимацию
	int nModelCount = 0;
	if ( !CManipulatorManager::GetValue( &nModelCount, pVisObj, "Models" ) )
	{
		return false;
	}
  for ( int i = 0; i < nModelCount; ++i )
	{
		const string szModelPath = StrFmt( "Models.[%d].Model", i );
		string szModelName;
		if ( !CManipulatorManager::GetParamsFromReference( szModelPath, pVisObj, 0, &szModelName, 0 ) )
		{
			continue;
		}
		if ( models.find( szModelName ) == models.end() )
		{ 
			models.insert( szModelName );
		}
	}
	for ( hash_set<string>::const_iterator it = models.begin(); it != models.end(); ++it )
	{
		CPtr<IManipulator> pModel = Singleton<IResourceManager>()->CreateObjectManipulator( "Model", *it );
		if ( !pModel )
		{
			continue;
		}

		pModel->RemoveNode( "Animations", NODE_REMOVEALL_INDEX );
		string szSkeletonName = "";
    CPtr<IManipulator> pSkeleton = CManipulatorManager::CreateManipulatorFromReference( "Skeleton", pModel, 0, &szSkeletonName, 0 );
		if ( !pSkeleton )
			continue;

		pSkeleton->RemoveNode( "Animations", NODE_REMOVEALL_INDEX );

		// воссоздать анимации для скелета
		for ( int i = nStage; i >= 0; --i )
		{
			string szSrcName = "";
			string szRootJoint = "";

			CManipulatorManager::GetValue( &szSrcName, pSkeleton, "SrcName" );
			CManipulatorManager::GetValue( &szRootJoint, pSkeleton, "RootJoint" );

			int nAnimationIndex = 0;
			CManipulatorManager::GetValue( &nAnimationIndex, pSkeleton, "Animations" );

			pSkeleton->InsertNode( "Animations", NODE_ADD_INDEX );

			const string szAnimationName = szSkeletonName + StrFmt( " (%d - %d)", frames[i].nStartTime, frames[nStage].nEndTime );
			if ( pFolderCallback->IsUniqueName( "AnimB2", szAnimationName ) && !pFolderCallback->InsertObject( "AnimB2", szAnimationName ) )
			{
				return false;
			}
			CPtr<IManipulator> pAnimation = Singleton<IResourceManager>()->CreateObjectManipulator( "AnimB2", szAnimationName );
			if ( !pAnimation )
			{
				return false;
			}
			CManipulatorManager::SetValue( szSrcName, pAnimation, "SrcName", false );
			CManipulatorManager::SetValue( szRootJoint, pAnimation, "RootJoint", false );
			CManipulatorManager::SetValue( "ANIMATION_DEATH", pAnimation, "Type", false );
			CManipulatorManager::SetValue( 0, pAnimation, "Action" );
			CManipulatorManager::SetValue( false, pAnimation, "Looped" );
			CManipulatorManager::SetValue( frames[i].nStartTime, pAnimation, "FirstFrame" );
			CManipulatorManager::SetValue( frames[nStage].nEndTime, pAnimation, "LastFrame" );
			CManipulatorManager::SetValue( "", pAnimation, "AABBAName", false );
			CManipulatorManager::SetValue( "", pAnimation, "AABBDName", false );
			CManipulatorManager::SetValue( 1.0f, pAnimation, "MoveSpeed" );

			const string szAnimationPath = StrFmt( "Animations.[%d]", nAnimationIndex );
			string szTypeAndName;
			CStringManager::GetRefValueFromTypeAndName( &szTypeAndName, "AnimB2", szAnimationName, TYPE_SEPARATOR_CHAR );
			if ( !CManipulatorManager::SetValue( szTypeAndName, pSkeleton, szAnimationPath, true ) )
				return false;
		}
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// сделать копию модели, если szOldModelName и szNewModelName совпадают, просто очистить модель от анимации, прописать новые RootMesh и RootJoint, поправить текстуры
const bool CBuildingRPGStatsExporter::CopyModel( const string &szOldModelName, const string &szNewName, const string &szRoot )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	if ( szOldModelName != szNewName && !pFolderCallback->CopyObject( "Model", szNewName, szOldModelName ) )
		return false;

	CPtr<IManipulator> pModel = Singleton<IResourceManager>()->CreateObjectManipulator( "Model", szNewName );
	if ( !pModel )
		return false;

	//удалить анимацию
	pModel->RemoveNode( "Animations", NODE_REMOVEALL_INDEX );

	//копировать геометрию
	string szGeometry = "";
	string szGeometryType = "";
	if ( !CManipulatorManager::GetParamsFromReference( "Geometry", pModel, &szGeometryType, &szGeometry, 0 ) )
		return false;
	if ( szNewName != szGeometry && !pFolderCallback->CopyObject( szGeometryType, szNewName, szGeometry ) )
		return false;
	CPtr<IManipulator> pGeometry = Singleton<IResourceManager>()->CreateObjectManipulator( szGeometryType, szNewName );
	if ( !pGeometry )
		return false;
	if ( !CManipulatorManager::SetValue( szRoot, pGeometry, "RootMesh", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( szRoot, pGeometry, "RootJoint", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( "", pGeometry, "AIGeometry", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( szNewName, pModel, "Geometry", true ) )
		return false;

	//копировать скелет
	string szSkeleton = "";
	string szSkeletonType = "";
	if ( !CManipulatorManager::GetParamsFromReference( "Skeleton", pModel, &szSkeletonType, &szSkeleton, 0 ) )
		return false;
	if ( szNewName != szSkeleton && !pFolderCallback->CopyObject( szSkeletonType, szNewName, szSkeleton ) )
		return false;
	CPtr<IManipulator> pSkeleton = Singleton<IResourceManager>()->CreateObjectManipulator( szSkeletonType, szNewName );
	if ( !pSkeleton )
		return false;
	if ( !CManipulatorManager::SetValue( szRoot, pSkeleton, "RootJoint", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( szNewName, pModel, "Skeleton", true ) )
		return false;
	pSkeleton->RemoveNode( "Animations", NODE_REMOVEALL_INDEX );

	//при первом проходе оставляем материалы пустыми, необходимо переэкспортить модельку
	pModel->RemoveNode( "Materials", NODE_REMOVEALL_INDEX );
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// создать новый VisObj на основе уже существующего
const bool CBuildingRPGStatsExporter::CreateVisObj( IManipulator* pManipulator, const string &szObjectName, const string &szRoot )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	string szName;
	if ( !CManipulatorManager::GetParamsFromReference( "visualObject", pManipulator, 0, &szName, 0 ) )
		return false;

	if ( !pFolderCallback->CopyObject( "VisObj", szObjectName, szName ) )
		return false;

	CPtr<IManipulator> pVisObj = Singleton<IResourceManager>()->CreateObjectManipulator( "VisObj", szObjectName );
	if ( !pVisObj )
		return false;

	//делаем копии моделей, если в оригинальном объекте модель для сезона новая, в новом объекте также будет новая
	int nModelCount = 0;
	if ( !CManipulatorManager::GetValue( &nModelCount, pVisObj, "Models" ) )
		return false;

	hash_map<string, int> models; // имя модели - на индекс имени в names
	vector<string> names; // новые имена моделей

	for ( int i = 0; i < nModelCount; ++i )
	{
		const string szModelPath = StrFmt( "Models.[%d].", i );
		string szModelName = "";
		if ( !CManipulatorManager::GetParamsFromReference( szModelPath + "Model", pVisObj, 0, &szModelName, 0 ) )
		{
			continue;
		}
		hash_map<string, int>::const_iterator pos = models.find( szModelName );
		int nNameIndex = -1;
		if ( pos == models.end() )
		{
			string szSeason;
			if ( !CManipulatorManager::GetValue( &szSeason, pVisObj, szModelPath + "Season" ) )
			{
				continue;
			}
			const string szNewModelName = szObjectName + " (" + szSeason + ")";
			if ( !CopyModel( szModelName, szNewModelName, szRoot ) )
			{
				continue;
			}
			nNameIndex = names.size();
			names.push_back( szNewModelName );
			models[szModelName] = nNameIndex;
		}
		else
		{
			nNameIndex = pos->second;
		}
		CManipulatorManager::SetValue( names[nNameIndex], pVisObj, szModelPath + "Model", true );
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CBuildingRPGStatsExporter::ProcessVisObj( IManipulator *pManipulator, const string &szRefName, const string &szNewName, const string &szRoot, const vector<SAnimationInfo> &frames, const int nStage )
{
	string szVisObjName = "";
	if ( !CManipulatorManager::GetParamsFromReference( szRefName, pManipulator, 0, &szVisObjName, 0 ) )
	{
		szVisObjName = szNewName;
		if ( !CreateVisObj( pManipulator, szVisObjName, szRoot ) )
			return false;
		CManipulatorManager::SetValue( szVisObjName, pManipulator, szRefName, true );
	}
	return UpdateVisObj( pManipulator, szRefName, frames, nStage );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CBuildingRPGStatsExporter::UpdateEntrancesAndSlots( IManipulator *pManipulator, const string &szObjectName )
{
	CVec3 vPos, vRot3, vAIPos;
	CQuat qRot;
	CVec2 vWindowScale;
	int nSlotType;
	const int SLOT_WINDOW = 0;
	const int SLOT_DOOR = 1;
	const int SLOT_SECTION = 2;
	int nDir;

	CLocalDoors doors;
	CLocalWindows windows;

	doors.clear();
	windows.clear();
	vector<bool> slotsUsed;				// keeps index in the new list
	int nExistingSlots = 0;
	CManipulatorManager::GetValue( &nExistingSlots, pManipulator, "slots" );
	slotsUsed.resize( nExistingSlots, false );

	// Получаем манипулятор на VisObject
	CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pManipulator, 0, 0, 0 );
	if ( !pVisObjectManipulator )
		return false;

	// Get Model manipulator
	CPtr<IManipulator> pItModel = CreateModelManipulatorFromVisObj( pVisObjectManipulator, 0 );
	if ( !pItModel )
		return false;

	// Get Geometry manipulator
	CPtr<IManipulator> pGeomManipulator = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pItModel, 0, 0, 0 );
	if ( !pGeomManipulator )
		return false;

	// Get all attributes from model
	{
		CGrannyBoneAttributesList attribs;
		if ( GetGeometryAttributes( pGeomManipulator, &attribs ) )
		{
			// Export light properties to DB if successful
			for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it ) 
			{
				if ( PatMat( it->szBoneName.c_str(), "lwindow??" ) )						// Process window slot
					nSlotType = SLOT_WINDOW;
				else if ( PatMat( it->szBoneName.c_str(), "ldoor??" ) )					// Process door slot
					nSlotType = SLOT_DOOR;
				else if ( PatMat( it->szBoneName.c_str(), "lsection??" ) )					// Process section slot
					nSlotType = SLOT_SECTION;
				else
					continue;

				it->GetAttribute( "translatex", &vPos.x );
				it->GetAttribute( "translatey", &vPos.y );
				it->GetAttribute( "translatez", &vPos.z );

				it->GetAttribute( "rotatex", &vRot3.x );
				it->GetAttribute( "rotatey", &vRot3.y );
				it->GetAttribute( "rotatez", &vRot3.z );

				qRot.FromEulerAngles( vRot3.z, vRot3.y, vRot3.x );
				nDir = int( QuatToDirection( qRot ) );

			// Add to list, fill data
			if ( nSlotType != SLOT_DOOR ) 
			{
				SLocalWindowInfo info;
				if ( nSlotType == SLOT_SECTION )
				{
					Vis2AI( &info.vDamageCenter, vPos );// Store position (in AI coords)
					info.qRot = QNULL;
					info.nDir = 0;
					info.vPos = VNULL3;
					it->GetAttribute( "starttime", &(info.nStartTime) );
					it->GetAttribute( "endtime", &(info.nEndTime) );
				}
				else
				{
					Vis2AI( &info.vPos, vPos );				// Store position (in AI coords)
					info.qRot = qRot;
					info.nDir = nDir;
					info.vDamageCenter = VNULL3;
					info.nStartTime = 0;
					info.nEndTime = 0;
				}
				info.vWindowScale = VNULL2;
				it->GetAttribute( "windowscalex", &info.vWindowScale.x );
				it->GetAttribute( "windowscaley", &info.vWindowScale.y );
				if ( info.vWindowScale.x == 0.0f )				// if any of the scales is 0, make default
					info.vWindowScale.x = 1.0f;
				if ( info.vWindowScale.y == 0.0f )
					info.vWindowScale.y = 1.0f;

					// Search for according entry
					info.szLocatorName = it->szRealName;
					info.nFound = -1;
					for ( int i = 0; i < nExistingSlots; ++i )
					{
						const string szEntryNameDBA = StrFmt( "slots.[%d].LocatorName", i );
						string szEntryLocatorName;
						CManipulatorManager::GetValue( &szEntryLocatorName, pManipulator, szEntryNameDBA );
						if ( szEntryLocatorName == info.szLocatorName )			// Match found
						{
							info.nFound = i;
							slotsUsed[ i ] = true;
							break;
						}
					}
					info.bSection = nSlotType == SLOT_SECTION;
					windows.push_back( info );
				}
				else
				{
					SLocalDoorInfo info;
					Vis2AI( &info.vPos, vPos );				// Store position (in AI coords)
					info.nDir = nDir;
					doors.push_back( info );
				}
			}
		}
	}

	// Add doors
	int nEntranceIndex = 0;
	pManipulator->RemoveNode( "entrances" );
	for ( CLocalDoors::iterator it = doors.begin(); it != doors.end(); ++it )
	{
		if ( !pManipulator->InsertNode( "entrances" ) ) 
			continue;

		const string szNodePrefix = StrFmt( "entrances.[%d].", nEntranceIndex );

		// Write data
		CManipulatorManager::SetVec3( it->vPos, pManipulator, szNodePrefix + "Pos" );
		pManipulator->SetValue( szNodePrefix + "Dir", it->nDir );			
		pManipulator->SetValue( szNodePrefix + "Stormable", true );			// All entrances are stormable by default

		++nEntranceIndex;
	}
	// Add windows
	int nSlotNodes = nExistingSlots;
	int nCurrentSlot;

	// get default window
	string szDefaultWindowDay = "";
	string szDefaultWindowNight = "";
	string szDefaultWindowDestroyed = "";
	string szDefaultWindowEffect = "";
	CManipulatorManager::GetValue( &szDefaultWindowDay, pManipulator, "DefaultWindow.DayObj" );
	CManipulatorManager::GetValue( &szDefaultWindowNight, pManipulator, "DefaultWindow.NightObj" );
	CManipulatorManager::GetValue( &szDefaultWindowDestroyed, pManipulator, "DefaultWindow.DestroyedObj" );
	CManipulatorManager::GetValue( &szDefaultWindowEffect, pManipulator, "DefaultWindow.DestroyEffect" );

	string szModelFileName = "";
	if ( !CManipulatorManager::GetValue( &szModelFileName, pGeomManipulator, "SrcName" ) )
		szModelFileName = "";

	for ( CLocalWindows::iterator it = windows.begin(); it != windows.end(); ++it )
	{
		if ( it->nFound == -1 ) 
		{
			if ( !pManipulator->InsertNode( "slots" ) ) 
				continue;

			nCurrentSlot = nSlotNodes;
			++nSlotNodes;
		}
		else
			nCurrentSlot = it->nFound;

		const string szNodePrefix = StrFmt( "slots.[%d].", nCurrentSlot );

		pManipulator->SetValue( szNodePrefix + "LocatorName", it->szLocatorName );			
		CManipulatorManager::SetVec3( it->vPos, pManipulator, szNodePrefix + "Pos" );
		CManipulatorManager::SetVec3( it->vDamageCenter, pManipulator, szNodePrefix + "DamageCenter" );
		pManipulator->SetValue( szNodePrefix + "Direction", float(float(it->nDir) * 360.0f / 65536.0f) );			

		CVec4 vRot = it->qRot.GetInternalVector();
		CManipulatorManager::SetVec4( vRot, pManipulator, szNodePrefix + "Rot" ); 

		CManipulatorManager::SetVec2( it->vWindowScale, pManipulator, szNodePrefix + "WindowScale" );

		// add default window for a new slot
		if ( it->nFound == -1 && !it->bSection )
		{
			if ( szDefaultWindowDay != "" )
				pManipulator->SetValue( szNodePrefix + "Window.DayObj", szDefaultWindowDay );
			if ( szDefaultWindowNight != "" )
				pManipulator->SetValue( szNodePrefix + "Window.NightObj", szDefaultWindowNight );
			if ( szDefaultWindowDestroyed != "" )
				pManipulator->SetValue( szNodePrefix + "Window.DestroyedObj", szDefaultWindowDestroyed );
			if ( szDefaultWindowEffect != "" )
				pManipulator->SetValue( szNodePrefix + "Window.DestroyEffect", szDefaultWindowEffect );
		}

		if ( it->bSection )
		{
			CGrannyBoneAttributesList attribs;
			string szSectionName = it->szLocatorName;
			szSectionName.erase( 0, 1 );
			const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			granny_file_info *pGFI = NMEGeomAttribs::GetAttribs( pUserData->constUserData.szExportSourceFolder + szModelFileName, szSectionName, "" );
			if ( ReadAttributes( &attribs, pGFI, szSectionName, true ) )
			{
				pManipulator->RemoveNode( szNodePrefix + "Window.NightDamageLevels", NODE_REMOVEALL_INDEX );
				// подготовка массивов
				int nStageCount = 0;
				for ( CGrannyBoneAttributesList::const_iterator itAttr = attribs.begin(); itAttr != attribs.end(); ++itAttr )  
				{
					if ( PatMat( itAttr->szBoneName.c_str(), "*lstage??" ) )						// Process destruction stages
						++nStageCount;
				}
				int nLevelsCount = 0;
				CManipulatorManager::GetValue( &nLevelsCount, pManipulator, szNodePrefix + "Window.DayDamageLevels" );
				if ( nLevelsCount < nStageCount )
				{
					for ( int i = nLevelsCount; i < nStageCount; ++i )
						pManipulator->InsertNode( szNodePrefix + "Window.DayDamageLevels", NODE_ADD_INDEX );
				}
				else if ( nLevelsCount > nStageCount )
				{
					for ( int i = nStageCount; i < nLevelsCount; ++i )
						pManipulator->RemoveNode( szNodePrefix + "Window.DayDamageLevels", i );
				}
				// заполнение информации об анимациях
				vector<SAnimationInfo> vFrames;
				vFrames.resize( nStageCount );
				for ( CGrannyBoneAttributesList::const_iterator itAttr = attribs.begin(); itAttr != attribs.end(); ++itAttr )
				{
					if ( PatMat( itAttr->szBoneName.c_str(), "*lstage??" ) )						// Process destruction stages
					{
						const string szStageNumber = itAttr->szBoneName.substr( itAttr->szBoneName.size() - 2 );
						const int nStageNumber = NStr::ToInt( szStageNumber ) - 1;
						if ( nStageNumber < 0 || nStageNumber > nStageCount )
						{
							NLog::GetLogger()->Log( LT_ERROR, "Wrong stage locator\n" );
							NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Section: %s; Stage locator: %s (must be Lstage01 .. Lstage%02d)\n", szSectionName.c_str(), itAttr->szBoneName.c_str(), nStageCount ) );
							return false;
						}

						itAttr->GetAttribute( "starttime", &(vFrames[nStageNumber].nStartTime) );
						itAttr->GetAttribute( "endtime", &(vFrames[nStageNumber].nEndTime) );
					}
				}

				// создание необходимой структуры
				for ( CGrannyBoneAttributesList::const_iterator itAttr = attribs.begin(); itAttr != attribs.end(); ++itAttr ) 
				{
					if ( PatMat( itAttr->szBoneName.c_str(), "*lstage??" ) )						// Process destruction stages
					{
						const string szStageNumber = itAttr->szBoneName.substr( itAttr->szBoneName.size() - 2 );
						const int nStageNumber = NStr::ToInt( szStageNumber ) - 1;

						const string szVisObjName = szObjectName + PATH_SEPARATOR_CHAR + szSectionName + PATH_SEPARATOR_CHAR + StrFmt( "stage%02d", nStageNumber );
						const string szVisObjPath = szNodePrefix + "Window.DayDamageLevels" + StrFmt( ".[%d].VisObj", nStageNumber );
						ProcessVisObj( pManipulator, szVisObjPath, szVisObjName, szSectionName, vFrames, nStageNumber );
					}
				}
				{ // DayObj - целый объект
					const string szVisObjName = szObjectName + PATH_SEPARATOR_CHAR + szSectionName + PATH_SEPARATOR_CHAR + "whole";
					const string szVisObjPath = szNodePrefix + "Window.DayObj";
					ProcessVisObj( pManipulator, szVisObjPath, szVisObjName, szSectionName, vFrames, -1 );
				}
				/*
				{ // DestroyedObj - разрушенный объект
					const string szVisObjName = szObjectName + PATH_SEPARATOR_CHAR + szSectionName + PATH_SEPARATOR_CHAR + "destroyed";
					const string szVisObjPath = szNodePrefix + "Window.DestroyedObj";
					ProcessVisObj( pManipulator, pFolderCallback, szVisObjPath, szVisObjName, szSectionName, vFrames, nStageCount );
				}
				*/
			}
		}
	}
	
	// Clean up unused windows
	for ( int i = nExistingSlots - 1; i >= 0; --i )
	{
		if ( !slotsUsed[ i ] )
			pManipulator->RemoveNode( "slots", i );
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CBuildingRPGStatsExporter::CreateTexture( const string &szTextureName, const string &szFileName )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	if ( pFolderCallback->IsUniqueName( "Texture", szTextureName ) && !pFolderCallback->InsertObject( "Texture", szTextureName ) )
		return false;

	CPtr<IManipulator> pTexture = Singleton<IResourceManager>()->CreateObjectManipulator( "Texture", szTextureName );
	if ( !pTexture )
		return false;

	CPtr<IManipulator> pTextureFolder = Singleton<IResourceManager>()->CreateFolderManipulator( "Texture" );
	if ( !pTextureFolder )
		return false;

	CManipulatorManager::SetValue( szFileName, pTexture, "SrcName", false );
	CManipulatorManager::SetValue( string( "REGULAR" ), pTexture, "Type", false );
	CManipulatorManager::SetValue( string( "CONVERT_TRANSPARENT" ), pTexture, "ConversionType", false );
	CManipulatorManager::SetValue( string( "CLAMP" ), pTexture, "AddrType", false );
	CManipulatorManager::SetValue( string( "TF_DXT3" ), pTexture, "Format", false );
	CManipulatorManager::SetValue( 0.0f, pTexture, "MappingSize" );
	CManipulatorManager::SetValue( 0, pTexture, "NMips" );
	CManipulatorManager::SetValue( 0, pTexture, "Gain" );
	CManipulatorManager::SetValue( false, pTexture, "InstantLoad" );
	CManipulatorManager::SetValue( false, pTexture, "FlipY" );

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const string CBuildingRPGStatsExporter::GetMaterial( const string &szModelName, const string &szModelPath, const int nMaterial, const bool bTransparent, const bool bReflective )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	const int nMaterialIndex = nMaterial + ( bTransparent ? 0x10000000 : 0 ) + ( bReflective ? 0x20000000 : 0 );
	hash_map<int, string>::const_iterator pos = materials.find( nMaterialIndex );
	if ( pos == materials.end() )
	{
		const string szMaterialNamePrefix = szModelName + StrFmt( " (%d.tga", nMaterial );
		string szMaterialName = szMaterialNamePrefix;
		if ( bTransparent )
			szMaterialName += ", transp";
		if ( bReflective )
			szMaterialName += ", mirror";
		szMaterialName += ")";

		if ( pFolderCallback->IsUniqueName( "Material", szMaterialName ) && !pFolderCallback->InsertObject( "Material", szMaterialName ) )
			return "";
		CPtr<IManipulator> pMaterial = Singleton<IResourceManager>()->CreateObjectManipulator( "Material", szMaterialName );
		if ( !pMaterial )
			return "";

		string szTexture = "";
		string szMirrorTexture = "";
		if ( bReflective )
		{
			szMirrorTexture = szMaterialNamePrefix + ", mirror)";
			if ( !CreateTexture( szMirrorTexture, szModelPath + StrFmt( "%d_m.tga", nMaterial ) ) )
				return "";
		}
		if ( bTransparent )
		{
			szTexture = szMaterialNamePrefix + ", transp)";
			if ( !CreateTexture( szTexture, szModelPath + StrFmt( "%dt.tga", nMaterial ) ) )
				return "";
		}
		else
		{
			szTexture = szMaterialNamePrefix + ")";
				if ( !CreateTexture( szTexture, szModelPath + StrFmt( "%d.tga", nMaterial ) ) )
					return "";
		}
 
		CManipulatorManager::SetValue( szTexture, pMaterial, "Texture", true );
		CManipulatorManager::SetValue( bReflective ? 1 : 0, pMaterial, "MetalMirror" );
		CManipulatorManager::SetValue( szMirrorTexture, pMaterial, "Mirror", true );
		CManipulatorManager::SetValue( bTransparent ? string( "AM_TRANSPARENT" ) : string( "AM_ALPHA_TEST" ), pMaterial, "AlphaMode" );

    materials[nMaterialIndex] = szMaterialName;

		Singleton<IExporterContainer>()->ExportObject( pMaterial, "Material", szMaterialName, true, EXPORT_REFERENCES );

		return szMaterialName;
	}
	else
		return pos->second;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int MAT_TRANSPARENT = 0x00000001;
const int MAT_REFLECTIVE  = 0x00000002;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CBuildingRPGStatsExporter::UpdateModels( IManipulator *pManipulator, const string &szRefName, const string &szObjectName, const int nMaterial )
{
	CPtr<IManipulator> pVisObj = CManipulatorManager::CreateManipulatorFromReference( szRefName, pManipulator, 0, 0, 0 ); 
	if ( !pVisObj )
		return false;

	int nModelsCount = 0;
	CManipulatorManager::GetValue( &nModelsCount, pVisObj, "Models" );
	hash_set<string> models;
	for ( int i = 0; i < nModelsCount; ++i )
	{
		string szModelName;
		CManipulatorManager::GetParamsFromReference( StrFmt( "Models.[%d].Model", i ), pVisObj, 0, &szModelName, 0 );
		if ( !szModelName.empty() )
		{	
			models.insert( szModelName );
		}
	}
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	CPtr<IManipulator> pModelFolder = Singleton<IResourceManager>()->CreateFolderManipulator( "Model" );
	if ( !pModelFolder )
	{
		return false;
	}
	for ( hash_set<string>::const_iterator itModel = models.begin(); itModel != models.end(); ++itModel )
	{
		CPtr<IManipulator> pModel = Singleton<IResourceManager>()->CreateObjectManipulator( "Model", *itModel );
		if ( !pModel )
		{
			return false;
		}
		// чтение аттрибутов каждого mesh'а: поиск transparent и reflective
		CGrannyBoneAttributesList attribs;
		ReadAttributes( &attribs, NMEGeomAttribs::GetAttribsByModel( pModel ), "", true );
		hash_map<string, int> attributes;
		for ( CGrannyBoneAttributesList::const_iterator itAttr = attribs.begin(); itAttr != attribs.end(); ++itAttr ) 
		{
			SGrannyBoneAttributes::CAttributeMap::const_iterator posTransparent = itAttr->attributeMap.find( "transparent" );
			const bool bTransparent = posTransparent != itAttr->attributeMap.end() ? posTransparent->second : false;
			SGrannyBoneAttributes::CAttributeMap::const_iterator posReflective = itAttr->attributeMap.find( "reflective" );
			const bool bReflective = posReflective != itAttr->attributeMap.end() ? posReflective->second : false;
			attributes[itAttr->szBoneName] = ( bTransparent ? MAT_TRANSPARENT : 0 ) + ( bReflective ? MAT_REFLECTIVE : 0 );
		}

		string szGeometryName;
		CPtr<IManipulator> pGeometry = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModel, 0, &szGeometryName, 0 );
		if ( !pGeometry || szGeometryName.empty() )
			return false;
		string szModelPath = "";
		CManipulatorManager::GetValue( &szModelPath, pGeometry, "SrcName" );
		if ( szModelPath == "" )
			return false;
		int nSlashIndex = szModelPath.rfind( '\\' );
		if ( nSlashIndex == -1 )
			nSlashIndex = szModelPath.rfind( '/' );
		if ( nSlashIndex == -1 )
			return false;
		szModelPath.erase( nSlashIndex + 1 );
//		CGrannyFileInfoGuard pInfo( pUserData->szExportDestinationFolder + StrFmt( "bin\\geometries\\%d", nGeomID ) );
		string szGeomFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\geometries\\";
		CDBPtr<NDb::SGeometry> pDBGeometry = NDb::Get<NDb::SGeometry>( CDBID( szGeometryName ) );
		CGrannyFileInfoGuard pInfo(  NBinResources::GetExistentBinaryFileName( szGeomFolder, pDBGeometry->GetRecordID(), pDBGeometry->uid ) ); // uid
		if ( pInfo->ModelCount != 1 ) 
			return false;

		pModel->RemoveNode( "Materials", NODE_REMOVEALL_INDEX );

		for ( int i = 0; i < pInfo->Models[0]->MeshBindingCount; ++i )
		{
			string szBoneName = pInfo->Models[0]->MeshBindings[i].Mesh->BoneBindings[0].BoneName;
			NStr::ToLower( &szBoneName );
			const int nAttributes = attributes[szBoneName];
			pModel->InsertNode( "Materials", NODE_ADD_INDEX );
			CManipulatorManager::SetValue( GetMaterial( *itModel, szModelPath, nMaterial, nAttributes & MAT_TRANSPARENT, nAttributes & MAT_REFLECTIVE ), pModel, StrFmt( "Materials.[%d]", i ), true );
		}
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CBuildingRPGStatsExporter::UpdateSectionMaterials( IManipulator *pManipulator, const string &szObjectName )
{
	int nSlotsCount = 0;
	if ( !CManipulatorManager::GetValue( &nSlotsCount, pManipulator, "slots" ) )
		return false;
	
	for ( int i = 0; i < nSlotsCount; ++i )
	{
		string szSlotPrefix = StrFmt( "slots.[%d].", i );
		string szLocatorName = "";
		if ( !CManipulatorManager::GetValue( &szLocatorName, pManipulator, szSlotPrefix + "LocatorName" ) || PatMat( szLocatorName.c_str(), "Lsection??" ) == 0 )
			continue;

		UpdateModels( pManipulator, szSlotPrefix + "Window.DayObj", szObjectName, 1 );
		UpdateModels( pManipulator, szSlotPrefix + "Window.DestroyedObj", szObjectName, 2 );

		int nStageCount = 0;
		if ( CManipulatorManager::GetValue( &nStageCount, pManipulator, szSlotPrefix + "Window.DayDamageLevels" ) )
		{
			for ( int n = 0; n < nStageCount; ++n )
				UpdateModels( pManipulator, szSlotPrefix + StrFmt( "Window.DayDamageLevels.[%d].VisObj", n ), szObjectName, n + 3 );
		}
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXPORT_RESULT CBuildingRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																											const string &rszObjectTypeName,
																											const string &rszObjectName,
																											bool bForce,
																											EXPORT_TYPE exportType )
{
	CObjectBaseRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	//
	if ( exportType == ET_BEFORE_REF )
		UpdateEntrancesAndSlots( pManipulator, rszObjectName );

	if ( exportType == ET_AFTER_REF )
		UpdateSectionMaterials( pManipulator, rszObjectName );

	// точки вылета пыли
	if ( exportType != ET_BEFORE_REF )
		CreateDestructionDustPoints( pManipulator );
	
	return ER_SUCCESS;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
