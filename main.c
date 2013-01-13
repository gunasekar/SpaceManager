#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"

int Read32BitHeader( FILE *f ){
    unsigned char buf[4];
    fread( buf, 4, 1, f );
    return  (((unsigned int)buf[0])<<24) +
            (((unsigned int)buf[1])<<16) +
            (((unsigned int)buf[2])<<8)  +
            (unsigned int)buf[3];
}

void Write32BitHeader( int x, FILE *f ){
    fputc( (x>>24)&255, f );
    fputc( (x>>16)&255, f );
    fputc( (x>>8)&255, f );
    fputc( x&255, f );
}

long GetFileSize( FILE *f ){
    long pos, size;
    pos = ftell( f );
    fseek( f, 0, SEEK_END );
    size = ftell( f );
    fseek( f, pos, SEEK_SET );
    return size;
}

void Help( char *prgname ){
    printf( "Usage: %s -Operation infile outfile\n\n", prgname );
    printf( "Operations:\n" );
    printf( "  c       Compress\n" );
    printf( "  d       Deompress\n\n" );
}

int main( int argc, char **argv ){
    FILE *f;
    unsigned char *InputFileByteStream, *OutputFileByteStream, Operation;
    unsigned int  InputFileSize, OutputFileSize=0, Comp_Check, Huff_Check;
    char *InputFileName, *OutputFileName;

    if( argc != 4 )
    {
        Help( argv[ 0 ] );
        return 0;
    }

    if(argv[1][0]=='-')
    Operation = argv[1][1];
    else{
        Help( argv[ 0 ] );
        return 0;
    }

    if( (Operation != 'c') && (Operation != 'd') )
    {
        Help( argv[ 0 ] );
        return 0;
    }

    InputFileName  = argv[ 2 ];
    OutputFileName = argv[ 3 ];

    f = fopen( InputFileName, "rb" );
    if( !f )
    {
        printf( "Unable to open input file \"%s\".\n", InputFileName );
        return 0;
    }

    InputFileSize = GetFileSize( f );

    if( Operation == 'd' )
    {
        Comp_Check = Read32BitHeader( f );
        Huff_Check = Read32BitHeader( f );
        if(Comp_Check != 12345 && Huff_Check != 67890)
        {
            printf("Input File is not the File compressed by this Compression Program!\nUnable to decompress it!\n");
            return 0;
        }
        OutputFileSize = Read32BitHeader( f );
        InputFileSize -= 12;
    }

    printf( "Huffman Encoding : \n" );
    switch( Operation )
    {
        case 'c': printf( "Compressing " ); break;
        case 'd': printf( "Decompressing " ); break;
    }
    printf( "%s to %s...\n", InputFileName, OutputFileName );

    printf( "Input file: %d bytes\n", InputFileSize );
    InputFileByteStream = (unsigned char *) malloc( InputFileSize );

    if( !InputFileByteStream )
    {
        printf( "Not enough memory\n" );
        fclose( f );
        return 0;
    }

    fread( InputFileByteStream, InputFileSize, 1, f );
    fclose( f );

    if( Operation == 'd' )
    {
        printf( "Output file: %d bytes\n", OutputFileSize );
    }

    f = fopen( OutputFileName, "wb" );
    if( !f )
    {
        printf( "Unable to open output file \"%s\".\n", OutputFileName );
        free( InputFileByteStream );
        return 0;
    }

    if( Operation == 'c' )
    {
        Write32BitHeader( 12345, f);
        Write32BitHeader( 67890, f );
        Write32BitHeader( InputFileSize, f );
        OutputFileSize = (InputFileSize*104+50)/100 + 384;
    }

    OutputFileByteStream = malloc( OutputFileSize );
    if( !OutputFileByteStream )
    {
        printf( "Not enough memory\n" );
        fclose( f );
        free( InputFileByteStream );
        return 0;
    }

    if(Operation == 'c'){
	OutputFileSize = Compress( InputFileByteStream, OutputFileByteStream, InputFileSize );
	printf( "Output file: %d bytes (%.1f%%)\n", OutputFileSize,100*(float)OutputFileSize/(float)InputFileSize );
    }
    else
    {
        UnCompress( InputFileByteStream, OutputFileByteStream, InputFileSize, OutputFileSize );
    }

    fwrite( OutputFileByteStream, OutputFileSize, 1, f );
    fclose( f );

    free( InputFileByteStream );
    free( OutputFileByteStream );

    return 0;
}
