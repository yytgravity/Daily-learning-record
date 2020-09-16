#include<stdio.h>
#include<Windows.h>
#pragma comment(lib, "Msimg32.lib") 

int main()
{
	BYTE *pBit = (BYTE *)malloc(16 * 16 * 4);//alloca byte array
	BITMAPINFO bi;
	HDC hdcemf,hdcmem;
	HENHMETAFILE  hemf;
	HBITMAP hbmp = NULL;
	BITMAP bmp;
	int ret;
	int result;
	DWORD color[16];
	RECT rect = { 0, 0, 100, 100 };
	bi.bmiHeader.biSize = 0x28;
	bi.bmiHeader.biWidth = 0x10;
	bi.bmiHeader.biHeight = 0x10;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 0x18;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;
	void *pdata = malloc(0x100);
	
	hdcemf = CreateEnhMetaFile(NULL, TEXT("fuzz_17.emf"), NULL, TEXT("ffwefefef77\0 ddwdd\0"));
	hbmp = LoadBitmap(NULL, MAKEINTRESOURCE(2));
	hdcmem = CreateCompatibleDC(hdcemf);
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	/*
	StretchBlt(hdcemf, 100, 100, 100, 100, hdcmem, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
	SetDIBitsToDevice(hdcemf,100,100,100,100,10,10,10,10,pdata,&bi, DIB_RGB_COLORS);
	StretchDIBits(hdcemf, 100, 100, 100, 100, 100, 100, 10, 10, pdata, &bi, DIB_RGB_COLORS, SRCCOPY);
	*/

	/*
	static int w = 200;
	static int h = 100;
	static BLENDFUNCTION bf = { 0 };

	AlphaBlend(hdcemf, 0, 0, w, h, hdcmem, 0, 0, w, h, bf);
	*/

	/*	AngleArc(hdcemf, 10, 10, 5, 10, 20);*/

	//Arc(hdcemf,10,20,30,35,41,43,50,50);

	/*
	AbortPath(hdcemf);
	BeginPath(hdcemf);
	EndPath(hdcemf);
	FlattenPath(hdcemf);
	WidenPath(hdcemf);
	*/
	/*
	LPLOGPALETTE lpPaletteNew = NULL;
	HPALETTE hpalNew = NULL;
	hpalNew = CreatePalette(lpPaletteNew);
	ColorCorrectPalette(hdcemf, hpalNew, 1, 5);
	*/

	/*
	MaskBlt(hdcemf, 100, 100, 50, 10, hdcmem, 100, 100, hbmp, 10, 10, 0);
	*/

	/*
	POINT point[3];
	point[0].x = 0;
	point[0].y = 10;
	point[1].x = 0;
	point[1].y = 0;
	point[2].x = 10;
	point[2].y = 10;

	PlgBlt(hdcemf, point, hdcmem, 0, 0, 10, 10, 0, 0, 0);
	*/


	//BitBlt(hdcemf, 0, 0, 10, 10, hdcmem, 0, 0, SRCCOPY);

	TransparentBlt(hdcemf, 0, 0, 218, 199, hdcmem, 0, 0, 218, 199, RGB(0, 0, 0xff));

	DeleteDC(hdcmem);
	DeleteObject(hbmp);
	hemf = CloseEnhMetaFile(hdcemf);
	DeleteEnhMetaFile(hemf);
	free(pdata);
	return 0;
}