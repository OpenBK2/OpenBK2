basestruct int;

struct S
{
};

typedef S S_Reference;
typedef S_Reference D_Reference;

class A
{
  D_Reference *pD;
};

