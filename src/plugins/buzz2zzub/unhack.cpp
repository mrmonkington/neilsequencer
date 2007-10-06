#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include "unhack.h"
#include "MemoryModule.h"

using namespace std;

bool hexStringToBytes(std::string str, unsigned char* pc) {
    for (size_t i=0; i<str.length()/2; i++) {
        string b=str.substr(i*2, 2);
        if (!isxdigit(str[0])) return false;
        if (!isxdigit(str[1])) return false;
        int c;
        sscanf(b.c_str(), "%x", &c);
        pc[i]=(unsigned char)c;
    }
    return true;
}

/***

    unhack

***/
unsigned int unhack::hackmidi[256];
hackseqtype* unhack::hackseq=0;
unsigned int unhack::hackbpm=125;
std::map<std::string, std::vector<std::string> > unhack::patches;
std::map<HMODULE, std::string > unhack::modules;
std::map<HINSTANCE, HMODULE > unhack::instances;

void unhack::enablePatch(std::string machineName, std::string expr) {
    if (unhack::hackseq==0) {
        unhack::hackseq = new hackseqtype();
        memset(unhack::hackmidi, 0, sizeof(unhack::hackmidi));
    }

    vector<string>& p = patches[machineName];
    p.push_back(expr);
}

bool unhack::isPatch(std::string machineName) {
    return patches.find(machineName)!=patches.end();
}

void unhack::process(std::string machineName, void* data, size_t len) {
    vector<string>& p = patches[machineName];
    if (p.size()==0) return ;

    for (size_t i = 0; i<p.size(); i++) {
        string token=p[i];
        if (token=="bpm") {
            bpm(data, len, machineName);
        } else
        if (token=="seq") {
            seq(data, len, machineName);
        } else
        if (token=="midi") {
            midi(data, len, machineName);
        } else
        if (token.find_first_of("replace(")==0) {
            string param=token.substr(8);
            param=param.substr(0, param.length()-1);

            int lc=param.find_first_of(',');
            if (lc==param.npos) continue;

            string param1=param.substr(0, lc);
            string param2=param.substr(lc+1);
            if (param1.size()!=param2.size()) continue; // different patch length
            if ((param1.size()&1)!=0) continue;        // odd patch length

            // convert params to bytes
            unsigned char* src=new unsigned char[param1.size()/2];
            unsigned char* dst=new unsigned char[param2.size()/2];
            bool ret=hexStringToBytes(param1, src)&&hexStringToBytes(param2, dst);
            if (ret)            
                replace(data, len, src, dst, param1.size()/2, machineName, "Custom replace");

            delete[] src;
            delete[] dst;
        }
    }

}

void unhack::replace(void* buffer, size_t bufferlen, unsigned char* find, unsigned char* repl, size_t findlen, std::string machineName, std::string fixName) {
    unsigned char* srch = (unsigned char*)buffer;

    int count = 0;
    for (size_t i = 0; i<bufferlen-findlen; i++) {
        size_t j = 0;
        for (; j<findlen; j++) {
            if (srch[j]!=find[j]) break;
        }
        if (j==findlen) {
            memcpy(srch, repl, findlen);
            printf(("Applied patch '" + fixName + "' on " + machineName + "\n").c_str());
        }

        srch++;
    }
}

void unhack::bpm(void* buffer, size_t bufferlen, std::string machineName) {
    // bpm hack 0x004F0180
    unsigned char find[4] = { 0x80, 0x01, 0x4F, 0x00 };
    unsigned char repl[4];
    unsigned int bpm=(unsigned int)&unhack::hackbpm;
    memcpy(repl, &bpm, 4);
    replace(buffer, bufferlen, find, repl, 4, machineName, "BPM patch");
}

void unhack::seq(void* buffer, size_t bufferlen, std::string machineName) {
    // sequencer hack 0x004F01B0
    unsigned char find[4] = { 0xB0, 0x01, 0x4F, 0x00 };
    unsigned char repl[4];
    unsigned int seq=(unsigned int)&unhack::hackseq;
    memcpy(repl, &seq, 4);
    replace(buffer, bufferlen, find, repl, 4, machineName, "Sequencer patch");
}

void unhack::midi(void* buffer, size_t bufferlen, std::string machineName) {
    // midi hack 0x004DA280
    unsigned char find[4] = { 0x80, 0xA2, 0x4D, 0x00 };
    unsigned char repl[4];
    unsigned int seq=(unsigned int)&unhack::hackmidi;
    memcpy(repl, &seq, 4);
    replace(buffer, bufferlen, find, repl, 4, machineName, "MIDI patch");
}


