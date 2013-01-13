typedef struct {
    unsigned char *BytePtr;
    unsigned int  BitPos;
} BitStream;

typedef struct {
    int Symbol;
    unsigned int Count;
    unsigned int Code;
    unsigned int Bits;
} freq_table;

typedef struct huff_encodenode_struct HuffmanEncodeNode;

struct huff_encodenode_struct {
    HuffmanEncodeNode *ChildA, *ChildB;
    int Count;
    int Symbol;
};

typedef struct huff_decodenode_struct HuffmanDecodeNode;

struct huff_decodenode_struct {
    HuffmanDecodeNode *ChildA, *ChildB;
    int Symbol;
};

#define MAX_TREE_NODES 511

static void InitBitStream( BitStream *stream, unsigned char *buf ){
  stream->BytePtr  = buf;
  stream->BitPos   = 0;
}

static unsigned int ReadBitFromBitStream( BitStream *stream ){
  unsigned int  x, bit;
  unsigned char *buf;
  buf = stream->BytePtr;
  bit = stream->BitPos;
  x = (*buf & (1<<(7-bit))) ? 1 : 0;
  bit = (bit+1) & 7;
  if( !bit ){
    ++ buf;
  }
  stream->BitPos = bit;
  stream->BytePtr = buf;
  return x;
}

static unsigned int ReadByteFromBitStream( BitStream *stream ){
  unsigned int  x, bit;
  unsigned char *buf;
  buf = stream->BytePtr;
  bit = stream->BitPos;
  x = (*buf << bit) | (buf[1] >> (8-bit));
  ++ buf;
  stream->BytePtr = buf;
  return x;
}

static void WriteBitsToBitStream( BitStream *stream, unsigned int x, unsigned int bits ){
  unsigned int  bit_pos, count;
  unsigned char *buf;
  unsigned int  mask;
  buf = stream->BytePtr;
  bit_pos = stream->BitPos;
  mask = 1 << (bits-1);
  for( count = 0; count < bits; ++ count )
  {
    *buf = (*buf & (0xff^(1<<(7-bit_pos)))) + ((x & mask ? 1 : 0) << (7-bit_pos));
    x <<= 1;
    bit_pos = (bit_pos+1) & 7;
    if( !bit_pos )
    {
      ++ buf;
    }
  }
  stream->BytePtr = buf;
  stream->BitPos  = bit_pos;
}

static void CreateFreq_Table( unsigned char *InputFileByteStream, freq_table *sym, unsigned int InputFileSize )
{
  int k;

  for( k = 0; k < 256; ++ k )
  {
    sym[k].Symbol = k;
    sym[k].Count  = 0;
    sym[k].Code   = 0;
    sym[k].Bits   = 0;
  }

  for( k = InputFileSize; k; -- k )
  {
    sym[*InputFileByteStream ++].Count ++;
  }
}

static void StoreHuffmanTree( HuffmanEncodeNode *node, freq_table *sym, BitStream *stream, unsigned int code, unsigned int bits )
{
  unsigned int sym_idx;
  if( node->Symbol >= 0 )
  {
    WriteBitsToBitStream( stream, 1, 1 );
    WriteBitsToBitStream( stream, node->Symbol, 8 );

    for( sym_idx = 0; sym_idx < 256; ++ sym_idx )
    {
      if( sym[sym_idx].Symbol == node->Symbol ) break;
    }

    sym[sym_idx].Code = code;
    sym[sym_idx].Bits = bits;
    return;
  }
  else
  {
    WriteBitsToBitStream( stream, 0, 1 );
  }

  StoreHuffmanTree( node->ChildA, sym, stream, (code<<1)+0, bits+1 );
  StoreHuffmanTree( node->ChildB, sym, stream, (code<<1)+1, bits+1 );
}


