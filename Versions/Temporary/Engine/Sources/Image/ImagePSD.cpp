#include "StdAfx.h"
#include "ImagePSD.h" 

enum EStreamSeek
{
	E_STREAM_SEEK_SET	= 0,
	E_STREAM_SEEK_CUR	= 1,
	E_STREAM_SEEK_END	= 2
};

namespace NImage
{
	// Storage type for RLE-lines-like data
	typedef vector<char> CData1D;

	// Storage type for RLE-channel-like data
	typedef vector<CData1D> CData2D;

	// Storage type for RLE-image-like data
	typedef vector<CData2D> CData3D;

	class CReadPSD
	{
		// Class for reading special PSD data
	private:
		CDataStream *pPSDfile;

	public:
		// Class Constructor
		CReadPSD( CDataStream *pStream )
		{
			pPSDfile = pStream;
		}// End of Constructor definition

		// Some default wrappers
		bool Seek( long offset, EStreamSeek origin )
		{
			switch ( origin )
			{
				case E_STREAM_SEEK_SET:
					return pPSDfile->Seek( offset );
				case E_STREAM_SEEK_CUR:
					return pPSDfile->Seek( pPSDfile->GetPosition() + offset );
				case E_STREAM_SEEK_END:
					return pPSDfile->Seek( pPSDfile->GetSize() + offset );
				default:
					NI_ASSERT( false, StrFmt( "unknown seek origin %d", (int)origin ) );
					return false;
			}
		}// End of Seek definition

		// PSD-specific byte-order readers
		int Get8() 
		{
			int i;

			pPSDfile->Read( &i, 4 );
			Seek( -3, E_STREAM_SEEK_CUR );

			return i & 0x000000FF;
		}// End of Get8 definition

		int Get16() 
		{
			int hi = Get8();
			int lo = Get8();

			return ( hi << 8 ) | lo;
		}// End of Get16 definition

		int Get32() 
		{
			int a = Get8();
			int b = Get8();
			int c = Get8();
			int d = Get8();

			return ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
		}// End of Get32 definition

		// Data readers
		CData2D GetRAWimage( int nChannels, int nHeight, int nWidth )
		{
			// Welcome, STORAGE!!!
			CData2D img;
			int nSize = nHeight * nWidth;

			// Resizing to fit actual data

			// Space for Channels
			img.resize( nChannels );
			for ( int i = 0; i < nChannels; ++i )
			{
				// Space for Lines
				img[i].resize( nSize );
				// Reading Data
				pPSDfile->Read( &img[i][0], nSize );
			}

			return img;
		}// End of GetRAWimage definition

		CData2D GetRLEimage( int nChannels, int nHeight, int nWidth )
		{
			// Welcome, STORAGE!!!
			CData3D src3D; // For reading RLE data
			CData2D dst2D; // For uncompressed data 

			// Resizing to fit actual RLE-data

			// Space for Channels
			src3D.resize( nChannels );

			for ( int i = 0; i < nChannels; ++i)
			{
				// Space for RLE-lines
				src3D[i].resize( nHeight );

				for ( int j = 0; j < nHeight; ++j )
				{
					// Space for RLE-data
					int nLineSize = Get16();
					src3D[i][j].resize( nLineSize );
					// Attention! Cannot read data right here, cause RLE-lines lenghths 
					// stored separately from RLE-lines data. 
					// (I've just made an attempt to do so.. ;))
				}
			}

			// Reading RLE data and preparing storage for uncompressed data
			dst2D.resize( nChannels );

			for ( int i = 0; i < nChannels; ++i )
			{
				// We'll use push_back method to fill the vector, 
				// so we need some reset..
				dst2D[i].resize( 0 ); 

				for ( int j = 0; j < nHeight; ++j )
				{
					int nLineSize = src3D[i][j].size();
					pPSDfile->Read( &src3D[i][j][0], nLineSize );
				}
			}

			// Now we have src3D with RLE data and an empty, 
			// properly sized dst2D for storing uncompressed data!
			// Let's go bowling! 8) ... Oh, I mean - do some decoding.. ;)

			for ( int i = 0; i < nChannels; ++i )
			{
				for ( int j = 0; j < nHeight; ++j )
				{
					CData1D::iterator itRLE = src3D[i][j].begin();

					do
					{
						char cRLEcount = *itRLE;
						if ( cRLEcount >= 0 )
						{
							for ( int k = 0; k <= cRLEcount; ++k )
							{
								++itRLE;
								dst2D[i].push_back( *itRLE );
							}// End of "k"-loop
							++itRLE;
						}
						else
						{
							++itRLE;
							for ( int k = 0; k <= abs( cRLEcount ); ++k )
							{
								dst2D[i].push_back( *itRLE );
							}// End of "k"-loop
							++itRLE;
						}
					} while ( itRLE != src3D[i][j].end() );
				}// End of "j"-loop
			}// End of "i"-loop

			return dst2D;

		}// End of GetRLEimage definition

	};// End of CReadPSD definition

