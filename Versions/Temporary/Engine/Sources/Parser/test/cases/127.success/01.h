basestruct int;
attribute type_id( hexbinary );

// some class C, just for test
[type_id = 0xF5e24]
class C
{
  // very important variable
  int a[0x00a];
};

