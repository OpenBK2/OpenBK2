#pragma once
namespace NGScene
{

class IParticleFilter : virtual public CObjectBase
{
public:
	virtual void FilterParticles( const vector<CVec3> &positions, const vector<char> &skipped, vector<char> *pFilter ) = 0;
};
}
