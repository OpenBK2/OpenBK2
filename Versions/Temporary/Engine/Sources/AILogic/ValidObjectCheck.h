#pragma once

template<class T>
inline bool IsValidObj( const T &pObj )
{
	return pObj && pObj->IsRefValid() && pObj->IsAlive();
}


