#include <Common.h>

#include <System/Cake.h>

#include <Utils/AppMem.h>

#include <Build.h>

#include <Windows.h>

#include <shv/main.h>
#include <shv/natives.h>
#include <CefApp\CakeWebView.h>

#include "CefApp/CefModule.h"

#include <Psapi.h>

NAMESPACE_BEGIN
	;

static void appKeyboardHandler(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow)
{
	if (!wasDownBefore && !isUpNow) {
		_pGame->OnKeyDown(key);
	} else if (wasDownBefore && isUpNow) {
		_pGame->OnKeyUp(key);
	}
}

struct NativeReg
{
	NativeReg* next;
	void* handlers[7];
	uint32_t numEntries;
	uint64_t hashes[7];
};
void PresentCallBack(void* chain)
{
	if (_pGame) {
		_pGame->m_render.presentCallback(chain);
	}

}
struct PatternByte
{
	PatternByte() : ignore(true) {
		//
	}

	PatternByte(std::string byteString, bool ignoreThis = false) {
		data = StringToUint8(byteString);
		ignore = ignoreThis;
	}

	bool ignore;
	UINT8 data;

private:
	UINT8 StringToUint8(std::string str) {
		std::istringstream iss(str);

		UINT32 ret;

		if (iss >> std::hex >> ret) {
			return (UINT8)ret;
		}

		return 0;
	}
};

struct Pattern
{
	static DWORD64 Scan(DWORD64 dwStart, DWORD64 dwLength, std::string s) {
		std::vector<PatternByte> p;
		std::istringstream iss(s);
		std::string w;

		while (iss >> w) {
			if (w.data()[0] == '?') { // Wildcard
				p.push_back(PatternByte());
			}
			else if (w.length() == 2 && isxdigit(w.data()[0]) && isxdigit(w.data()[1])) { // Hex
				p.push_back(PatternByte(w));
			}
			else {
				return NULL; // You dun fucked up
			}
		}

		for (DWORD64 i = 0; i < dwLength; i++) {
			UINT8* lpCurrentByte = (UINT8*)(dwStart + i);

			bool found = true;

			for (size_t ps = 0; ps < p.size(); ps++) {
				if (p[ps].ignore == false && lpCurrentByte[ps] != p[ps].data) {
					found = false;
					break;
				}
			}

			if (found) {
				return (DWORD64)lpCurrentByte;
			}
		}

		return NULL;
	}

	static DWORD64 Scan(MODULEINFO mi, std::string s) {
		return Scan((DWORD64)mi.lpBaseOfDll, (DWORD64)mi.SizeOfImage, s);
	}
};


bool memory_compare(const BYTE* data, const BYTE* pattern, const char* mask)
{
	for (; *mask; ++mask, ++data, ++pattern)
	{
		if (*mask == 'x' && *data != *pattern)
		{
			return false;
		}
	}
	return (*mask) == NULL;
}
MODULEINFO g_MainModuleInfo = { 0 };
UINT64 FindPattern(char* pattern, char* mask)
{	//Edited, From YSF by Kurta999
	UINT64 i;
	UINT64 size;
	UINT64 address;

	MODULEINFO info = { 0 };

	address = (UINT64)GetModuleHandle(NULL);
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &info, sizeof(MODULEINFO));
	size = (UINT64)info.SizeOfImage;

	for (i = 0; i < size; ++i)
	{
		if (memory_compare((BYTE*)(address + i), (BYTE*)pattern, mask))
		{
			return (UINT64)(address + i);
		}
	}
	return 0;
}
void NoIntro()
{
	// CREDITS: CitizenMP

	//Disable logos since they add loading time
	UINT64 logos = FindPattern("platform:/movies/rockstar_logos", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	if (logos != 0)
	{
		//memset((void*)(logos + 0x11), 0x00, 0x0E);
		memcpy((void*)logos, "./nonexistingfilenonexistingfil", 32);

		//DisableLegalMessagesCompletely();
		DWORD64 dwSplashScreen = Pattern::Scan(g_MainModuleInfo, "72 1F E8 ? ? ? ? 8B 0D");
		if (dwSplashScreen == NULL)  //If the module is still encrypted at the time of injection, run the No Intro code.
		{
			while (dwSplashScreen == NULL)
			{
				Sleep(10);
				dwSplashScreen = Pattern::Scan(g_MainModuleInfo, "72 1F E8 ? ? ? ? 8B 0D");
			}

			if (dwSplashScreen != NULL)
				*(unsigned short*)(dwSplashScreen) = 0x9090; //NOP out the check to make it think it's time to stop.

			DWORD64 dwRockStarLogo = Pattern::Scan(g_MainModuleInfo, "70 6C 61 74 66 6F 72 6D 3A");
			int iCounter = 0;
			while (dwRockStarLogo == NULL)
			{
				Sleep(10);
				dwRockStarLogo = Pattern::Scan(g_MainModuleInfo, "70 6C 61 74 66 6F 72 6D 3A");
			}

			if (dwRockStarLogo != NULL)
				*(unsigned char*)(dwRockStarLogo) = 0x71; //Replace the P with some garbage so it won't find the file.

			Sleep(15000); //Wait until the logo code has finished running.
						  //Restore the EXE to its original state.
			*(unsigned char*)(dwRockStarLogo) = 0x70;
			*(unsigned short*)(dwSplashScreen) = 0x1F72;
		}
	}
}
static void appInitialize(HMODULE hInstance)
{
	
	
	logOpen(PROJECT_NAME_SHORT ".log");

	logWrite("Initializing v" PROJECT_VERSION);

	if (!GetModuleInformation(GetCurrentProcess(), GetModuleHandle(0), &g_MainModuleInfo, sizeof(g_MainModuleInfo))) {
		logWrite("Unable to get MODULEINFO from GTA5.exe");
	}
	NoIntro();

	uint8_t* pMapNatives = memFindPattern("48 89 5C 24 ?? 48 89 7C 24 ?? 45 33 C0 4C");
	if (pMapNatives != nullptr) {
		logWrite("Dumping natives...");

		uint32_t offset = *(uint32_t*)(pMapNatives + 0x1D + 3);
		NativeReg** table = (NativeReg**)(pMapNatives + 0x24 + offset);

		int numNatives = 0;

		FILE* fh = fopen("natives.txt", "wb");
		if (fh != nullptr) {
			int tableIndex = 0;
			NativeReg* cur = cur = table[tableIndex];
			do {
				while (cur != nullptr) {
					for (uint32_t i = 0; i < cur->numEntries; i++) {
						fprintf(fh, "0x%016llX @ %p\n", cur->hashes[i], cur->handlers[i]);
						numNatives++;
					}
					cur = cur->next;
				}
				cur = table[++tableIndex];
			} while (cur != nullptr);
			fclose(fh);
		}

		logWrite("Done!");
	}

	CefModuleManager = new CefModule(hInstance);
	_pGame = new Cake(hInstance);

	keyboardHandlerRegister(appKeyboardHandler);
	
	

	presentCallbackRegister(PresentCallBack);

	
}

static void appUninitialize()
{
	logWrite("Uninitializing");

	keyboardHandlerUnregister(appKeyboardHandler);
	presentCallbackUnregister(PresentCallBack);
	delete _pGame;

	memTest();
	logClose();
}


NAMESPACE_END;

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH) {

		

		NAMESPACE_NAME::appInitialize(hInstance);

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CefModule::ServiceWorker, (LPVOID)hInstance, NULL, NULL);
	} else if (reason == DLL_PROCESS_DETACH) {
		NAMESPACE_NAME::appUninitialize();
	}
	return TRUE;
}
