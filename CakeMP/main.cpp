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

static void appInitialize(HMODULE hInstance)
{
	
	
	logOpen(PROJECT_NAME_SHORT ".log");

	logWrite("Initializing v" PROJECT_VERSION);


	CefModuleManager = new CefModule(hInstance);
	_pGame = new Cake(hInstance);

	keyboardHandlerRegister(appKeyboardHandler);
	
	

	//presentCallbackRegister(PresentCallBack);

	
}

static void appUninitialize()
{
	logWrite("Uninitializing");

	keyboardHandlerUnregister(appKeyboardHandler);
	//presentCallbackUnregister(PresentCallBack);
	delete _pGame;

	memTest();
	logClose();
}


NAMESPACE_END;

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH) {

		

		NAMESPACE_NAME::appInitialize(hInstance);

		//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CefModule::ServiceWorker, (LPVOID)hInstance, NULL, NULL);
	} else if (reason == DLL_PROCESS_DETACH) {
		NAMESPACE_NAME::appUninitialize();
	}
	return TRUE;
}
