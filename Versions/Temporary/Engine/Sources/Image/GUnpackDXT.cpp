#include "stdafx.h"
#include "GUnpackDXT.h"

#include <cstdint>

namespace NImage
{
struct SDDSHeader
{
	uint32_t dwWidth, dwHeight;
};

// ************************************************************************************************************************ //
// **
// ** DXT subformats decoding
// **
// **
// **
// ************************************************************************************************************************ //

struct SDXTColBlock
{
	uint16_t col0;
	uint16_t col1;
	// no bit fields - use bytes
	uint8_t row[4];
};

struct SDXTAlphaBlockExplicit
{
	uint16_t row[4];
};

struct SDXTAlphaBlock3BitLinear
{
	uint8_t alpha0;
	uint8_t alpha1;

	uint8_t stuff[6];
};
// use cast to struct instead of RGBA_MAKE as struct is much
struct SColor8888
{
	uint8_t b;		//  Last one is MSB, 1st is LSB.
	uint8_t g;		//  order of the output ARGB or BGRA, etc...
	uint8_t r;		// change the order of names to change the
	uint8_t a;
};

struct SColor565
{
	unsigned nBlue  : 5;		// order of names changes
	unsigned nGreen : 6;		// byte order of output to 32 bit
	unsigned nRed	: 5;
};

inline void GetColorBlockColors( SDXTColBlock *pBlock, SColor8888 *col_0, SColor8888 *col_1, 
																 SColor8888 *col_2, SColor8888 *col_3, uint16_t &wrd )
{
	// There are 4 methods to use - see the Time_ functions.
	// 1st = shift = does normal approach per byte for color comps
	// 2nd = use freak variable bit field SColor565 for component extraction
	// 3rd = use super-freak uint32_t adds BEFORE shifting the color components
	//  This lets you do only 1 add per color instead of 3 uint8_t adds and
	//  might be faster
	// Call RunTimingSession() to run each of them & output result to txt file

 
	// freak variable bit structure method
	// normal math
	// This method is fastest

	SColor565 *pCol = (SColor565*) &(pBlock->col0);

	col_0->a = 0xff;
	col_0->r = pCol->nRed;
	col_0->r <<= 3;				// shift to full precision
	col_0->g = pCol->nGreen;
	col_0->g <<= 2;
	col_0->b = pCol->nBlue;
	col_0->b <<= 3;

	pCol = (SColor565*) & (pBlock->col1 );
	col_1->a = 0xff;
	col_1->r = pCol->nRed;
	col_1->r <<= 3;				// shift to full precision
	col_1->g = pCol->nGreen;
	col_1->g <<= 2;
	col_1->b = pCol->nBlue;
	col_1->b <<= 3;


	if ( pBlock->col0 > pBlock->col1 )
	{
		// Four-color block: derive the other two colors.    
		// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
		// These two bit codes correspond to the 2-bit fields 
		// stored in the 64-bit block.

		wrd = ((uint16_t)col_0->r * 2 + (uint16_t)col_1->r )/3;
											// no +1 for rounding
											// as bits have been shifted to 888
		col_2->r = (uint8_t)wrd;

		wrd = ((uint16_t)col_0->g * 2 + (uint16_t)col_1->g )/3;
		col_2->g = (uint8_t)wrd;

		wrd = ((uint16_t)col_0->b * 2 + (uint16_t)col_1->b )/3;
		col_2->b = (uint8_t)wrd;
		col_2->a = 0xff;

		wrd = ((uint16_t)col_0->r + (uint16_t)col_1->r *2 )/3;
		col_3->r = (uint8_t)wrd;

		wrd = ((uint16_t)col_0->g + (uint16_t)col_1->g *2 )/3;
		col_3->g = (uint8_t)wrd;

		wrd = ((uint16_t)col_0->b + (uint16_t)col_1->b *2 )/3;
		col_3->b = (uint8_t)wrd;
		col_3->a = 0xff;

	}
	else
	{
		// Three-color block: derive the other color.
		// 00 = color_0,  01 = color_1,  10 = color_2,  
		// 11 = transparent.
		// These two bit codes correspond to the 2-bit fields 
		// stored in the 64-bit block. 

		// explicit for each component, unlike some refrasts...
		
		// TRACE("block has alpha\n");

		wrd = ((uint16_t)col_0->r + (uint16_t)col_1->r )/2;
		col_2->r = (uint8_t)wrd;
		wrd = ((uint16_t)col_0->g + (uint16_t)col_1->g )/2;
		col_2->g = (uint8_t)wrd;
		wrd = ((uint16_t)col_0->b + (uint16_t)col_1->b )/2;
		col_2->b = (uint8_t)wrd;
		col_2->a = 0xff;

		// adding random to unpacking is stupid, guys!
		// this color should be black!
		col_3->r = 0x00;		// random color to indicate alpha
		col_3->g = 0x00;
		col_3->b = 0x00;
		col_3->a = 0x00;

	}
}			//  Get color block colors (...)

inline void DecodeColorBlock( uint32_t *pImPos, SDXTColBlock *pColorBlock, int width,
								              uint32_t *col_0, uint32_t *col_1, uint32_t *col_2, uint32_t *col_3 )
{
	// width is width of image in pixels
	uint32_t bits;
	int r,n;

	// bit masks = 00000011, 00001100, 00110000, 11000000
	const uint32_t masks[] = { 3, 12, 3 << 4, 3 << 6 };
	const int   shift[] = { 0, 2, 4, 6 };

	// r steps through lines in y
	for ( r = 0; r < 4; ++r, pImPos += width - 4 )	// no width*4 as uint32_t ptr inc will *4
	{

		// width * 4 bytes per pixel per line
		// each j dxtc row is 4 lines of pixels

		// pImPos = (uint32_t*)((uint32_t)pBase + i*16 + (r+j*4) * hdr.dwWidth * 4 );

		// n steps through pixels
		for ( n = 0; n < 4; ++n )
		{
			bits =		pColorBlock->row[r] & masks[n];
			bits >>=	shift[n];

			switch( bits )
			{
			case 0 :
				*pImPos = *col_0;
				pImPos++;		// increment to next uint32_t
				break;
			case 1 :
				*pImPos = *col_1;
				pImPos++;
				break;
			case 2 :
				*pImPos = *col_2;
				pImPos++;
				break;
			case 3 :
				*pImPos = *col_3;
				pImPos++;
				break;
			default:
				DebugTrace( "Your logic is jacked! bits == 0x%x\n", bits );
				pImPos++;
				break;
			}
		}
	}
}

inline void  DecodeAlphaExplicit( uint32_t *pImPos, SDXTAlphaBlockExplicit *pAlphaBlock, int width, uint32_t alphazero )
{
	// alphazero is a bit mask that when & with the image color
	//  will zero the alpha bits, so if the image DWORDs  are
	//  ARGB then alphazero will be 0x00ffffff or if
	//  RGBA then alphazero will be 0xffffff00
	//  alphazero constructed automaticaly from field order of SColor8888 structure

	// decodes to 32 bit format only
	int row, pix;

	uint16_t wrd;

	SColor8888 col;
	col.r = col.g = col.b = 0;

	for ( row = 0; row < 4; ++row, pImPos += width - 4 )
	{
		// pImPow += pImPos += width-4 moves to next row down

		wrd = pAlphaBlock->row[ row ];

		for( pix = 0; pix < 4; pix++ )
		{
			// zero the alpha bits of image pixel
			*pImPos &= alphazero;

			col.a = wrd & 0x000f;		// get only low 4 bits
//			col.a <<= 4;				// shift to full byte precision
										// NOTE:  with just a << 4 you'll never have alpha
										// of 0xff,  0xf0 is max so pure shift doesn't quite
										// cover full alpha range.
										// It's much cheaper than divide & scale though.
										// To correct for this, and get 0xff for max alpha,
										//  or the low bits back in after left shifting
			col.a = col.a | (col.a << 4 );	// This allows max 4 bit alpha to be 0xff alpha
											//  in final image, and is crude approach to full 
											//  range scale

			*pImPos |= *((uint32_t*)&col);	// or the bits into the prev. nulled alpha

			wrd >>= 4;		// move next bits to lowest 4

			pImPos++;		// move to next pixel in the row

		}
	}
}

inline void DecodeAlpha3BitLinear( uint32_t * pImPos, SDXTAlphaBlock3BitLinear * pAlphaBlock,
									int width, uint32_t alphazero)
{
	static uint8_t s_Bits[4][4];
	static uint16_t s_Alphas[8];
	static SColor8888 s_ACol[4][4];

	s_Alphas[0] = pAlphaBlock->alpha0;
	s_Alphas[1] = pAlphaBlock->alpha1;

	
	// 8-alpha or 6-alpha block?    

	if( s_Alphas[0] > s_Alphas[1] )
	{
		// 8-alpha block:  derive the other 6 alphas.    
		// 000 = alpha_0, 001 = alpha_1, others are interpolated

		s_Alphas[2] = ( 6 * s_Alphas[0] +     s_Alphas[1]) / 7;	// bit code 010
		s_Alphas[3] = ( 5 * s_Alphas[0] + 2 * s_Alphas[1]) / 7;	// Bit code 011    
		s_Alphas[4] = ( 4 * s_Alphas[0] + 3 * s_Alphas[1]) / 7;	// Bit code 100    
		s_Alphas[5] = ( 3 * s_Alphas[0] + 4 * s_Alphas[1]) / 7;	// Bit code 101
		s_Alphas[6] = ( 2 * s_Alphas[0] + 5 * s_Alphas[1]) / 7;	// Bit code 110    
		s_Alphas[7] = (     s_Alphas[0] + 6 * s_Alphas[1]) / 7;	// Bit code 111
	}    
	else
	{
		// 6-alpha block:  derive the other alphas.    
		// 000 = alpha_0, 001 = alpha_1, others are interpolated

		s_Alphas[2] = (4 * s_Alphas[0] +     s_Alphas[1]) / 5;	// Bit code 010
		s_Alphas[3] = (3 * s_Alphas[0] + 2 * s_Alphas[1]) / 5;	// Bit code 011    
		s_Alphas[4] = (2 * s_Alphas[0] + 3 * s_Alphas[1]) / 5;	// Bit code 100    
		s_Alphas[5] = (    s_Alphas[0] + 4 * s_Alphas[1]) / 5;	// Bit code 101
		s_Alphas[6] = 0;										// Bit code 110
		s_Alphas[7] = 255;									// Bit code 111
	}


	// Decode 3-bit fields into array of 16 BYTES with same value

	// first two rows of 4 pixels each:
	// pRows = (Alpha3BitRows*) & ( pAlphaBlock->stuff[0] );
	const uint32_t mask = 0x00000007;		// bits = 00 00 01 11

	uint32_t bits = *( (uint32_t*) & ( pAlphaBlock->stuff[0] ));

	s_Bits[0][0] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[0][1] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[0][2] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[0][3] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[1][0] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[1][1] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[1][2] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[1][3] = (uint8_t)( bits & mask );

	// now for last two rows:

	bits = *( (uint32_t*) & ( pAlphaBlock->stuff[3] ));		// last 3 bytes

	s_Bits[2][0] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[2][1] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[2][2] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[2][3] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[3][0] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[3][1] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[3][2] = (uint8_t)( bits & mask );
	bits >>= 3;
	s_Bits[3][3] = (uint8_t)( bits & mask );


	// decode the codes into alpha values
	int row, pix;


	for( row = 0; row < 4; row++ )
	{
		for( pix=0; pix < 4; pix++ )
		{
			s_ACol[row][pix].a = (uint8_t) s_Alphas[ s_Bits[row][pix] ];

			ASSERT( s_ACol[row][pix].r == 0 );
			ASSERT( s_ACol[row][pix].g == 0 );
			ASSERT( s_ACol[row][pix].b == 0 );
		}
	}



	// Write out alpha values to the image bits

	for( row=0; row < 4; row++, pImPos += width-4 )
	{
		// pImPow += pImPos += width-4 moves to next row down

		for( pix = 0; pix < 4; pix++ )
		{
			// zero the alpha bits of image pixel
			*pImPos &=  alphazero;

			*pImPos |=  *((uint32_t*) &(s_ACol[row][pix]));	// or the bits into the prev. nulled alpha
			pImPos++;
		}
	}
}

void DecompressDXT1( uint32_t *pRes, const SDDSHeader &hdr, const uint8_t *pCompBytes )
{
	// This was hacked up pretty quick & slopily
	// decompresses to 32 bit format 0xARGB

	const int xblocks = hdr.dwWidth / 4;
	const int yblocks = hdr.dwHeight / 4;
	uint32_t *pBase  = (uint32_t*)pRes;
	uint32_t *pImPos = (uint32_t*)pBase;			// pos in decompressed data
//	uint16_t  *pPos   = (uint16_t*)pCompBytes;	// pos in compressed data
	int i,j;

	SDXTColBlock *pBlock;

	SColor8888 col_0, col_1, col_2, col_3;

	uint16_t wrd;

	for( j = 0; j < yblocks; ++j )
	{
		// 8 bytes per block
		pBlock = (SDXTColBlock*) ( (uintptr_t)pCompBytes + j * xblocks * 8 );


		for( i=0; i < xblocks; i++, pBlock++ )
		{

			// inline func:
			GetColorBlockColors( pBlock, &col_0, &col_1, &col_2, &col_3, wrd );


			// now decode the color block into the bitmap bits
			// inline func:

			pImPos = (uint32_t*)((uintptr_t)pBase + i*16 + (j*4) * hdr.dwWidth * 4 );


			DecodeColorBlock( pImPos, pBlock, hdr.dwWidth, (uint32_t*)&col_0, (uint32_t*)&col_1,
												(uint32_t*)&col_2, (uint32_t*)&col_3 );
		}
	}
}

void DecompressDXT3( uint32_t *pRes, const SDDSHeader &hdr, const uint8_t *pCompBytes )
{
	const int xblocks = hdr.dwWidth / 4;
	const int yblocks = hdr.dwHeight / 4;
	uint32_t *pBase  = (uint32_t*)pRes;
	uint32_t *pImPos = (uint32_t*)pBase;			// pos in decompressed data
//	uint16_t  *pPos   = (uint16_t*)pCompBytes;	// pos in compressed data
	int i,j;

	SDXTColBlock *pBlock;
	SDXTAlphaBlockExplicit *pAlphaBlock;

	SColor8888 col_0, col_1, col_2, col_3;


	uint16_t wrd;

	// fill alphazero with appropriate value to zero out alpha when
	//  alphazero is ANDed with the image color 32 bit uint32_t:
	col_0.a = 0;
	col_0.r = col_0.g = col_0.b = 0xff;
	uint32_t alphazero = *((uint32_t*) &col_0);



	//	TRACE("blocks: x: %d    y: %d\n", xblocks, yblocks );

	for ( j = 0; j < yblocks; ++j )
	{
		// 8 bytes per block
		// 1 block for alpha, 1 block for color

		pBlock = (SDXTColBlock*) ( (uintptr_t)pCompBytes + j * xblocks * 16 );

		for ( i = 0; i < xblocks; i++, pBlock++ )
		{

			// inline
			// Get alpha block

			pAlphaBlock = (SDXTAlphaBlockExplicit*) pBlock;

			// inline func:
			// Get color block & colors
			pBlock++;
			GetColorBlockColors( pBlock, &col_0, &col_1, &col_2, &col_3, wrd );

			// Decode the color block into the bitmap bits
			// inline func:

			pImPos = (uint32_t*)((uintptr_t)pBase + i*16 + (j*4) * hdr.dwWidth * 4 );


			DecodeColorBlock( pImPos, pBlock, hdr.dwWidth, (uint32_t*)&col_0, (uint32_t*)&col_1,
								(uint32_t*)&col_2, (uint32_t*)&col_3 );

			// Overwrite the previous alpha bits with the alpha block
			//  info
			// inline func:
			DecodeAlphaExplicit( pImPos, pAlphaBlock, hdr.dwWidth, alphazero );


		}
	}
}

void DecompressDXT2( uint32_t *pRes, const SDDSHeader &hdr, const uint8_t *pCompBytes )
{
	// Can do color & alpha same as dxt3, but color is pre-multiplied 
	//   so the result will be wrong unless corrected. 
	DecompressDXT3( pRes, hdr, pCompBytes );
}

void DecompressDXT5( uint32_t *pRes, const SDDSHeader &hdr, const uint8_t *pCompBytes )
{
	const int xblocks = hdr.dwWidth / 4;
	const int yblocks = hdr.dwHeight / 4;
	uint32_t *pBase  = (uint32_t*)pRes;
	uint32_t *pImPos = (uint32_t*)pBase;			// pos in decompressed data
//	uint16_t  *pPos   = (uint16_t*)pCompBytes;	// pos in compressed data
	int i,j;

	SDXTColBlock *pBlock;
	SDXTAlphaBlock3BitLinear *pAlphaBlock;

	SColor8888 col_0, col_1, col_2, col_3;
	uint16_t wrd;

	// fill alphazero with appropriate value to zero out alpha when
	//  alphazero is ANDed with the image color 32 bit uint32_t:
	col_0.a = 0;
	col_0.r = col_0.g = col_0.b = 0xff;
	uint32_t alphazero = *((uint32_t*) &col_0);

	////////////////////////////////
	//	TRACE("blocks: x: %d    y: %d\n", xblocks, yblocks );

	for ( j = 0; j < yblocks; ++j )
	{
		// 8 bytes per block
		// 1 block for alpha, 1 block for color

		pBlock = (SDXTColBlock*) ( (uintptr_t)pCompBytes + j * xblocks * 16 );

		for ( i = 0; i < xblocks; i++, pBlock++ )
		{

			// inline
			// Get alpha block

			pAlphaBlock = (SDXTAlphaBlock3BitLinear*) pBlock;

			// inline func:
			// Get color block & colors
			pBlock++;

			// TRACE("pBlock:   0x%.8x\n", pBlock );

			GetColorBlockColors( pBlock, &col_0, &col_1, &col_2, &col_3, wrd );

			// Decode the color block into the bitmap bits
			// inline func:

			pImPos = (uint32_t*)((uintptr_t)pBase + i*16 + (j*4) * hdr.dwWidth * 4 );


			DecodeColorBlock( pImPos, pBlock, hdr.dwWidth, (uint32_t*)&col_0, (uint32_t*)&col_1,
								(uint32_t*)&col_2, (uint32_t*)&col_3 );

			// Overwrite the previous alpha bits with the alpha block
			//  info

			DecodeAlpha3BitLinear( pImPos, pAlphaBlock, hdr.dwWidth, alphazero );


		}
	}
}

void DecompressDXT4( uint32_t *pRes, const SDDSHeader &hdr, const uint8_t *pCompBytes )
{
	// Can do color & alpha same as dxt5, but color is pre-multiplied 
	//   so the result will be wrong unless corrected. 
	DecompressDXT5( pRes, hdr, pCompBytes );
}


void UnpackDXT( int nDxt, int nXSize, int nYSize, const void *pData, CArray2D<uint32_t> *pRes )
{
	SDDSHeader hdr;
	hdr.dwWidth = nXSize;
	hdr.dwHeight = nYSize;
	pRes->SetSizes( nXSize, nYSize );
	switch ( nDxt ) 
	{
	case 1:
		DecompressDXT1( (uint32_t*)&(*pRes)[0][0], hdr, (const uint8_t*)pData );
		break;
	case 2:
		DecompressDXT2( (uint32_t*)&(*pRes)[0][0], hdr, (const uint8_t*)pData );
		break;
	case 3:
		DecompressDXT3( (uint32_t*)&(*pRes)[0][0], hdr, (const uint8_t*)pData );
		break;
	case 4:
		DecompressDXT4( (uint32_t*)&(*pRes)[0][0], hdr, (const uint8_t*)pData );
		break;
	case 5:
		DecompressDXT5( (uint32_t*)&(*pRes)[0][0], hdr, (const uint8_t*)pData );
		break;
	}
}
}

