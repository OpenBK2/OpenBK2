basestruct int;

class C
{
};

typedef C *C_Reference;

struct S
{
};
typedef S S_Reference;

class A
{
  C_Reference pC;
  S_Reference S;
};

