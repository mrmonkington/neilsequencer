
int ovd_library_loaded;
void (__cdecl *ovd_regimg_m)(char *, HINSTANCE, char *);
void overloaderdata_registerimage(char * imagename, HINSTANCE tinst, char * resname)
{
	if (ovd_library_loaded == 1) {
		ovd_regimg_m(imagename, tinst, resname);
	}
}
void overloaderdata_initialize()
{
	ovd_library_loaded = 0;
	HMODULE h_dll_ovd;

	h_dll_ovd = LoadLibrary("Overloader Data.dll");
	if (h_dll_ovd == NULL) {
		h_dll_ovd = LoadLibrary("../Overloader Data.dll");
	}
	if (h_dll_ovd == NULL) {
		h_dll_ovd = LoadLibrary("../../Overloader Data.dll");
	}
	if (h_dll_ovd != NULL) {
		ovd_library_loaded = 1;
		ovd_regimg_m=(void (__cdecl *)(char *, HINSTANCE, char *))GetProcAddress(h_dll_ovd,"OvD_RegisterImage");
	}
}
