//  ------------------------------------------------------------------
//  Compression utilities for B&W images
//  ------------------------------------------------------------------

#include <cstdint>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

int packbits( u_int8_t *out, const u_int8_t *buffer, int length );
int packzeroes( u_int8_t *out, const u_int8_t *const buffer, int length );
void unpackzeroesx( char *d, const char *s, size_t maxlen );

//  #### Note: u_int8_t is C not C++! (uint8_t)
