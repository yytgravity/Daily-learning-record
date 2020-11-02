#include <iostream>
#include <windows.h>
#include <stdio.h>
#include "ras.h"
#include "rasdlg.h"
#include "raserror.h"
#pragma comment(lib, "rasapi32.lib")
#pragma comment(lib, "Rasdlg.lib")

extern "C" __declspec(dllexport) int main(int argc, char** argv);

int main(int argc, char** argv)
{
	DWORD dwCb = 0;
	DWORD dwRet = ERROR_SUCCESS;
	DWORD dwErr = ERROR_SUCCESS;
	DWORD dwErr1 = ERROR_SUCCESS;
	DWORD dwEntries = 0;
	LPRASENTRYNAME lpRasEntryName = NULL;
	DWORD rc;
	DWORD dwSize = 0;
	LPCSTR lpszPhonebook = argv[1];
	DWORD dwRasEntryInfoSize = 0;
	LPRASENTRY g_lpRasEntry=0;
	LPTSTR lpszEntry = "EntryName\0";
	LPTSTR lpszEntry1 = "EntryName222\0";

	//printf("main: %p\n", (void*)main);

	if (argc < 2) {
		printf("Usage: %s <bpk file>\n", argv[0]);
		return 0;
	}

	// Call RasEnumEntries with lpRasEntryName = NULL. dwCb is returned with the required buffer size and 
	// a return code of ERROR_BUFFER_TOO_SMALL
	dwRet = RasEnumEntriesA(NULL, lpszPhonebook, lpRasEntryName, &dwCb, &dwEntries);

	if (dwRet == ERROR_BUFFER_TOO_SMALL) 
	{
		// Allocate the memory needed for the array of RAS entry names.
		lpRasEntryName = (LPRASENTRYNAME)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
		if (lpRasEntryName == NULL) {
			//wprintf(L"HeapAlloc failed!\n");
			return 0;
		}
		// The first RASENTRYNAME structure in the array must contain the structure size
		lpRasEntryName[0].dwSize = sizeof(RASENTRYNAME);

		// Call RasEnumEntries to enumerate all RAS entry names
		dwRet = RasEnumEntries(NULL, lpszPhonebook, lpRasEntryName, &dwCb, &dwEntries);

		// If successful, print the RAS entry names 
		if (ERROR_SUCCESS == dwRet) {

			//printf("Number of Entries %d\n", dwEntries);
			//wprintf(L"The following RAS entry names were found:\n");

			for (DWORD i = 0; i < dwEntries; i++) 
			{
				//printf("%s\n", lpRasEntryName[i].szEntryName);

				rc = RasValidateEntryName(NULL, lpRasEntryName[i].szEntryName);

				if (rc == ERROR_INVALID_NAME)
				{
					printf("--- RasValidateEntryName returned invalid name in CreateTestEntry: %d\n",rc);
				}
				else if (rc == ERROR_SUCCESS)
				{
					if ((dwErr = RasGetEntryProperties(lpszPhonebook, lpRasEntryName[i].szEntryName, NULL, &dwRasEntryInfoSize, NULL, 0)) != ERROR_BUFFER_TOO_SMALL)
					{
						g_lpRasEntry = (LPRASENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRasEntryInfoSize);
						if (NULL == g_lpRasEntry) 
						{
							return 0;
						}
						g_lpRasEntry->dwSize = sizeof(RASENTRY);
					}

					if (dwRasEntryInfoSize > 0)
					{
						g_lpRasEntry = (LPRASENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRasEntryInfoSize);
						if (NULL == g_lpRasEntry)
						{
							return 0;
						}
						g_lpRasEntry->dwSize = sizeof(RASENTRY);
						//g_lpRasEntry->dwFramingProtocol = RASFP_Ppp;
						//g_lpRasEntry->dwfOptions = 0;
						//g_lpRasEntry->dwType = RASET_Broadband;
						//g_lpRasEntry->dwfNetProtocols = RASNP_Ip;
					}
					else
					{
						g_lpRasEntry = NULL;
					}
					if ((dwErr1 = RasGetEntryProperties(lpszPhonebook, lpRasEntryName[i].szEntryName, g_lpRasEntry, &dwRasEntryInfoSize, NULL, 0)) == ERROR_SUCCESS)
					{
						if (RasSetEntryProperties(lpszPhonebook, lpszEntry, g_lpRasEntry, dwRasEntryInfoSize, NULL, 0) == ERROR_SUCCESS)
							if ((RasRenameEntry(lpszPhonebook, lpszEntry, lpszEntry1)) == ERROR_SUCCESS)
								if ((RasDeleteEntry(lpszPhonebook, lpszEntry1)) == ERROR_SUCCESS)
									printf("we 111 are done!!!!\n");
					}
					HeapFree(GetProcessHeap(), 0, g_lpRasEntry);
					g_lpRasEntry = NULL;
				}
			}
		}
		//Deallocate memory for the connection buffer
		HeapFree(GetProcessHeap(), 0, lpRasEntryName);
		lpRasEntryName = NULL;
	}
	return 0;
}