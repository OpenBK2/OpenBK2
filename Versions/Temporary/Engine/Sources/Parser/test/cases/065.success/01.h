attribute attr1( int );
attribute hidden();
basestruct int;

struct SB
{
};

forward enum S;

[attr1 = 10;]
class CA
{
	typedef int int_n;
	forward class CB;

public:
	struct S { };

protected:
	class CB { };
};

struct SA
{
	[hidden] typedef int int_n;
public:

	class CB
	{
	};

private:
	forward enum E;

	[attr1 = 10]
	typedef E e_typedef;

	enum E
	{
	  e1 = 10,
	};
protected:
};

enum S
{
	e1 = 15
};

