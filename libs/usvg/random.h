typedef unsigned long int  u4;
typedef struct ranctx { u4 a; u4 b; u4 c; u4 d; } ranctx;
u4 ranval( ranctx *x );

void raninit( ranctx *x, u4 seed );