static void CreateHuffmanTree( freq_table *sym, BitStream *stream )
{
  HuffmanEncodeNode nodes[MAX_TREE_NODES], *node_1, *node_2, *root;
  unsigned int k, num_symbols, nodes_left, next_idx;


  num_symbols = 0;
  for( k = 0; k < 256; ++ k )
  {
    if( sym[k].Count > 0 )
    {
      nodes[num_symbols].Symbol = sym[k].Symbol;
      nodes[num_symbols].Count = sym[k].Count;
      nodes[num_symbols].ChildA = (HuffmanEncodeNode *) 0;
      nodes[num_symbols].ChildB = (HuffmanEncodeNode *) 0;
      ++ num_symbols;
    }
  }

  root = (HuffmanEncodeNode *) 0;
  nodes_left = num_symbols;
  next_idx = num_symbols;
  while( nodes_left > 1 )
  {
    node_1 = (HuffmanEncodeNode *) 0;
    node_2 = (HuffmanEncodeNode *) 0;
    for( k = 0; k < next_idx; ++ k )
    {
      if( nodes[k].Count > 0 )
      {
        if( !node_1 || (nodes[k].Count <= node_1->Count) )
        {
          node_2 = node_1;
          node_1 = &nodes[k];
        }
        else if( !node_2 || (nodes[k].Count <= node_2->Count) )
        {
          node_2 = &nodes[k];
        }
      }
    }

    root = &nodes[next_idx];
    root->ChildA = node_1;
    root->ChildB = node_2;
    root->Count = node_1->Count + node_2->Count;
    root->Symbol = -1;
    node_1->Count = 0;
    node_2->Count = 0;
    ++ next_idx;
    -- nodes_left;
  }

  if( root )
  {
    StoreHuffmanTree( root, sym, stream, 0, 0 );
  }
  else
  {
    root = &nodes[0];
    StoreHuffmanTree( root, sym, stream, 0, 1 );
  }
}

static HuffmanDecodeNode * RecoverHuffmanTree( HuffmanDecodeNode *nodes, BitStream *stream, unsigned int *nodenum )
{
  HuffmanDecodeNode * this_node;

  this_node = &nodes[*nodenum];
  *nodenum = *nodenum + 1;

  this_node->Symbol = -1;
  this_node->ChildA = (HuffmanDecodeNode *) 0;
  this_node->ChildB = (HuffmanDecodeNode *) 0;

  if( ReadBitFromBitStream( stream ) )
  {
    this_node->Symbol = ReadByteFromBitStream( stream );

    return this_node;
  }

  this_node->ChildA = RecoverHuffmanTree( nodes, stream, nodenum );

  this_node->ChildB = RecoverHuffmanTree( nodes, stream, nodenum );

  return this_node;
}

int Compress( unsigned char *InputFileByteStream, unsigned char *OutputFileByteStream, unsigned int InputFileSize )
{
  freq_table sym[256], tmp;
  BitStream bitstream;
  unsigned int k, total_bytes, swaps, symbol;

  if( InputFileSize < 1 )
    return 0;

  InitBitStream( &bitstream, OutputFileByteStream );

  CreateFreq_Table( InputFileByteStream, sym, InputFileSize );

  CreateHuffmanTree( sym, &bitstream );

  do
  {
    swaps = 0;
    for( k = 0; k < 255; ++ k )
    {
      if( sym[k].Symbol > sym[k+1].Symbol )
      {
        tmp      = sym[k];
        sym[k]   = sym[k+1];
        sym[k+1] = tmp;
        swaps    = 1;
      }
    }
  }
  while( swaps );

  for( k = 0; k < InputFileSize; ++ k )
  {
    symbol = InputFileByteStream[k];
    WriteBitsToBitStream( &bitstream, sym[symbol].Code, sym[symbol].Bits );
  }

  total_bytes = (int)(bitstream.BytePtr - OutputFileByteStream);
  if( bitstream.BitPos > 0 )
  {
    ++ total_bytes;
  }

  return total_bytes;
}

void UnCompress( unsigned char *in, unsigned char *out, unsigned int insize, unsigned int outsize )
{
  HuffmanDecodeNode nodes[MAX_TREE_NODES], *root, *node;
  BitStream stream;
  unsigned int      k, node_count;
  unsigned char     *buf;

  if( insize < 1 ) return;

  InitBitStream( &stream, in );

  node_count = 0;
  root = RecoverHuffmanTree( nodes, &stream, &node_count );

  buf = out;
  for( k = 0; k < outsize; ++ k )
  {
    node = root;
    while( node->Symbol < 0 )
    {
      if( ReadBitFromBitStream( &stream ) )
        node = node->ChildB;
      else
        node = node->ChildA;
    }

    *buf ++ = (unsigned char) node->Symbol;
  }
}
