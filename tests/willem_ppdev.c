/* Test-only DOSBox ppdev Willem model. Regular files only, no hardware. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/ppdev.h>
#include <linux/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
static struct stat target;
static int ready, sample_oracle;
static unsigned address_bits=24;
static uint8_t *rom, data, control, read_byte, read_mask;
static uint32_t shift;
static unsigned clocks, address, loaded, alias, pulses, errors;
static unsigned pulse_addr, pulse_data, active;
static FILE *logfile, *rawfile;
static void raw(char op, unsigned port, unsigned value) {
 if(rawfile) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
  fprintf(rawfile,"%c %04X %02X %lld.%09ld\n",op,port,value,(long long)t.tv_sec,t.tv_nsec); }
}
__attribute__((constructor)) static void setup(void) {
 const char *ep=getenv("WL_ENDPOINT"), *path=getenv("WL_IMAGE"), *log=getenv("WL_LOG");
 struct stat sb;
 if (!ep || !path || !log || stat(ep,&target) || !S_ISREG(target.st_mode)) return;
 int fd=open(path,O_RDWR|O_NOFOLLOW);
 if(fd<0 || fstat(fd,&sb) || !S_ISREG(sb.st_mode) || sb.st_size!=2048) _exit(2);
 rom=mmap(NULL,2048,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0); close(fd);
 if(rom==MAP_FAILED) _exit(2);
 logfile=fopen(log,"w"); if(!logfile) _exit(2);
 setvbuf(logfile,NULL,_IOLBF,0);
 alias=getenv("WL_ALIAS")?strtoul(getenv("WL_ALIAS"),NULL,0):0;
 if(getenv("WL_RAW_LOG")) { rawfile=fopen(getenv("WL_RAW_LOG"),"w"); if(rawfile)setvbuf(rawfile,NULL,_IOLBF,0); }
 sample_oracle=getenv("WL_SAMPLE_ORACLE")!=NULL;
 if(getenv("WL_ADDRESS_BITS")) address_bits=strtoul(getenv("WL_ADDRESS_BITS"),NULL,0);
 ready=1;
}
static unsigned selected(void) { return address & 2047U & ~alias; }
static void out_data(uint8_t v) {
 uint8_t old=data; data=v;
 if(active && v!=old) { ++errors; fprintf(logfile,"ERROR data changed during pulse\n"); }
 if(control&2) {
  if(!(old&1)&&(v&1)) { shift=((shift<<1)|((v&2)?1:0))&0xffffff; address=shift; ++clocks; }
  if(!(old&2)&&(v&2)&&(v&4)) {
   read_byte=((control&4)&&!(control&8))?rom[selected()]:255;
   read_mask=128; loaded=1;
  }
  if(!sample_oracle && !(old&4)&&(v&4)&&loaded) { if(loaded==1) loaded=2; else read_mask>>=1; }
 }
}
static void out_control(uint8_t raw) {
 uint8_t old=control; control=raw^11;
 if((old&2)&&!(control&2)&&clocks) {
  if(address_bits && (control&4) && clocks!=address_bits) { ++errors; fprintf(logfile,"ERROR %u clocks\n",clocks); }
  address=shift&0xffffff; clocks=0;
 }
 if((control&5)==1) { ++errors; fprintf(logfile,"ERROR VPP without VCC\n"); }
 if(!(old&8)&&(control&8)&&(control&5)==5) {
  if(control&2) { ++errors; fprintf(logfile,"ERROR OE enabled at pulse\n"); }
  pulse_addr=selected(); pulse_data=data; active=1;
 }
 if((old&8)&&!(control&8)&&active) {
  rom[pulse_addr]&=pulse_data; active=0; ++pulses;
  fprintf(logfile,"PULSE %04x %02x\n",pulse_addr,pulse_data);
 }
}
int ioctl(int fd,unsigned long request,...) {
 __builtin_va_list ap; __builtin_va_start(ap,request);
 void *arg=__builtin_va_arg(ap,void*); __builtin_va_end(ap);
 struct stat sb;
 if(!ready || _IOC_TYPE(request)!=PP_IOCTL || fstat(fd,&sb) ||
    sb.st_dev!=target.st_dev || sb.st_ino!=target.st_ino)
  return syscall(SYS_ioctl,fd,request,arg);
 uint8_t *v=arg;
 switch(request) {
 case PPCLAIM: case PPRELEASE: case PPSETMODE: case PPDATADIR: return 0;
 case PPWDATA: raw('W',0x378,*v); out_data(*v); return 0;
 case PPWCONTROL: raw('W',0x37a,*v); out_control(*v); return 0;
 case PPRDATA: *v=data; raw('R',0x378,*v); return 0;
 case PPRCONTROL: *v=control^11; raw('R',0x37a,*v); return 0;
 case PPRSTATUS: *v=((data&1)?0:0x80)|((read_byte&read_mask)?0:0x40); raw('R',0x379,*v); if(sample_oracle)read_mask>>=1; return 0;
 default: errno=ENOTTY; return -1;
 }
}
__attribute__((destructor)) static void finish(void) {
 if(ready) fprintf(logfile,"FINAL pulses=%u errors=%u rails=%u\n",pulses,errors,control&5);
}
