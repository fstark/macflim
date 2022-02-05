#include "ruler.hpp"

const uint8_ruler uint8_ruler::ruler;
const uint16_ruler uint16_ruler::ruler;
const uint32_ruler uint32_ruler::ruler;

class tester
{
public:
    tester()
    {
        const uint8_ruler &r = uint8_ruler::ruler;
        assert( r.distance( 0b00000001, 0b00000001 ) == 0 );
        assert( r.distance( 0b10000001, 0b00000001 ) == 2 );
        assert( r.distance( 0b10000001, 0b00000000 ) == 4 );
        assert( r.distance( 0b11111111, 0b00000000 ) == 16 );

        assert( r.distance( 0b00000001, 0b00000010 ) == 1 );
        assert( r.distance( 0b00000001, 0b00000100 ) == 2 );
        assert( r.distance( 0b00000001, 0b00001000 ) == 3 );


        bit_ruler<uint16_t> bit_ruler16;
        bit_ruler<uint32_t> bit_ruler32;

        assert( bit_ruler16.distance( 1, 0 )==5 );
        assert( bit_ruler32.distance( 1, 0 )==6 );

            //  This shows that distance isn't ideal
        assert( bit_ruler16.distance( 0b0000100000000000, 0b1000000000000000 )==6 );
        assert( bit_ruler16.distance( 0b0000100000000000, 0b0001000000000000 )==6 );

        assert( bit_ruler32.distance( 0b10000000000000000000000000000001, 0b00000000000000000000000000000000 )==12 );
        assert( bit_ruler32.distance( 0b10100000000000000000000000000000, 0b00000000000000000000000000000000 )==12 );
        assert( bit_ruler32.distance( 0b00100000000000000000000000000000, 0b10000000000000000000000000000000 )==4 );

        assert( bit_ruler16.distance( 0b0010000000000000, 0b1000000000000000 )==4 );
    }
};

tester t;