	int NImage::RecognizeFormatPSD( CDataStream *pStream )
	{
		// 0 - Not PSD-image at all
		// 1 - Valid PSD-image (RGB, 8-bit per channel)
		// 2 - Not valid PSD-image (rather, than RGB, 8-bit per channel)

		// Create PSD reader and attach it to the stream
		CReadPSD rPS( pStream );
		int nTmp = 0; // Storage for miscelaneous header data

		// Main test - is this file PSD-type at all?
		rPS.Seek( 0, E_STREAM_SEEK_SET );	// Rewind file

		nTmp = rPS.Get32();
		if ( nTmp != 0x38425053 )	// File should start with "8BPS" - Official PSD signature
			return 0;

		nTmp = rPS.Get32();
		if ( nTmp != 0x00010000 )	// Then - should be 0x00000100
			return 0;

		nTmp = rPS.Get32();			
		if ( nTmp != 0x00000000 )	// Then - should be 0x00000000
			return 0;

		// Ok, this file is realy PSD.

		// But!
        
		// Is it 8-bit per channel?
		rPS.Seek( 22, E_STREAM_SEEK_SET ); // Rewind file

		nTmp = rPS.Get16();
		if ( nTmp != 8 )
			return 2;

		// And is it in RGB mode?
		nTmp = rPS.Get16();
		if ( nTmp != 3 )
			return 2;

		// Ok, then!
		return 1;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool LoadImagePSD( CArray2D<DWORD> &pRes, CDataStream *pStream )
	{
		// Create PSD reader and attach it to the stream
		CReadPSD rPS( pStream );

		rPS.Seek( 12, E_STREAM_SEEK_SET ); // Rewind to actual Header Data

		int nChannels = rPS.Get16();
		int nHeight = rPS.Get32();
		int nWidth = rPS.Get32();
		int nSize = nWidth * nHeight;

		rPS.Seek( 4, E_STREAM_SEEK_CUR ); // Rewind to the end of Header Section

		int tmp; // Tmp storage for reading size data and rewinding

		// Rewinding to image data
		tmp = rPS.Get32();
		rPS.Seek( tmp, E_STREAM_SEEK_CUR ); // Rewind Color Mode Section

		tmp = rPS.Get32();
		rPS.Seek( tmp, E_STREAM_SEEK_CUR ); // Rewind Image Resources Section

		tmp = rPS.Get32();
		rPS.Seek( tmp, E_STREAM_SEEK_CUR ); // Rewind Layer and Mask Section

		// Finally, we are at the beginning of the Image Data Section, 
		// where all channels are stored!!! Let's make STORAGE...
		CData2D PSimg2D;

		// ...and fill it with a huge ammount of beautiful pixels!!
		if ( rPS.Get16() ) // Test compression
		{
			// Read RLE image
			PSimg2D = rPS.GetRLEimage( nChannels, nHeight, nWidth );
		}
		else
		{
			// Read RAW image
			PSimg2D = rPS.GetRAWimage( nChannels, nHeight, nWidth );
		}

		vector<DWORD> image;
		image.resize( nSize );

		for ( int i = 0; i < nSize; ++i )
		{
			DWORD r = PSimg2D[0][i] & 0x000000FF;
			DWORD g = PSimg2D[1][i] & 0x000000FF;
			DWORD b = PSimg2D[2][i] & 0x000000FF;

			DWORD a = 0x000000FF;

			if ( nChannels > 3 )
			{
				a = PSimg2D[3][i] & 0x000000FF;
			}

			image[i] = ( a << 24) | (r << 16) | (g << 8) | b;
		}

		pRes.SetSizes( nWidth, nHeight );
		memcpy( &( pRes[0][0] ), &image[0], sizeof ( image[0] ) * nSize );

		return true;
	}

}


