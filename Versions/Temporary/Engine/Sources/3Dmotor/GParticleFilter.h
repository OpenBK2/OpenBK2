#pragma once
namespace NGScene
{

class IParticleFilter : virtual public CObjectBase
{
public:
	virtual void FilterParticles( const std::vector<CVec3> &positions, const std::vector<char> &skipped, std::vector<char> *pFilter ) = 0;
};
}

