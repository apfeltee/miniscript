
#if !defined(__dummystd_hdr_stddef_h)
#define __dummystd_hdr_stddef_h
/* dummy file for very old compilers */

#if !defined(__BCC__)
typedef unsigned int size_t;
#endif



#if !defined(__BCC__)
typedef long int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef int int16_t;
typedef unsigned int uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef char wchar_t;

#endif


#endif /*__dummystd_hdr_stddef_h */


