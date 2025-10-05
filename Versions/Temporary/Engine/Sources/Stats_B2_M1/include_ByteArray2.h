int GetSizeX() const { return data.empty() ? 0 : data[0].data.size(); }
int GetSizeY() const { return data.size(); }

const std::vector<int> &operator[]( const int nIndex ) const { return data[nIndex].data; }
std::vector<int> &operator[]( const int nIndex ) { return data[nIndex].data; }
void SetSizes( const int nSizeX, const int nSizeY )
{
	data.resize( nSizeY );
	for ( int i = 0; i < nSizeY; ++i )
		data[i].data.resize( nSizeX );
}

bool IsEmpty() const { return data.empty(); }

void PostLoad( bool bInEditor )
{
	// CRAP{ due to incorrect data in database (stored in [X][Y] ordering instead of [Y][X])
	const int nSizeX = GetSizeX();
	if ( nSizeX > 0 )
	{
		// first, correct size
		for ( int i = 0; i < data.size(); ++i )
			data[i].data.resize( nSizeX );
		// second (for non-in-editor (game) case), transpose 2d array
		if ( !bInEditor )
		{
			const int nSizeY = GetSizeY();
			std::vector<SByteArray1> newData( nSizeX );
			for ( int i = 0; i < nSizeX; ++i )
				newData[i].data.resize( nSizeY );
			//
			for ( int i = 0; i < nSizeX; ++i )
			{
				for ( int j = 0; j < nSizeY; ++j )
					newData[i].data[j] = data[j].data[i];
			}
			newData.swap( data );
		}
	}
	// CRAP}
}
