#include "flimformat.hpp"

//  Computes fletcher 16 checksum over data
//  Data size() needs to be multiple of 2
void fletcher( long &checksum, const std::vector<uint8_t> &data )
{
    assert( (data.size()%2)==0 );
    for (size_t i=0;i!=data.size();i+=2)
    {
        checksum += ((int)(data[i]))*256+data[i+1];
        checksum %= 65535;
    }
}

//  Computes fletcher, data is big endian
void fletcher( long &checksum, uint16_t data )
{
    checksum += data;
    checksum %= 65535;
}
