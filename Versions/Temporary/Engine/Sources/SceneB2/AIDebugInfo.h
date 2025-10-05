#pragma once

#include "Scene.h"

namespace NDb
{
	struct SMaterial;
	struct SPassProfile;
}

namespace NAIVisInfo
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CEngineInfo: public CPtrFuncBase<NGScene::CObjectInfo>
	{
		CObj<CObjectBase> pMesh;
		CDBPtr<NDb::SMaterial> pMaterial;

		std::vector<CVec2> aiVerts;
		std::vector<STriangle> tris;

		bool bNeedUpdate;
	protected:
		virtual bool NeedUpdate()
		{ 
			return bNeedUpdate; 
		}

		virtual void Recalc();
	public:
		void InitEngineInfo( const NDb::SMaterial *pColor );
		void Invalidate();

		void SetVerts( std::vector<CVec2> &_aiVerts ) { aiVerts = _aiVerts; }
		void SetTris( std::vector<STriangle> &_aiTris ) { tris = _aiTris; }

		void SetNeedUpdate( const bool _bNeedUpdate ) { bNeedUpdate = _bNeedUpdate; }
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CEngineInfoHolder
	{
		CObj<CEngineInfo> pEngineInfo;
	public:
		CEngineInfoHolder() { pEngineInfo = new CEngineInfo(); }
		~CEngineInfoHolder() { pEngineInfo->Invalidate(); }

		void InitEngineInfo( const NDb::SMaterial *pColor ) { pEngineInfo->InitEngineInfo( pColor ); }
		void SetNeedUpdate( const bool bNeedUpdate ) { pEngineInfo->SetNeedUpdate( bNeedUpdate ); }

		void SetVerts( std::vector<CVec2> &aiVerts ) { pEngineInfo->SetVerts( aiVerts ); }
		void SetTris( std::vector<STriangle> &aiTris ) { pEngineInfo->SetTris( aiTris ); }
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CDebugObject : public CObjectBase
	{
	public:
		virtual void SetColor( const NDb::SMaterial *pColor ) = 0;
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CDebugSegment : public CDebugObject, public CEngineInfoHolder
	{
		OBJECT_BASIC_METHODS( CDebugSegment );
		
		static const float F_MIN_AI_THICKNESS;
		static const float F_AI_SPLICE_LEN;

		std::vector<CSegment> segmentsSplice;

		float fThick;
		CVec2 vDirPerp;
	public:
		void Init( const CSegment &aiSegment, const int nThickness );
		void SetPosition( const CSegment &aiSegment );

		void SetColor( const NDb::SMaterial *pColor ) { InitEngineInfo( pColor ); }
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CDebugCircle : public CDebugObject, public CEngineInfoHolder
	{
		OBJECT_BASIC_METHODS( CDebugCircle );

		static const float F_AI_TURN_X;
		static const float F_AI_TURN_Y;
		static const int N_SPLICE;
	public:
		void Init( const CCircle &aiCircle );
		void SetPosition( const CCircle &aiCircle );

		void SetColor( const NDb::SMaterial *pColor ) { InitEngineInfo( pColor ); }
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CDebugMarker : public CDebugObject, public CEngineInfoHolder
	{
		OBJECT_BASIC_METHODS( CDebugMarker );
	public:
		void Init( const std::vector<SVector> &tiles ) { Init( tiles, 0, tiles.size() ); }
		void Init( const std::vector<SVector> &tiles, const int nStart, const int nEnd );
		void SetPosition( const std::vector<SVector> &tiles, const int nStart, const int nEnd );

		void SetColor( const NDb::SMaterial *pColor ) { InitEngineInfo( pColor ); }
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CDebugMarkers : public CDebugObject
	{
		OBJECT_BASIC_METHODS( CDebugMarkers );

		typedef std::vector< CPtr<CDebugMarker> > TMarkers;
		TMarkers markers;
	public:
		void Init( const std::vector<SVector> &tiles );
		void SetPosition( const std::vector<SVector> &tiles );

		void SetColor( const NDb::SMaterial *pColor );
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


