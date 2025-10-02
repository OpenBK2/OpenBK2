#pragma once

namespace NDb
{
	struct SMaterial;
}

namespace NGScene
{

const NDb::SMaterial *GetReplacedMaterial( const string &szName, int nInd );

}

