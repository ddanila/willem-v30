/* Bounded hardware experiments based on captured WILEP996 2716 transactions.
 * All numeric arguments hexadecimal. R file bits highVPP; W addr byte bits.
 * Reads finish before creating a file. Writes: one FF cell, one PIT2 pulse.
 * High-VPP read is program verification with CE kept low, not a pulse.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void dos_outb(unsigned,unsigned);
extern unsigned dos_inb(unsigned);
extern void dos_wait_us(unsigned);
extern void dos_2716_pulse(unsigned);
static unsigned char data[2048];
static unsigned bad;
static unsigned hex(s)
char *s;
{
 unsigned n=0,d;
 if(!*s) bad=1;
 while(*s) {
  if(*s>='0'&&*s<='9')d=*s-'0';
  else if(*s>='a'&&*s<='f')d=*s-'a'+10;
  else if(*s>='A'&&*s<='F')d=*s-'A'+10;
  else {bad=1;return 0;}
  if(n>4095U){bad=1;return 0;}
  n=n*16U+d;s++;
 }
 return n;
}
static void control(v) unsigned v; { dos_outb(0x37aU,v);dos_wait_us(100U); }
static void output(v) unsigned v; { dos_outb(0x378U,v);dos_wait_us(20U); }
static void address(a,bits) unsigned a,bits;
{
 unsigned long mask;
 unsigned v;
 for(mask=1UL<<(bits-1U);mask;mask>>=1) {
  v=((unsigned long)a&mask)?6U:4U;
  output(v);output(v|1U);output(v);
 }
 output(4U);
}
static unsigned input(void)
{
 unsigned n=0,i;
 output(6U);output(2U);output(0U);output(4U);
 for(i=0;i<8U;i++) {
  n=(n<<1)|((dos_inb(0x379U)&0x40U)?0U:1U);
  output(0U);output(4U);
 }
 return n;
}
static void off(void)
{
 /* VPP off first, CE low, then VCC off. */
 control(0x0fU);control(0x0bU);output(0U);
}
int main(argc,argv) int argc; char **argv;
{
 unsigned bits,high,a=0,v=0,i,actual;
 int write_mode,result=1;
 FILE *f;
 if(argc!=5 || (strcmp(argv[1],"R")&&strcmp(argv[1],"W")))return 1;
 write_mode=!strcmp(argv[1],"W");
 if(write_mode) {a=hex(argv[2]);v=hex(argv[3]);bits=hex(argv[4]);high=0;}
 else {bits=hex(argv[3]);high=hex(argv[4]);}
 if(bad || (bits!=11U&&bits!=24U) || high>1U || a>=2048U || v>=255U)return 1;
 if(write_mode || high) {
  char gate[48];
  f=fopen("RF2.OK","rb");if(!f)return 1;
  if(!fgets(gate,48,f)){fclose(f);return 1;}fclose(f);
  if(strncmp(gate,"WILLEM-2716-5V-25V-GATE-1",23))return 1;
 }
 off();control(9U);address(0U,24U); /* initialize upper address stages */
 control(0x0dU);dos_wait_us(10000U); /* VCC on, CE low, OE low */
 if(write_mode) {
  address(a,bits);actual=input();
  if(actual!=255U) {printf("REFUSED %04X=%02X not FF\n",a,actual);goto done;}
  /* Original trace: shift -> 0C -> 0E -> data -> 06 -> 0E -> 00 -> 0F -> 0D. */
  address(a,bits);control(0x0cU);dos_wait_us(10000U);control(0x0eU);
  output(v);dos_wait_us(1000U);dos_2716_pulse(0x378U);
  output(0U);control(0x0fU);control(0x0dU);
  actual=input();printf("ORIGINAL sequence %04X <- %02X read=%02X; one PIT2 50ms pulse\n",a,v,actual);
  result=(actual==v)?0:2;
 } else {
  if(high) { control(0x0cU);dos_wait_us(10000U); }
  for(i=0;i<2048U;i++){address(i,bits);data[i]=(unsigned char)input();}
  result=0;
 }
done:
 off();puts("SAFE OFF: VPP off, VCC off");
 if(!write_mode && result==0) {
  f=fopen(argv[2],"wb");if(!f)return 1;
  if(fwrite(data,1,2048,f)!=2048U)result=1;
  fclose(f);
 }
 return result;
}
