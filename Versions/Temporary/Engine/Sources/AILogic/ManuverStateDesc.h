#pragma once

namespace NDb
{
	enum EPlanesAttitude;
	struct SManuverDescriptor;
};

class CManuverStateDesc
{
	float enemyDirection;
	float selfDirection;
	float speedAngle;
	float distance;
	float selfHeight;
	float heightDifference;
	float selfSpeed;
	float enemySpeed;

	NDb::EPlanesAttitude att;

	template<class TParam>
		bool IsParamSuitable( const float fVal, const CDBPtr<TParam> pRange ) const
	{
		if ( pRange && ( pRange->fMin > fVal || pRange->fMax < fVal ) )
			return false;
		return true;
	}

public:	

	CManuverStateDesc() {  }
	/*enum EPlanesAttitude*/ const int GetAtt() const { return att; }

	// fill parameters according plane's & enemy's state
	void Init( const enum EPlanesAttitude _att, interface IPlane *pPos, interface IPlane *pEnemy );
	
	//const float & GetLO( const /*enum EParameterID*/ int id ) const { return parameters[id].first; }
	//const float & GetHI( const /*enum EParameterID*/ int id ) const { return parameters[id].second; }

	bool CheckSuitable( const NDb::SManuverDescriptor *pDesc ) const;	
};

