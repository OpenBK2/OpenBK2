attribute a1( int );
attribute a2( string );
attribute a3();

[a1 = 0; a2="2134"] [a3]
class CA
{
};


[a1=0] struct SS { };

[a3] struct SA : public SS
{
};

