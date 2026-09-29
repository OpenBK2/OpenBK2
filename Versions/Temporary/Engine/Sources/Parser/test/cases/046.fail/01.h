struct S1 { };
struct S3 { };

struct S2 : public S1, private S3, public S1 { };

