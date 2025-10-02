#include "stdafx.h"

#include "AIDebugInfo.h"
#include "../DebugTools/DebugInfoManager.h"
#include "../System/FastMath.h"
#include "../Stats_B2_M1/DBPassProfile.h"

namespace NAIVisInfo
{

struct SProfileInfo
{
	NDb::SPassProfile profile;
	CVec2 vCenter;
	WORD wDir;

	SProfileInfo() { }
	SProfileInfo( const CVec2 &_vCenter, const WORD _wDir, const NDb::SPassProfile &_profile )
		: vCenter( _vCenter ), wDir( _wDir ), profile( _profile ) { }
};

static hash_map<int, SProfileInfo> profiles;
static bool bShow = false;

void AddProfile( const int nID, const CVec3 &vCenter, const WORD wDir, const NDb::SPassProfile &profile )
{
	profiles[nID] = SProfileInfo( CVec2( vCenter.x, vCenter.y ), wDir, profile );
}

void RemoveProfile( const int nID )
{
	profiles.erase( nID );
}

void ToggleLockProfiles()
{
	bShow = !bShow;
	for ( hash_map<int, SProfileInfo>::iterator iter = profiles.begin(); iter != profiles.end(); ++iter )
	{
		const NDb::SPassProfile &profile = iter->second.profile;
		const CVec2 vCenter = iter->second.vCenter;
		const WORD wDir = iter->second.wDir;
		if ( !profile.polygons.empty() )
		{
			int nCnt = 0;
			for ( int i = 0; i < profile.polygons.size(); ++i )
			{
				for ( int j = 0; j < profile.polygons[i].verts.size() - 1; ++j )
				{
					if ( bShow )
					{
						const float fAngle = float(wDir) / 65536 * FP_2PI;
						const CVec2 vRotation( NMath::Cos(fAngle), NMath::Sin(fAngle) );

						CSegment segm( vCenter + (profile.polygons[i].verts[j] ^ vRotation), vCenter + (profile.polygons[i].verts[j + 1] ^ vRotation) );
						DebugInfoManager()->CreateSegment( (nCnt << 20) | iter->first, segm, 4, NDebugInfo::WHITE );
					}
					else
						DebugInfoManager()->DeleteObject( (nCnt << 20) | iter->first );

					++nCnt;
				}
			}
		}
	}
}

}


