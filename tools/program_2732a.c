/* Intel D2732A only: 5V VCC, 21V VPP, 50ms active-low CE.
 * Fixed LPT378 PCB3B, special 2732 routing. One pulse/cell, no retries.
 * Reuses the qualified read/address core and PIT2 timing. No DOS calls
 * with VPP enabled. First image byte followed by complete alias scan.
 */
#include <stdio.h>
#include <string.h>
#include "willem.h"
extern void dos_outb(unsigned,unsigned);
extern unsigned dos_inb(unsigned);
extern void dos_wait_us(unsigned);
extern void dos_2732_pulse(unsigned);
static unsigned char image[4096];
static void data_out(void *ctx,int v) { dos_outb(0x378U,(unsigned)v);dos_wait_us(20U); }
static void control_out(void *ctx,int v) { dos_outb(0x37aU,(unsigned)v);dos_wait_us(20U); }
static wl_u8 status_in(void *ctx) { return (wl_u8)dos_inb(0x379U); }
static void delay(void *ctx,int us) { dos_wait_us((unsigned)us); }
static void program(struct willem *w,unsigned a,unsigned v)
{
 /* Inhibit CE before any VPP transition. Shift only while VPP off. */
 wl_we(w,1);
 wl_set_address(w,(wl_u32)a,WL_ADDR_FIRST_BIT);
 wl_set_data(w,(int)v);
 dos_wait_us(1000U);
 wl_vpp(w,1);
 dos_wait_us(10000U);
 dos_2732_pulse(0x378U); /* raw 06 idle -> 0E active -> 06 idle */
 dos_wait_us(1000U);
 wl_vpp(w,0);
 dos_wait_us(10000U);
 wl_we(w,0);
}
int main(int argc,char **argv)
{
 struct wl_io io;
 struct willem w;
 FILE *f;
 unsigned n,i,first=4096U,actual=0,wanted=0,pulses=0;
 int extra,result=1;
 char gate[64];
 if(argc!=3 || strcmp(argv[2],"/WRITE21"))return 1;
 f=fopen("2732A.OK","rb");if(!f)return 1;
 if(!fgets(gate,sizeof(gate),f)){fclose(f);return 1;}fclose(f);
 if(strcmp(gate,"D2732A-5V-21V-50MS\n"))return 1;
 f=fopen(argv[1],"rb");if(!f)return 1;
 n=fread(image,1,4096,f);extra=fgetc(f);fclose(f);
 if(n!=4096U || extra!=EOF)return 1;
 for(i=0;i<4096U;i++)if(image[i]!=255U){first=i;break;}
 if(first==4096U)return 1;
 memset(&io,0,sizeof(io));io.data_write=data_out;io.control_write=control_out;
 io.status_read=status_in;io.delay_us=delay;
 /* Inhibit rails before wl_init changes the data lines. */
 dos_outb(0x37aU,0x0bU);
 wl_init(&w,&io);wl_set_read_timing(&w,20U,20U,20U,20U,10U);
 wl_begin_2732_read(&w);
 for(i=0;i<4096U;i++) {
  actual=wl_read_byte(&w,i);wanted=255U;
  if(actual!=wanted){result=2;goto done;}
 }
 program(&w,first,(unsigned)image[first]);pulses++;
 for(i=0;i<4096U;i++) {
  actual=wl_read_byte(&w,i);wanted=i==first?image[first]:255U;
  if(actual!=wanted){result=3;goto done;}
 }
 for(i=first+1U;i<4096U;i++) {
  if(image[i]==255U)continue;
  program(&w,i,(unsigned)image[i]);pulses++;
  actual=wl_read_byte(&w,i);wanted=image[i];
  if(actual!=wanted){result=4;goto done;}
 }
 /* Power-cycle before full verification, including untouched FF cells. */
 wl_end_read(&w);dos_wait_us(50000U);dos_wait_us(50000U);
 wl_begin_2732_read(&w);
 for(i=0;i<4096U;i++) {
  actual=wl_read_byte(&w,i);wanted=image[i];
  if(actual!=wanted){result=5;goto done;}
 }
 result=0;
done:
 wl_end_read(&w);
 puts("SAFE OFF: VPP off, VCC off");
 printf("D2732A result=%d pulses=%u address=%04X actual=%02X wanted=%02X\n",result,pulses,i,actual,wanted);
 if(!result)puts("WRITE PASSED: all 4096 bytes verified after power cycle");
 return result;
}
