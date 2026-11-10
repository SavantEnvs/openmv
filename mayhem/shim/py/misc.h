#ifndef SHIM_PY_MISC_H
#define SHIM_PY_MISC_H
#include <stdlib.h>
#define m_malloc(n) malloc(n)
#define m_malloc0(n) calloc(1, (n))
#define m_realloc(p, n) realloc((p), (n))
void m_free(void *p);
#define m_new(t, n) ((t *) malloc(sizeof(t) * (n)))
#define m_del(t, p, n) free(p)
#endif
