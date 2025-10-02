#include "00.h"

forward class B;
        
class A
{
  forward class B;
  B *pB;

  class B
  {
  };
	
};

class C
{
  B *pB;
};

class B
{
};

