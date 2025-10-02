attribute a_int( int );
attribute a_string( string );
attribute a_dword( DWORD );
attribute a_float( float );
attribute a_word( WORD );
attribute a_bool( bool );
attribute a();

basestruct int;

[a; a_float = +1.45]
typedef int s_int1;

[a_float = -+1e10f; a_dword = 10; a_word = 15; a_string = "asdfasdf"; a_int = 0; a_bool = false]
[a] typedef int s_int2;

[a_float = 1.0] [a_dword = 10; a_word = 15;] [a;] 
[a] [a_string = "asdfasdf"; a_int = 0] typedef int s_int3;

[a;][a_float = 1e-15] [a_dword = 10; a_word = 15;]
typedef int s_int4;

enum e
{
e_1,
};

typedef e s_enum4;

