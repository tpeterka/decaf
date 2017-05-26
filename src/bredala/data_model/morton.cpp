unsigned int Morton_3D_Encode_10bit( unsigned int index1, unsigned int index2, unsigned int index3 )
{ // pack 3 10-bit indices into a 30-bit Morton code
  index1 &= 0x000003ff;
  index2 &= 0x000003ff;
  index3 &= 0x000003ff;
  index1 |= ( index1 << 16 );
  index2 |= ( index2 << 16 );
  index3 |= ( index3 << 16 );
  index1 &= 0x030000ff;
  index2 &= 0x030000ff;
  index3 &= 0x030000ff;
  index1 |= ( index1 << 8 );
  index2 |= ( index2 << 8 );
  index3 |= ( index3 << 8 );
  index1 &= 0x0300f00f;
  index2 &= 0x0300f00f;
  index3 &= 0x0300f00f;
  index1 |= ( index1 << 4 );
  index2 |= ( index2 << 4 );
  index3 |= ( index3 << 4 );
  index1 &= 0x030c30c3;
  index2 &= 0x030c30c3;
  index3 &= 0x030c30c3;
  index1 |= ( index1 << 2 );
  index2 |= ( index2 << 2 );
  index3 |= ( index3 << 2 );
  index1 &= 0x09249249;
  index2 &= 0x09249249;
  index3 &= 0x09249249;
  return( index1 | ( index2 << 1 ) | ( index3 << 2 ) );
}

void Morton_3D_Decode_10bit( const unsigned int morton,
                                 unsigned int& index1, unsigned int& index2, unsigned int& index3 )
    { // unpack 3 10-bit indices from a 30-bit Morton code
      unsigned int value1 = morton;
      unsigned int value2 = ( value1 >> 1 );
      unsigned int value3 = ( value1 >> 2 );
      value1 &= 0x09249249;
      value2 &= 0x09249249;
      value3 &= 0x09249249;
      value1 |= ( value1 >> 2 );
      value2 |= ( value2 >> 2 );
      value3 |= ( value3 >> 2 );
      value1 &= 0x030c30c3;
      value2 &= 0x030c30c3;
      value3 &= 0x030c30c3;
      value1 |= ( value1 >> 4 );
      value2 |= ( value2 >> 4 );
      value3 |= ( value3 >> 4 );
      value1 &= 0x0300f00f;
      value2 &= 0x0300f00f;
      value3 &= 0x0300f00f;
      value1 |= ( value1 >> 8 );
      value2 |= ( value2 >> 8 );
      value3 |= ( value3 >> 8 );
      value1 &= 0x030000ff;
      value2 &= 0x030000ff;
      value3 &= 0x030000ff;
      value1 |= ( value1 >> 16 );
      value2 |= ( value2 >> 16 );
      value3 |= ( value3 >> 16 );
      value1 &= 0x000003ff;
      value2 &= 0x000003ff;
      value3 &= 0x000003ff;
      index1 = value1;
      index2 = value2;
      index3 = value3;
    }
