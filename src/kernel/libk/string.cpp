#include "string.h"

void memset(void *a, int c, size_t size)
{
	char* dst = (char*)a;

	for (size_t i = 0; i < size; i++)
		dst[i] = c;
}

int strncmp( const char * s1, const char * s2, size_t n )
{
    while ( n && *s1 && ( *s1 == *s2 ) )
    {
        ++s1;
        ++s2;
        --n;
    }
    if ( n == 0 )
    {
        return 0;
    }
    else
    {
        return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
    }
}

size_t strlen(const char *a)
{
	size_t b = 0;
	while (*a++)
		b++;
	return b;
}

void strncpyz(char *dst, const char *src, size_t size)
{
    while (*src && size--)
        *dst++ = *src++;
    *dst++ = '\0';
}
