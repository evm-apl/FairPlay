#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>

#define byte unsigned char

//We are going to TX these in big endian bit order
//So the bit fields are flipped from how they appear
//on the line
typedef struct {
  byte inning_tens:3;
  byte error:1;
  byte unused:3;
  byte bright:1;
  
  byte ball:3;
  byte strike:2;
  byte out:2;
  byte hit:1;

  byte inning_ones;
  
  byte home_ones;
  byte home_tens;

  byte guest_ones;
  byte guest_tens;
} bb_message_t;

//  FairPlay digit pattern
//  (bit position in byte)
//
//  007
//  5 1
//  567
//  4 2
//  337
//

byte digi_pat(byte n)
{
  byte pat[] = {0xBF,0x86,0xDB,0x4F,0xE6,0xED,0xFD,0x87,0xFF,0xEF};

  return pat[n];
}

void print_struct(bb_message_t * b) {
  printf("%d %d %d %d %d %d %d %d %d %d %d %d\n",
    b->home_tens,
    b->home_ones,
    b->guest_tens,
    b->guest_ones,
    b->inning_tens,
    b->inning_ones,
    b->ball,
    b->strike,
    b->out,
    b->hit,
    b->error,
    b->bright);
}

void printMsg(bb_message_t * bs)
{
  int i=0;
  int byte_off, bit_off;
  char b;
  unsigned char * m = (unsigned char *)bs;
  int size = sizeof(bb_message_t)*8;

  for (i=0; i<size; i++) {
    byte_off=i/8;
    bit_off=i%8;
    b=m[byte_off]>>(7-bit_off);
    printf("%d",b&1);
  }
  printf("\n");
}


unsigned int tpow(unsigned int n) {
  unsigned int i=0,r=1;
  for(i;i<n;i++) {
   r*=2;
  }
  return r;
}

void fill_struct(bb_message_t * b) {
  unsigned int m;
  FILE * f = NULL;
  while (f == NULL) {
    f = fopen("/var/sb/scoreboard.txt","r");
  }
  fscanf(f, "%d", &m);
  if (m/10 != 0) {
    b->guest_tens=digi_pat(m/10);
  }
  else {
    b->guest_tens=0;
  }
  b->guest_ones=digi_pat(m%10);
  fscanf(f, "%d", &m);
  if (m/10 != 0) {
    b->home_tens=digi_pat(m/10);
  }
  else {
    b->home_tens=0;
  }
  b->home_ones=digi_pat(m%10);
  fscanf(f, "%d", &m);
  //b->inning2=m;
  b->inning_ones=digi_pat(m%10);
  b->inning_tens=m/10;
  fscanf(f, "%d", &m);
  b->ball=tpow(m)-1;
  fscanf(f, "%d", &m);
  b->strike=tpow(m)-1;
  fscanf(f, "%d", &m);
  b->out=tpow(m)-1;
  fscanf(f, "%d", &m);
  b->hit=m;
  fscanf(f, "%d", &m);
  b->error=m;
  fscanf(f, "%d", &m);
  b->bright=m;
  fclose(f);
}

void txBit(char b)
{
  int d=5;
  b&=1;
  if (b==1) {
    d=20;
  }
  //LOW/HIGH inverted from
  //the output signal because the
  //MAX3232 will invert it
  digitalWrite(0,LOW);
  delayMicroseconds(d);
  digitalWrite(0,HIGH);
  delayMicroseconds(30-d);
} 

//size in bits
void txMsg(unsigned char * m, int size)
{
  int i=0;
  int byte_off, bit_off;
  char b;

  for (i=0; i<size; i++) {
    byte_off=i/8;
    bit_off=i%8;
    b=m[byte_off]>>(7-bit_off);
    txBit(b&1);
    //printf("%d",b&1);
    //printf("%d %02x %02x\n",i,m[byte_off],b&1);
  }
  //printf("\n");
}

int main()
{
  int i;
  unsigned long long tword,t;
  unsigned long long * tp;
  bb_message_t b;

  wiringPiSetup();
  pinMode(0,OUTPUT);
 
  //tword=0x5555555555555555;

  i=0;
  while(1) {
    if (i%5 == 0) {
      fill_struct(&b);
      printMsg(&b);
    }
    txMsg((unsigned char *)&b, 56);
    //delay to next train
    //50 - (30microsec * 56 bits)
    delay(48.32);
    i++;
  }

}
