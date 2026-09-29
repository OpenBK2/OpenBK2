#include "00.h"

forward class B;
        
class A
{
  forward class B;
  B *pB;

  class D
  {
    enum B { b_1 = 10, };
    B e_b;

   class E
   {
     B e_1, e_2[], e_3[10];
   };

  };

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

