#ifndef _huffman_h_
#define _huffman_h_

int Huffman_Compress( unsigned char *in, unsigned char *out, unsigned int insize );
void Huffman_Uncompress( unsigned char *in, unsigned char *out, unsigned int insize, unsigned int outsize );

#endif
