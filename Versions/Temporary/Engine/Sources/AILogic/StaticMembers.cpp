#include "stdafx.h"

#include "StaticMembers.h"
#include "Commands.h"
#include "InBuildingStates.h"
#include "InTransportStates.h"
#include "InEntrenchmentStates.h"
#include "PlaneStates.h"
#include "TankStates.h"
#include "TransportStates.h"
#include "FormationStates.h"
#include "ArtilleryStates.h"
#include "ArtRocketStates.h"
#include "Soldier.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CStaticMembers::operator&( IBinSaver &saver )
{
	saver.Add( 1, &CQueueUnit::cmds );
	saver.Add( 2, &CAICommand::paths );
	saver.Add( 3, &CAICommand::cmdIds );

	if ( !saver.IsChecksum() )
	{
		saver.Add( 4, &CInBuildingStatesFactory::pFactory );
		saver.Add( 6, &CInEntrenchmentStatesFactory::pFactory );
		saver.Add( 8, &CInTransportStatesFactory::pFactory );
		saver.Add( 10, &CPlaneStatesFactory::pFactory );
		saver.Add( 12, &CSoldierStatesFactory::pFactory );
		saver.Add( 15, &CTankStatesFactory::pFactory );
		saver.Add( 16, &CTransportStatesFactory::pFactory );
		saver.Add( 17, &CFormationStatesFactory::pFactory );
	}
	if ( saver.IsReading() )
	{

		det_map<int, CPtr<CLinkObject> > objs;
		saver.Add( 18 ,&objs );
		int nMax = 0;
		for ( det_map<int, CPtr<CLinkObject> >::iterator it = objs.begin(); it != objs.end(); ++it )
			nMax = (std::max)( nMax, it->first );

		SLinkObjDataAutoMagic::pLinkObjData->link2object.resize( nMax + 1, 0 );
		for ( det_map<int, CPtr<CLinkObject> >::iterator it = objs.begin(); it != objs.end(); ++it )
			SLinkObjDataAutoMagic::pLinkObjData->link2object[it->first] = it->second;
	}
	else if ( !saver.IsChecksum() )
	{
		det_map<int, CPtr<CLinkObject> > objs;
		for ( int i = 0; i < SLinkObjDataAutoMagic::pLinkObjData->link2object.size(); ++i )
		{
			if ( SLinkObjDataAutoMagic::pLinkObjData->link2object[i] != 0 )
				objs[i] = SLinkObjDataAutoMagic::pLinkObjData->link2object[i];
		}
		saver.Add( 18 ,&objs );
	}
	saver.Add( 19, &SLinkObjDataAutoMagic::pLinkObjData->deletedObjects );
	saver.Add( 20, &SLinkObjDataAutoMagic::pLinkObjData->unitsID2object );
	saver.Add( 21, &SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID );
	saver.Add( 22, &SLinkObjDataAutoMagic::pLinkObjData->deletedUniqueObjects );

	if ( !saver.IsChecksum() )
	{
		saver.Add( 23, &CArtilleryStatesFactory::pFactory );
		saver.Add( 24, &CArtRocketStatesFactory::pFactory	);
	}
	saver.Add( 26, &CExistingObject::globalMark );
	saver.Add( 27, &SConsts::PRIORITIES );

	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

