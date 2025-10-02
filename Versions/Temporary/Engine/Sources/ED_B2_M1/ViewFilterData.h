#pragma once

// настройки фильтра визуализации объектов сцены для MapInfo Editor
struct SViewFilterData
{ 
	struct SObjTypeFilter
	{
		string szObjTypeName;
		bool bShow;
		///
		SObjTypeFilter() : bShow( false ) {}
		///
		int operator&( IXmlSaver &xs )
		{
			xs.Add( "ObjTypeName", &szObjTypeName );
			xs.Add( "Show", &bShow );
			return 0;
		}
	};
	vector<SObjTypeFilter> objTypeFilter;			// показывать или нет объекты (units, buildings, static ...)
	bool bShowGrid;														// отображать сетку
	string szGridSize;												// тип(размер) сетки
	bool bShowBBoxes;													// показывать bounding boxes для моделей
	bool bWireFrame;													// режим ренедеринга wireframe
	bool bShowTerrain;												// показывать terrain
	bool bShowShadows;												// показывать тени
	bool bShowWarfog;													// показывать warfog
	bool bShowStats;													// показывать статистику
	bool bMipmap;															// отображать уровни текстур
	bool bOverdraw;														// отображать число перерисовок пикселов
	///
	SViewFilterData() :
	bShowGrid( false ),
		szGridSize( RCSTR("Visual tile") ),
		bShowBBoxes( false ),
		bWireFrame( false ),
		bShowTerrain( true ),
		bShowShadows( true ),
		bShowWarfog( true ),
		bShowStats( false ),
		bMipmap( false ),
		bOverdraw( false )
	{
		vector<string> typeNames;
		GetObjectTypesForFiltering( &typeNames );
		for ( int i = 0; i < typeNames.size(); ++i )
		{
			SObjTypeFilter tf;
			tf.szObjTypeName = typeNames[i];
			tf.bShow = true;
			objTypeFilter.push_back( tf );
		}
	}
	///
	void SetDefault()
	{
		*this = SViewFilterData();
	}
	///
	void GetObjectTypesForFiltering( vector<string> *pObjTypes )
	{
		if ( !pObjTypes )
			return;
		// эти типы объектов можно будет показывать/скрывать
		const char *pszObjTypeNames[] = 
		{
				"ObjectRPGStats",
				"MechUnitRPGStats",
				"BuildingRPGStats",
				"FenceRPGStats",
				"EntrenchmentRPGStats",
				"SquadRPGStats",
				"BridgeRPGStats",
				"TerraObjSetRPGStats",
				"MineRPGStats",
		};
		pObjTypes->clear();
		for ( int i = 0; i < sizeof(pszObjTypeNames)/sizeof(pszObjTypeNames[0]); ++i )
		{
			pObjTypes->push_back( pszObjTypeNames[i] );
		}
	}
	///
	int operator&( IXmlSaver &xs )
	{
		xs.Add( "objTypeFilter", &objTypeFilter );
		xs.Add( "ShowGrid", &bShowGrid );	
		xs.Add( "GridSize", &szGridSize );	
		xs.Add( "ShowBBoxes", &bShowBBoxes );	
		xs.Add( "WireFrame", &bWireFrame );	
		xs.Add( "ShowShadows", &bShowShadows );	
		xs.Add( "ShowWarFog", &bShowWarfog );	
		xs.Add( "ShowStats", &bShowStats );	
		//
		if ( xs.IsReading() )
		{
			vector<string> typeNames;
//			vector<string> newTypeNames;
			GetObjectTypesForFiltering( &typeNames );
			// очистить список считанный из файла от типов, которых нет в pszObjTypeNames[]
			for ( vector<SObjTypeFilter>::iterator it = objTypeFilter.begin(); it != objTypeFilter.end(); )
			{
				bool bInvalidType = true;
				for ( int j = 0; j < typeNames.size() ; ++j )
				{
					if ( typeNames[j] == it->szObjTypeName )
					{
						bInvalidType = false;
						break;
					}
				}
				if ( bInvalidType )
				{
					objTypeFilter.erase( it );
					if (  it == objTypeFilter.end() )
						break;
				}
				else
				{
					++it;
				}
			}
			// добавить типы, которых нет еще в файле на диске
			for ( int i = 0; i < typeNames.size(); ++i )
			{
				bool bNew = true;
				for ( int j = 0; j < objTypeFilter.size(); ++j )
				{
					if ( objTypeFilter[j].szObjTypeName == typeNames[i] )
					{
						bNew = false;
						break;
					}
				}
				if ( bNew )
				{
					SObjTypeFilter tf;
					tf.szObjTypeName = typeNames[i];
					tf.bShow = true;
					objTypeFilter.push_back( tf );
				}
			}
		}
		//
		return 0;
	}
};


