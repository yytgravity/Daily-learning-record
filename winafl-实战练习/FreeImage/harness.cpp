#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <tchar.h>
using namespace std;

extern "C" __declspec(dllexport) int main(int argc, char** argv);

typedef DWORD(__stdcall* FreeImage_GetFileTypeU)(const wchar_t* IpszPathName, int flag);
typedef DWORD(__stdcall* FreeImage_Initialise)(BOOL load_local_plugins_only);
typedef DWORD(__stdcall* FreeImage_Delnitialise)();
typedef DWORD(__stdcall* FreeImage_LoadU)(DWORD format, const wchar_t* IpszPathName, int flag);
typedef DWORD(__stdcall* FreeImage_UnLoad)(DWORD dib);

FreeImage_Delnitialise Delnitialise;
FreeImage_Initialise Initialise;
FreeImage_GetFileTypeU GetFileTypeU;
FreeImage_LoadU LoadU;
DWORD load;
FreeImage_UnLoad UnLoad;

wchar_t* char2WCHAR(const char* text)
{
	size_t size = strlen(text)+1;
	wchar_t* wc = new wchar_t[size];
	mbstowcs(wc,text,size);
	return wc;
}

void FreeImage_test(HINSTANCE hinstLib, wchar_t* pathfile)
{
	(Initialise)(FALSE);
	DWORD FileType = GetFileTypeU(pathfile,0);
	load = (LoadU)(FileType, pathfile, 0);
	(UnLoad)(load);
	(Delnitialise)();
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: %s<xml file>\n",argv[0]);
		return 0;
	}
	wchar_t* PathName = char2WCHAR(argv[1]);
	HINSTANCE hinstlib;
	BOOL fFreeResult,fRunTimeLinkSuccess = FALSE;
	DWORD Error = NULL;

	hinstlib = LoadLibrary(TEXT("D:\\sakura\\abckantu\\ABC\\FreeImage.dll"));

	if (hinstlib != NULL)
	{
		fRunTimeLinkSuccess = TRUE;
		Initialise = (FreeImage_Initialise)GetProcAddress(hinstlib, ¡°FreeImage_Initialise¡±);
		GetFileTypeU = (FreeImage_GetFileTypeU)GetProcAddress(hinstlib, ¡±FreeImage_GetFileTypeU¡°);
		LoadU = (FreeImage_LoadU)GetProcAddress(hinstlib, ¡°FreeImage_LoadU¡±);
		UnLoad = (FreeImage_UnLoad)GetProcAddress(hinstlib, ¡°FreeImage_UnLoad¡±);
		Delnitialise = (FreeImage_Delnitialise)GetProcAddress(hinstlib, ¡±FreeImage_Delnitialise¡°);

		FreeImage_test(hinstlib, PathName);
		fFreeResult = FreeLibrary(hinstlib);
	}

	if (fRunTimeLinkSuccess = FALSE)
	{
		cout << "¼ÓÔØÊ§°Ü£¬ERROR£º" << Error << endl;
	}
	return 0;
}
