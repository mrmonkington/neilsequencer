#include <windows.h>
#include <memory.h>
#include "resource.h"

HINSTANCE dllInstance;


BOOL WINAPI DllMain ( HANDLE hModule, DWORD fwdreason, LPVOID lpReserved )
{
   switch (fwdreason) {
   case DLL_PROCESS_ATTACH:
      dllInstance = (HINSTANCE) hModule;
      break;

   case DLL_THREAD_ATTACH: break;
   case DLL_THREAD_DETACH: break;
   case DLL_PROCESS_DETACH: break;
   }
   return TRUE;
}

/*

BOOL APIENTRY AboutDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch(uMsg) {
   case WM_INITDIALOG: 
      return 1;

   case WM_SHOWWINDOW: 
      return 1;

   case WM_CLOSE:
      EndDialog (hDlg, TRUE);
      return 0;

   case WM_COMMAND:
      switch (LOWORD (wParam))
      {
      case IDOK:
         EndDialog(hDlg, TRUE);
         return 1;
      default:
         return 0;
      }
      break;
   }
   return 0;
}

*/