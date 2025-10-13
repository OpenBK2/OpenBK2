#pragma once

#include "Image/Image.h"

#include <cstdint>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SVectorStripeObjectPoint
{
	CVec3 vPos;														// point position
	CVec3 vNorm;													// normale at this point
	float fRadius;												// curvature radius
	float fWidth;													// width at this point
	bool	bKeyPoint;											// key point of the sampling
	float fOpacity;												// прозрачность ( 0..1 ) только для key point

	//----------------------------------------------------------------------------------------------------
	SVectorStripeObjectPoint()
		: vPos( VNULL3 ), vNorm( VNULL3 ), fRadius( 0.0f ), fWidth( 0.0f ), bKeyPoint( false ), fOpacity( 1.0f ) {}

	//----------------------------------------------------------------------------------------------------
	int operator&( IXmlSaver &saver );
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SVectorStripeObjectDesc
{
	enum EType
	{
		TYPE_UNKNOUN	= 0,
		TYPE_RIVER		= 1,
		TYPE_ROAD			= 2,
		TYPE_RAILROAD	= 3,
	};
	
	//----------------------------------------------------------------------------------------------------
	struct SLayer
	{
		uint8_t opacityCenter;									// прозрачность в центре потока
		uint8_t opacityBorder;									// прозрачность по краям
		float fStreamSpeed;									// условная скорость потока
		float fTextureStep;									// шаг текстурирования по тайлам
		int nNumCells;											// ширина потока в ячейках (в тайлах)
		bool bAnimated;											// animated layer
		std::string szTexture;							// текстура потока (или директория, если это анимированная текстура)
		float fDisturbance;									// mesh disturbance
		float fRelWidth;										// relative width

		SLayer()
			: opacityCenter( 0xff ), opacityBorder( 0x80 ), fStreamSpeed( 0.1f ), fTextureStep( 0.1f ),	fDisturbance( 0.3f ), fRelWidth( 1 ), nNumCells( 4 ), bAnimated( false ) {}
		
		int operator&( IXmlSaver &saver );
		int operator&( IBinSaver &saver );
	};
	
	//----------------------------------------------------------------------------------------------------
	int	eType;														// type
	int nPriority;												// priority
	float fPassability;										// passability
	uint32_t dwAIClasses;										// AI классы, которые не могут ходить по этой дороге

	enum ESoilParams
	{ 
		ESP_TRACE = 0x01,
		ESP_DUST	= 0x10,
		ESP_RAIL	= 0x20,
		ESP_SPLASH = 0x40,
	};
	uint8_t cSoilParams;											// параметры почвы - следы, пыль и т.д.
	
	//----------------------------------------------------------------------------------------------------
	// layers
	SLayer bottom;												// bottom central layer
	std::vector<SLayer> bottomBorders;		// bottom layer border parts
	std::vector<SLayer> layers;						// additional layers
	NImage::SColor miniMapCenterColor;						// цвет обьекта на минимапе ( центральная часть )
	NImage::SColor miniMapBorderColor;						// цвет обьекта на минимапе ( край )
	
	//----------------------------------------------------------------------------------------------------
	// ambient sound
	std::string szAmbientSound;
	
	//----------------------------------------------------------------------------------------------------
	SVectorStripeObjectDesc() 
		: eType( TYPE_UNKNOUN ), nPriority( 0 ), miniMapCenterColor( 0x00000000 ), miniMapBorderColor( 0x00000000 ),
			fPassability( 1.0f ), dwAIClasses( 0 ), cSoilParams( 0 ) { }
	
	//----------------------------------------------------------------------------------------------------
	virtual int operator&( IXmlSaver &saver );
	virtual int operator&( IBinSaver &saver );
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SVectorStripeObject : SVectorStripeObjectDesc
{
	std::string szDescName;								// complete path to descriptor

	//----------------------------------------------------------------------------------------------------
	// points
	std::vector<SVectorStripeObjectPoint> points;	// points
	std::vector<CVec3> controlpoints;			// control polyline points

	//----------------------------------------------------------------------------------------------------
	// object's ID
	int nID;															// ID

	//----------------------------------------------------------------------------------------------------

	virtual int operator&( IXmlSaver &saver );
	virtual int operator&( IBinSaver &saver );
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef std::vector<SVectorStripeObject> TVSOList;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

