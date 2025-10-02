#include "stdafx.h"


bool operator>( const SAIAngle &_1, const SAIAngle &_2 ) 
{ return _1.wAngle > _2.wAngle; }
bool operator>( int _1, const SAIAngle &_2 ) 
{ return _1 > _2.wAngle; }
bool operator>( const SAIAngle &_1, int _2 ) 
{ return _1.wAngle > _2; }

bool operator<( const SAIAngle &_1, const SAIAngle &_2 ) 
{ return _1.wAngle < _2.wAngle; }
bool operator<( int _1, const SAIAngle &_2 ) 
{ return _1 < _2.wAngle; }
bool operator<( const SAIAngle &_1, int _2 ) 
{ return _1.wAngle < _2; }

