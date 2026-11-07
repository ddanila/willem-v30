/* Synthetic IO.DLL for the original Willem Windows application under Wine.
 * No hardware access: writes update memory; reads provide BUSY loopback and
 * constant ACK=0. Only host-port order/values are observed, not electrical
 * timing or a complete virtual programmer. See docs/original-at29c512.md.
 * Build: i686-w64-mingw32-gcc -shared -O2 -Wl,--kill-at this.c -o io.dll
 */
#include <windows.h>
#include <stdio.h>
static unsigned char regs[65536];
static FILE *logfile;
static void logio(char op, unsigned p,unsigned v) {
 if(!logfile) {logfile=fopen("lpt-trace.log","w"); if(logfile)setvbuf(logfile,0,_IOFBF,65536);}
 if(logfile)fprintf(logfile,"%c %04X %02X\n",op,p,v);
}
__declspec(dllexport) void __stdcall PortOut(WORD p,BYTE v){regs[p]=v;logio('W',p,v);}
__declspec(dllexport) BYTE __stdcall PortIn(WORD p){BYTE v=(p==0x379)?((regs[0x378]&1)?0:0x80):regs[p];logio('R',p,v);return v;}
__declspec(dllexport) BOOL __stdcall IsDriverInstalled(void){return 1;}
__declspec(dllexport) void __stdcall ReleasePort(WORD p){(void)p;}
__declspec(dllexport) WORD __stdcall PortWordIn(WORD p){return PortIn(p);}
__declspec(dllexport) DWORD __stdcall PortDWordIn(WORD p){return PortIn(p);}
__declspec(dllexport) void __stdcall PortWordOut(WORD p,WORD v){PortOut(p,v);}
__declspec(dllexport) void __stdcall PortDWordOut(WORD p,DWORD v){PortOut(p,v);}
__declspec(dllexport) void __stdcall SetPortBit(WORD p,BYTE b){PortOut(p,PortIn(p)|(1<<b));}
__declspec(dllexport) void __stdcall ClrPortBit(WORD p,BYTE b){PortOut(p,PortIn(p)&~(1<<b));}
__declspec(dllexport) void __stdcall NotPortBit(WORD p,BYTE b){PortOut(p,PortIn(p)^(1<<b));}
__declspec(dllexport) BOOL __stdcall GetPortBit(WORD p,BYTE b){return !!(PortIn(p)&(1<<b));}
__declspec(dllexport) void __stdcall LeftPortShift(WORD p,WORD c){PortOut(p,PortIn(p)<<c);}
__declspec(dllexport) void __stdcall RightPortShift(WORD p,WORD c){PortOut(p,PortIn(p)>>c);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD r,LPVOID p){if(r==DLL_PROCESS_DETACH&&logfile)fclose(logfile);return TRUE;}
