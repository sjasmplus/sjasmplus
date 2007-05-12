#ifdef NDEBUG
#define assert(exp)     ((void)0)
#else
void __cdecl _assert(void *, void *, unsigned);
#define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )
#endif  /* DEBUG */