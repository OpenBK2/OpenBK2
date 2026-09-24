#include "MapEditorLib/stdafx.h"
#include "MapEditorLib/WxImageList.h"
#include <gtest/gtest.h>

TEST( ThumbnailAlpha, PreservesTransparentGreyAndOpaqueBlack )
{
	CArray2D<uint32_t> pixels(3, 1);
	pixels[0][0] = 0x00808080; // grey RGB is hidden by source alpha, not a colour key
	pixels[0][1] = 0xff000000; // black details must remain opaque
	pixels[0][2] = 0x80402010; // retain smooth, partially transparent edges too
	const wxImage image = NWxImageList::ToWxImage(pixels);
	ASSERT_TRUE(image.IsOk());
	ASSERT_TRUE(image.HasAlpha());
	EXPECT_EQ(image.GetAlpha(0, 0), 0);
	EXPECT_EQ(image.GetAlpha(1, 0), 255);
	EXPECT_EQ(image.GetAlpha(2, 0), 128);
	EXPECT_EQ(image.GetRed(2, 0), 0x40);
	EXPECT_EQ(image.GetGreen(2, 0), 0x20);
	EXPECT_EQ(image.GetBlue(2, 0), 0x10);
}

TEST( ThumbnailAlpha, FullyTransparentImagesStayTransparent )
{
	CArray2D<uint32_t> pixels(2, 2);
	pixels.FillEvery(0x00808080);
	const wxImage image = NWxImageList::ToWxImage(pixels);
	for ( int y = 0; y != 2; ++y )
		for ( int x = 0; x != 2; ++x ) EXPECT_EQ(image.GetAlpha(x, y), 0);
	EXPECT_FALSE(NWxImageList::ToWxImage(CArray2D<uint32_t>()).IsOk());
}
