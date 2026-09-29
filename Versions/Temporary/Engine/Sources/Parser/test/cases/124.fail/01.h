basestruct int;

class C
{
};

typedef C *C_Reference;
typedef C_Reference D_Reference;

class A
{
  D_Reference *pD;
};


