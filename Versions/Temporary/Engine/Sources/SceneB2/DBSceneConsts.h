#pragma once

// automatically generated file, don't change manually!

#include "System/FilePath.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	struct SAmbientLight;
	struct STexture;
	struct SAIGeometry;
	struct SVisObjIconsSet;
	struct SMaterial;

	struct SLightEffectConsts
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< STexture > pLightConeTexture;
		CDBPtr< STexture > pFlareTexture;
		float fFlareAppearTime;

		SLightEffectConsts() :
			__dwCheckSum( 0 ),
			fFlareAppearTime( 0.0f )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SSceneConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SSceneConsts )
	public:
		enum { typeID = 0x100AC381 };

		struct SSelectionMaterials
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			CDBPtr< SMaterial > pAir;
			CDBPtr< SMaterial > pWater;
			CDBPtr< SMaterial > pGround;

			SSelectionMaterials() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};

		struct STrackMaterials
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			CDBPtr< SMaterial > pTrack;

			STrackMaterials() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};

		struct SIconAIGeometry
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			CDBPtr< SAIGeometry > pIcon;

			SIconAIGeometry() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};

		struct STerraGenConsts
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			NFile::CFilePath szBorderSmoothNoise;
			NFile::CFilePath szTextureCombiningNoise;
			NFile::CFilePath szDebrisMaskNoise;

			STerraGenConsts() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};

		struct SDebugMaterials
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			CDBPtr< SMaterial > pRedMaterial;
			CDBPtr< SMaterial > pGreenMaterial;
			CDBPtr< SMaterial > pBlueMaterial;
			CDBPtr< SMaterial > pBlackMaterial;
			CDBPtr< SMaterial > pWhiteMaterial;

			SDebugMaterials() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		SSelectionMaterials selectionMaterials;
		STrackMaterials trackMaterials;
		CDBPtr< SMaterial > pShotTraceMaterial;
		STerraGenConsts terraGenConsts;
		SIconAIGeometry iconAIGeometry;
		CDBPtr< SVisObjIconsSet > pVisObjIconsSet;
		SLightEffectConsts lightFX;
		CDBPtr< SAmbientLight > pInterfaceLight;
		SDebugMaterials debugMaterials;

		SSceneConsts() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};
}

