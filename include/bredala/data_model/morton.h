#include <cstddef>

unsigned int Morton_3D_Encode_10bit( unsigned int index1, unsigned int index2, unsigned int index3 );

void Morton_3D_Decode_10bit( const unsigned int morton, unsigned int& index1, unsigned int& index2, unsigned int& index3 );