std::string unhack::machineNameFromFileName(std::string fileName) {
    size_t ls=fileName.find_last_of("\\/");
    if (ls!=string::npos)
        fileName=fileName.substr(ls+1);

    ls=fileName.find_last_of('.');
    if (ls!=string::npos)
        fileName=fileName.substr(0, ls);

    return fileName;
}

bool isMfc(std::string mfc) {
	std::transform(mfc.begin(), mfc.end(), mfc.begin(), std::tolower);
	return mfc == "mfc42";
}

HMODULE unhack::loadLibrary(LPCTSTR lpLibFileName) {
    std::string name=machineNameFromFileName(lpLibFileName);
	bool patch = isPatch(name);
	bool mfc = isMfc(name);

    if (patch || mfc) {

		char lpFilePath[MAX_PATH];
		if (mfc) {
			char* lpFilePart;
			if (0 == SearchPath(0, lpLibFileName, 0, MAX_PATH, lpFilePath, &lpFilePart))
				return 0;
		} else
			strncpy(lpFilePath, lpLibFileName, MAX_PATH);

        FILE* fp = fopen(lpFilePath, "rb");
	    if (fp == 0) return 0;

        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        unsigned char* data = new unsigned char[size];
        if (!data) return 0;

        fseek(fp, 0, SEEK_SET);
        fread(data, 1, size, fp);
        fclose(fp);

		if (patch) {
			// apply patch here
			process(name, data, size);
		}
        
        HMODULE module = (HMODULE)MemoryLoadLibrary(data, lpLibFileName);
        if (!module) return 0;

        delete[] data;
        
		// moved to WrapModuleCreate:
		// modules.insert(std::pair<HMODULE, std::string>(module, name));

        return module;
    } else
        return LoadLibrary(lpLibFileName);
}

BOOL unhack::freeLibrary(HMODULE hModule) {
    map<HMODULE, string>::iterator i=modules.find(hModule);
    if (i!=modules.end()) {
        modules.erase(hModule);
        MemoryFreeLibrary(hModule);
        return TRUE;
    } else
        return FreeLibrary(hModule);
}

FARPROC unhack::getProcAddress(HMODULE hModule, LPCSTR str) {
    map<HMODULE, string>::iterator i=modules.find(hModule);
    if (i != modules.end()) {
        return MemoryGetProcAddress(hModule, str);
    } else  
        return GetProcAddress(hModule, str);
}

void unhack::hackTick(int bpm, int loopBegin, int loopEnd, int songEnd, int songPos) {
	hackbpm = bpm;
	hackseq->loopBegin = loopBegin;
	hackseq->loopEnd = loopEnd;
	hackseq->songEnd = songEnd;
	hackseq->songPos = songPos;
}

extern "C" void WrapModuleCreate(HINSTANCE hCode, HMODULE hModule, LPCSTR name) {
	// on win32, HINSTANCE == HMODULE, but a memorymodule separates between the two, so we need to keep both
	unhack::modules.insert(std::pair<HMODULE, std::string>(hModule, name));
	unhack::instances.insert(std::pair<HINSTANCE, HMODULE>(hCode, hModule));
}

extern "C" void WrapModuleRelease(HINSTANCE code) {
	// TODO
}

extern "C" DWORD WINAPI WrapGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    // make sure to return a sane name for manually loaded dlls
	map<HMODULE, string>::iterator i = unhack::modules.find(hModule);
	
	// on win32, HINSTANCE == HMODULE, but a memorymodule separates between the two, so we look up the instance here
	if (i == unhack::modules.end()) {
		map<HINSTANCE, HMODULE>::iterator j = unhack::instances.find(hModule);
		if (j != unhack::instances.end())
			i = unhack::modules.find(j->second);
	}

    if (i != unhack::modules.end()) {
		// return a valid path
		_fullpath(lpFilename, i->second.c_str(), nSize);
		return strlen(lpFilename);
	} else
		return GetModuleFileNameA(hModule, lpFilename, nSize);
}

extern "C" HMODULE WINAPI WrapLoadLibraryA(LPCSTR lpFileName) {
	// disallow loading wo_pasio.dll to avoid PVST tripping bad
	if (0 != strstr(lpFileName, "wo_pasio.dll")) return 0;
	return unhack::loadLibrary(lpFileName);
}

extern "C" FARPROC WINAPI WrapGetProcAddress(HMODULE hModule, LPCSTR lpFileName) {
	return unhack::getProcAddress(hModule, lpFileName);
}
