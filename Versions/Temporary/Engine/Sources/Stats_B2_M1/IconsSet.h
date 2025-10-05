#pragma once

// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{
	struct SMaterial;
	struct STexture;

	struct SIconsSet : public CResource
	{
		OBJECT_BASIC_METHODS( SIconsSet )
	public:
		enum { typeID = 0x1311B302 };

		struct SIconType
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			enum EIconTypeEnum
			{
				ICONTYPE_NONE = 0,
				ICONTYPE_HPBAR = 1,
				ICONTYPE_GROUP00 = 2,
				ICONTYPE_GROUP01 = 3,
				ICONTYPE_GROUP02 = 4,
				ICONTYPE_GROUP03 = 5,
				ICONTYPE_GROUP04 = 6,
				ICONTYPE_GROUP05 = 7,
				ICONTYPE_GROUP06 = 8,
				ICONTYPE_GROUP07 = 9,
				ICONTYPE_GROUP08 = 10,
				ICONTYPE_GROUP09 = 11,
				ICONTYPE_BROKENTRUCK = 12,
				ICONTYPE_NEW_ABILITY = 13,
			};
			EIconTypeEnum eType;
			CTRect<float> rcRect;

			SIconType() :
				__dwCheckSum( 0 ),
				eType( ICONTYPE_NONE )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CDBPtr< SMaterial > pMaterialZCheck;
		CDBPtr< SMaterial > pMaterialNonZCheck;
		std::vector< SIconType > icons;

		SIconsSet() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SVisObjIconsSet : public CResource
	{
		OBJECT_BASIC_METHODS( SVisObjIconsSet )
	public:
		enum { typeID = 0x1313C400 };

		struct SVisObjIcon
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			enum EVisObjIconType
			{
				VOIT_NONE = 0,
				VOIT_GROUP00 = 1,
				VOIT_GROUP01 = 2,
				VOIT_GROUP02 = 3,
				VOIT_GROUP03 = 4,
				VOIT_GROUP04 = 5,
				VOIT_GROUP05 = 6,
				VOIT_GROUP06 = 7,
				VOIT_GROUP07 = 8,
				VOIT_GROUP08 = 9,
				VOIT_GROUP09 = 10,
				VOIT_BROKENTRUCK = 11,
				VOIT_NEW_ABILITY = 12,
				VOIT_INFANTRY = 13,
				VOIT_PROCESS = 14,
				VOIT_GUARD = 15,
				VOIT_NEED_REPAIR_KEY_OBJECT = 16,
				VOIT_NEED_RESUPPLY = 17,
				VOIT_AGRESSIVE = 18,
				VOIT_BEST_SHOT = 19,
				VOIT_PACIFIST = 20,
				VOIT_INVISIBLE = 21,
				VOIT_ABILITY_RANK_2 = 22,
				VOIT_ABILITY_RANK_3 = 23,
				VOIT_ABILITY_RANK_4 = 24,
			};
			EVisObjIconType eType;
			int nPriority;
			CTRect<float> rctexCoords;

			SVisObjIcon() :
				__dwCheckSum( 0 ),
				eType( VOIT_NONE ),
				nPriority( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CDBPtr< STexture > pTexture;
		std::vector< CTRect<float> > hpBarBorders;
		std::vector< CTRect<float> > hpBarColors;
		std::vector< SVisObjIcon > icons;
		std::vector< float > hpBarRanges;

		SVisObjIconsSet() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
}

namespace NDb
{
	std::string EnumToString( NDb::SIconsSet::SIconType::EIconTypeEnum eValue );
	SIconsSet::SIconType::EIconTypeEnum StringToEnum_NDb_SIconsSet_SIconType_EIconTypeEnum( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SIconsSet::SIconType::EIconTypeEnum>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SIconsSet::SIconType::EIconTypeEnum eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SIconsSet::SIconType::EIconTypeEnum ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SIconsSet_SIconType_EIconTypeEnum( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eValue );
	SVisObjIconsSet::SVisObjIcon::EVisObjIconType StringToEnum_NDb_SVisObjIconsSet_SVisObjIcon_EVisObjIconType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SVisObjIconsSet_SVisObjIcon_EVisObjIconType( szValue ); }
};

