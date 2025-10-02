#include "00.h"

class B
{
};

class A
{
  [editorDesc = "some variables"]
  B *pB1, *pB2, *pB3_arra[10];

  int n, p;

  [nocode]
  int n_vector[10], n1, n_unbound[];
};

