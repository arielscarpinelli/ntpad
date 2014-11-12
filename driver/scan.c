#include "hidgame.h"
#include "stdio.h"
#pragma optimize("at", on)

#define BTN_LEFT        10
#define BTN_RIGHT       9
#define BTN_UP          11
#define BTN_DOWN        12

#define BTN_TRA         13
#define BTN_CLR         14
#define BTN_CRS         15
#define BTN_SQU         16

#define BTN_L_1         17
#define BTN_R_1         18
#define BTN_L_2         19
#define BTN_R_2         20


    unsigned char id;
    int delay;

    PUCHAR base;
    LARGE_INTEGER tres_usec;
    LARGE_INTEGER N64_response_delay;

#define OUTSEGA(a,b) {int aa; for (aa=0; aa<50; aa++) WRITE_PORT_UCHAR((a), (b));}
#define SEGASEL1 0x24
#define SEGASEL0 0x25
#define SNES_PWR (128+64+32+16+8)
#define SNES_CLK 1  // base
#define SNES_LAT 2  // base
#define SNESIT(a) {int i; for (i=0; i<delay; i++) WRITE_PORT_UCHAR(base, (a));}
#define SIN (negate?((READ_PORT_UCHAR(base+1)&snes_din)?1:0):((READ_PORT_UCHAR(base+1)&snes_din)?0:1))
#define NEG(d, n) ((d&n)?0:1)
#define POS(d, n) ((d&n)?1:0)

#define N64_0(on) {_outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, 0xff ); _outp( (USHORT)base, 0xff );}
#define N64_1(on) {_outp( (USHORT)base, on ); _outp( (USHORT)base, on ); _outp( (USHORT)base, 0xFF ); _outp( (USHORT)base, 0xFF ); _outp( (USHORT)base, 0xFF ); _outp( (USHORT)base, 0xFF ); _outp( (USHORT)base, 0xFF); _outp( (USHORT)base, 0xff ); _outp( (USHORT)base, 0xff ); }
void N64_FFInit(register unsigned char on)
{
  register unsigned char i, j;
  _asm cli;                             //cli!
  for(i=0;i<6;i++)
    N64_0(on);

  for(i=0;i<3;i++)
    N64_1(on);

  for(i=0;i<14;i++)
    N64_0(on);

  N64_1(on);

  for(i=0;i<34;i++)
  {
    N64_1(on);
    for(j=0;j<7;j++)
    {
      N64_0(on);
    };
  }
  _asm sti;                             //sti!
  N64_1(on);

};
void N64_FF(register unsigned char on, unsigned char mode)
{
  register unsigned char i;
  _asm cli;                             //cli!
  for(i=0;i<6;i++)
    N64_0(on);

  for(i=0;i<4;i++)
    N64_1(on);

  for(i=0;i<9;i++)
    N64_0(on);

  N64_1(on);
  N64_1(on);
  N64_0(on);
  N64_1(on);
  N64_1(on);

  for(i=0;i<7;i++)
    N64_0(on);

  if(mode)
    N64_1(on)
  else
    N64_0(on);

  _asm sti;                             //sti!
  N64_1(on);
};

int ScanN64(CPSX *n64)
{
        int i;

        unsigned long data, lastdata=0;
        char ejex, ejey, j;
        const int DIN[5] = {64,32,16,8,128};
        const int NEGATE[5]={0, 0, 0,0,  1};
        unsigned char on;
	static unsigned char idon[5] = {0x00, 0xfe /*D0*/, 0xfb /*D2*/, 0xf7 /*D3*/, 0xef /*D4*/};
        int snes_din;
        int negate;
        KIRQL irql;

        snes_din=DIN[id-1];
        negate=NEGATE[id-1];
	
	on = idon[id];
        _asm cli;
        _outp( (USHORT) base, 0xff );    //out dx,al

        for (i=0; i<7; i++)
        {
            N64_0(on);
        }

        N64_1(on);
        N64_1(on);

        _asm sti;                              //sti
        
        WRITE_PORT_UCHAR( base, 0xff );
        KeDelayExecutionThread(KernelMode, FALSE, &N64_response_delay);
        
        data = 0;
	for (i=0; i<32; i++)
	{
                data <<= 1;
                WRITE_PORT_UCHAR( base, 0xfd );
                WRITE_PORT_UCHAR( base, 0xfd );
		data |= (SIN)?0:1;
                WRITE_PORT_UCHAR( base, 0xff );
                WRITE_PORT_UCHAR( base, 0xff );
	}

	if (((data>>23)&1)||((data>>22)&1))  // failed comm
                return 0;
	else
	{
                n64->box = (char)((data>>31) & 1); // a
                n64->cross = (char)((data>>30) & 1); // b
                n64->circle = (char)((data>>29) & 1); // z
                n64->triangle = (char)((data>>28) & 1); // start
		n64->up = (char)((data>>27) & 1);
		n64->dn = (char)((data>>26) & 1);
		n64->lf = (char)((data>>25) & 1);
		n64->rt = (char)((data>>24) & 1);
                n64->l1 = (char)((data>>21) & 1); // l
                n64->r1 = (char)((data>>20) & 1); // r
                n64->select = (char)((data>>19) & 1);  // cup
                n64->start = (char)((data>>18) & 1);  // cdn
                n64->l2 = (char)((data>>17) & 1);  // clf
                n64->r2 = (char)((data>>16) & 1);  // crt
                n64->lx = 127+ejex; // x
                n64->ly = 127-ejey;      // y
                n64->rx = 128;
                n64->ry = 128;
                n64->l3 = 0; n64->r3 = 0;
	}
        if(!n64->noFF)
        {
          if(!n64->Initialized)
          {
                N64_FFInit(on);
                n64->Initialized = 1;
          }          
          if(n64->motorc | (n64->motorg > 10))
          {
            N64_FF(on, 1);
          } else {
            N64_FF(on, 0);
          };
        };
        WRITE_PORT_UCHAR( base, 0xff );    //out dx,al
   return 1;
}

#define OUTP(a,b) {for (aa=0; aa<delay; aa++) WRITE_PORT_UCHAR((a), (b));}
#define OUTPS(a,b) {for (aa=0; aa<8; aa++) WRITE_PORT_UCHAR((a),(b)); }
void ScanSaturn(CPSX *saturn, int analog)
{
	const unsigned char tl1 = 4;
	const unsigned char tr1 = 2;  // pad 1
	const unsigned char th1 = 1;  // pad 1
	const unsigned char gnd1 = 64;// pad 1

	const unsigned char tl2 = 4;
	const unsigned char tr2 = 32; // pad 2
	const unsigned char th2 = 16; // pad 2
	const unsigned char gnd2 =128;// pad 2

	const unsigned char r = 16;
	const unsigned char l = 32;
	const unsigned char d = 64;
	const unsigned char u = 128; // inverted!

        UCHAR s0, s1, s2, s3;
	int sel2, sel1, sel0, gnd;
        UCHAR ad[10];
	unsigned char pwr;
    int aa;


	if (id==1)
	{
		sel2 = th1;
		sel1 = tr1;
		sel0 = tl1;
		gnd = gnd1;
	} else
	{
		sel2 = th2;
		sel1 = tr2;
		sel0 = tl2;
		gnd = gnd2;
	}

	pwr = 255 - (gnd + sel2 + sel1 + sel0);

	if (!analog)
	{
		OUTP( base, pwr+sel2+sel1+sel0 );
                s0=READ_PORT_UCHAR(base+1);

		OUTP( base, pwr     +sel1+sel0 );
                s1=READ_PORT_UCHAR(base+1);

		OUTP( base, pwr+sel2     +sel0 );
                s2=READ_PORT_UCHAR(base+1);

		OUTP( base, pwr          +sel0 );
                s3=READ_PORT_UCHAR(base+1);
	
                saturn->l2 = NEG(s0, 16); // l

                saturn->rt = NEG(s1, 16);
                saturn->lf = NEG(s1, 32);
                saturn->dn = NEG(s1, 64);
                saturn->up = POS(s1, 128);

                saturn->start = NEG(s2, 16);
                saturn->box = NEG(s2, 32); // a
                saturn->circle = NEG(s2, 64); // c
                saturn->cross = POS(s2, 128);  // b

                saturn->r2 = NEG(s3, 16); // r
                saturn->triangle = NEG(s3, 32); // x
                saturn->l1 = NEG(s3, 64); // y
                saturn->r1 = POS(s3, 128); // z
                saturn->lx = saturn->lf?(0):(saturn->rt?(255):(128));
                saturn->ly = saturn->up?(0):(saturn->dn?(255):(128));
                saturn->up = 0; saturn->dn = 0; saturn->lf = 0; saturn->rt = 0;
                saturn->rx = 128;
                saturn->ry = 128;
                saturn->l3 = 0; saturn->r3 = 0; saturn->select = 0; 

	}
	else
	{
		OUTPS( base, pwr+sel2+sel1+sel0 );  // 1st data
		OUTPS( base, pwr     +sel1+sel0 );  // 2nd data
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 3rd
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 4th
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 5th
                ad[0] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 6th
                ad[1] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 7th
                ad[2] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 8th
                ad[3] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 9th
                ad[4] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 10th
                ad[5] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 11th
                ad[6] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 12th
                ad[7] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 13th
                ad[8] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 14th
                ad[9] = READ_PORT_UCHAR(base+1);
		OUTPS( base, pwr          +sel0 );
		OUTPS( base, pwr                );  // 15th
		OUTPS( base, pwr     +sel1      );
		OUTPS( base, pwr     +sel1+sel0 );  // 16th
		OUTPS( base, pwr+sel2+sel1+sel0 );

                saturn->rt = NEG(ad[0],16);
                saturn->lf = NEG(ad[0],32);
                saturn->dn = NEG(ad[0],64);
                saturn->up = POS(ad[0],128);
                saturn->start = NEG(ad[1],16);
                saturn->box = NEG(ad[1],32); // a
                saturn->circle = NEG(ad[1],64); // c
                saturn->cross = POS(ad[1],128); // b
                saturn->r2 = NEG(ad[2],16); // r
                saturn->triangle = NEG(ad[2],32); // x
                saturn->l1 = NEG(ad[2],64); // y
                saturn->r1 = POS(ad[2],128); // z
                saturn->l2 = NEG(ad[3],16);  // l
                saturn->select = NEG(ad[3],32); // a1
                saturn->l3 = NEG(ad[3],64); // a2
                saturn->r3 = POS(ad[3],128);// a3

                saturn->lx = POS(ad[4],16)*128 + POS(ad[4],32)*64 + POS(ad[4],64)*32 + NEG(ad[4],128)*16 +
                                                 POS(ad[5],16)*8 + POS(ad[5],32)*4 + POS(ad[5],64)*2 + NEG(ad[5],128)*1;
                saturn->ly = POS(ad[6],16)*128 + POS(ad[6],32)*64 + POS(ad[6],64)*32 + NEG(ad[6],128)*16 +
                                                 POS(ad[7],16)*8 + POS(ad[7],32)*4 + POS(ad[7],64)*2 + NEG(ad[7],128)*1;
                saturn->ry = POS(ad[8],16)*128 + POS(ad[8],32)*64 + POS(ad[8],64)*32 + NEG(ad[8],128)*16 +
                                                 POS(ad[9],16)*8 + POS(ad[9],32)*4 + POS(ad[9],64)*2 + NEG(ad[9],128)*1;

                saturn->rx = 128;

	}


}

void ScanGenesis( CPSX *genesis, int newsega )
{
        UCHAR c, c2, c3,  s0, s1, s2;
        int aa;

        OUTP(base+2, 0x0c);
        if(!newsega)
        {
    
          OUTP(base, 254); // Power up, select=0
          s0 = READ_PORT_UCHAR(base+1);
          OUTP(base, 255); // Power up, select=1
          s1 = READ_PORT_UCHAR(base+1);
          c = READ_PORT_UCHAR(base+2);
	
          WRITE_PORT_UCHAR(base, 254); // sel=0;
          WRITE_PORT_UCHAR(base, 254); // sel=0;
          WRITE_PORT_UCHAR(base, 255); // sel=1;
          WRITE_PORT_UCHAR(base, 255); // sel=1;
          WRITE_PORT_UCHAR(base, 254); // sel=0;
          WRITE_PORT_UCHAR(base, 254); // sel=0;
          WRITE_PORT_UCHAR(base, 255); // sel=1;
          WRITE_PORT_UCHAR(base, 255); // sel=1;
          s2=READ_PORT_UCHAR(base+1);
          c2=READ_PORT_UCHAR(base+2);
	
          genesis->dn = (c&2)?1:0;
          genesis->up = (c&1)?1:0;
          genesis->lf = (s1&64)?0:1;
          genesis->rt = (s1&128)?1:0;
          genesis->box = (s0&32)?0:1;  // a
          genesis->cross = (s1&32)?0:1;  // b
          genesis->circle = (s1&16)?0:1;  // c
          genesis->start = (s0&16)?0:1;
          genesis->triangle = (c2&1)?1:0;  // x
          genesis->l1 = (c2&2)?1:0;  // y
          genesis->r1 = (s2&64)?0:1; // z
          genesis->select = (s2&128)?1:0;  // mode
        } else {
                                
          if(id!=1)
          {
            OUTP(base+2, SEGASEL0);
            c = READ_PORT_UCHAR(base+1);

            OUTP(base+2, SEGASEL1); 
            s1 = READ_PORT_UCHAR(base);
            c2 = READ_PORT_UCHAR(base+1);
	
            OUTP(base+2, SEGASEL0);
            OUTP(base+2, SEGASEL1);
            s2 = READ_PORT_UCHAR(base);
            c3 = READ_PORT_UCHAR(base+1);
	
            genesis->dn = (s1&128)?0:1;
            genesis->up = (s1&64)?0:1;
            genesis->lf = (c2&64)?0:1;
            genesis->rt = (c2&128)?1:0;
            genesis->box = (c&32)?0:1;  // a
            genesis->cross = (c2&32)?0:1;  // b
            genesis->circle = (c2&16)?0:1;  // c
            genesis->start = (c&16)?0:1;
            genesis->triangle = (c3&64)?0:1;  // x
            genesis->l1 = (s2&128)?0:1;  // y
            genesis->r1 = (s2&64)?0:1; // z
            genesis->select = (c3&128)?1:0;  // mode
          } else {

            OUTP(base+2, SEGASEL0);
            s0 = READ_PORT_UCHAR(base);
            OUTP(base+2, SEGASEL1); 
            s1 = READ_PORT_UCHAR(base);
            OUTP(base+2, SEGASEL0);
            OUTP(base+2, SEGASEL1);
            s2 = READ_PORT_UCHAR(base);

            genesis->dn = (s0&2)?0:1;
            genesis->up = (s0&1)?0:1;
            genesis->lf = (s1&4)?0:1;
            genesis->rt = (s1&8)?0:1;
            genesis->box = (s0&16)?0:1;  // a
            genesis->cross = (s1&16)?0:1;  // b
            genesis->circle = (s1&32)?0:1;  // c
            genesis->start = (s0&32)?0:1;
            genesis->triangle = (s2&4)?0:1;  // x
            genesis->l1 = (s2&2)?0:1;  // y
            genesis->r1 = (s2&1)?0:1; // z
            genesis->select = (s2&8)?0:1;  // mode
         }
         WRITE_PORT_UCHAR(base+2, 5); // leave powered, disable bidir, sel off
        }
        genesis->lx = genesis->lf?(0):(genesis->rt?(255):(128));
        genesis->ly = genesis->up?(0):(genesis->dn?(255):(128));
        genesis->up = 0; genesis->dn = 0; genesis->lf = 0; genesis->rt = 0;
        genesis->rx = 128;
        genesis->ry = 128;
        genesis->l2 = 0; genesis->r2 = 0; genesis->l3 = 0; genesis->r3 = 0;
}


void ScanAtari( CPSX *atari)
{
	int c;
    int aa;
	//	Atari		IBM
	//up	1		1	(c0-)
	//dn	2		14	(c1-)
	//lf	3		16	(c2+)
	//rt	4		17	(c3-)
	//btn	6		11	(s7-)  (connect 11 to 4p7R to 2 (d0), drive d0 1)
	//btn2	9		12	(s5+)  (connect 12 to 4p7R to 3 (d1), drive d1 1)
	//gnd	8		18
	OUTP(base+2, 4); //All C's high, disconnected
	OUTP(base, 2+1); // Power joy button sensor
        c = READ_PORT_UCHAR(base+1);
        atari->box = (c & 128)?1:0;
        atari->cross = (c & 32)?0:1;
        c=READ_PORT_UCHAR(base+2);
	atari->up = (c&1)?1:0;
	atari->dn = (c&2)?1:0;
	atari->lf = (c&4)?0:1;
	atari->rt = (c&8)?1:0;
        atari->lx = atari->lf?(0):(atari->rt?(255):(128));
        atari->ly = atari->up?(0):(atari->dn?(255):(128));
        atari->up = 0; atari->dn = 0; atari->lf = 0; atari->rt = 0;
        atari->rx = 128;
        atari->ry = 128;
        atari->circle = 0; atari->triangle = 0; atari->select = 0; atari->start = 0;
        atari->l1 = 0; atari->r1 = 0; atari->l2 = 0; atari->r2 = 0; atari->l3 = 0; atari->r3 = 0;

}

void ScanSNES(CPSX *psx, int snes)
{
   int pad_number;
   const int DIN[5] = {64,32,16,8,128};
   const int NEGATE[5]={0, 0, 0,0,  1};

   int snes_din;
   int negate;

   pad_number = id - 1;
   snes_din=DIN[pad_number];
   negate=NEGATE[pad_number];

   SNESIT(SNES_PWR+SNES_CLK); // Power up!
   SNESIT(SNES_PWR+SNES_LAT+SNES_CLK); // Latch it!
   SNESIT(SNES_PWR+SNES_CLK);
   psx->cross = SIN; // B
   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   if(snes)
     psx->circle = SIN; // Y
   else
     psx->box = SIN; // A de NES

   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   psx->select = SIN;
   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   psx->start = SIN;
   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   psx->up = SIN;
   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   psx->dn = SIN;
   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   psx->lf = SIN;
   SNESIT(SNES_PWR);
   SNESIT(SNES_PWR+SNES_CLK);
   psx->rt = SIN;
   if(snes)
   {
     SNESIT(SNES_PWR);
     SNESIT(SNES_PWR+SNES_CLK);
     psx->box = SIN; // A
     SNESIT(SNES_PWR);
     SNESIT(SNES_PWR+SNES_CLK);
     psx->triangle = SIN; // X
     SNESIT(SNES_PWR);
     SNESIT(SNES_PWR+SNES_CLK);
     psx->l1 = SIN; // L
     SNESIT(SNES_PWR);
     SNESIT(SNES_PWR+SNES_CLK);
     psx->r1 = SIN; // R
   };
   WRITE_PORT_UCHAR(base, 0); // Power it down
   psx->lx = psx->lf?(0):(psx->rt?(255):(128));
   psx->ly = psx->up?(0):(psx->dn?(255):(128));
   psx->up = 0; psx->dn = 0; psx->lf = 0; psx->rt = 0;
   psx->rx = 128;
   psx->ry = 128;
   psx->l2 = 0; psx->r2 = 0; psx->l3 = 0; psx->r3 = 0;
};

// =========================
// | o o o | o o o | o o o | Controller plug
//  \_____________________/
//   1 2 3   4 5 6   7 8 9
//
//   Controller          Parallel
//   1 - Data              10(pad1),13(pad2)
//   2 - Command           2
//   3 - 9V(shock)         +9V battery terminal
//   4 - GND               18 , also -9V battery terminal
//   5 - V+                6,7,8,9 through diodes
//   6 - ATT               3
//   7 - Clock             4
//   9 - ack               12(pad1), 15(pad2)

#define PS_CMD 0x01        // Bit 1 base
#define PS_ATT 0x02        // Bit 2 base
#define PS_CLK 0x04        // Bit 3 base (parallel port)
#define PS_PWR 0xf8        // Bit 4-8
#define PS_CMD_OFF 0xFE    // Bit 1 one complement
#define PS_DAT_PAD1 0x40 
#define PS_DAT_PAD2 0x10
#define PS_ACK_PAD1 0x20
#define PS_ACK_PAD2 0x08

unsigned char psxsendb;
unsigned char dat;
unsigned char ack;

unsigned char SendByte(register unsigned char byte, unsigned char wait)
{
    register int i;
	register unsigned char data;

	data=0;
    for (i=0; i<8; i++)
	{
        psxsendb ^= PS_CLK; // turn off clock. it has to be allways on....
        if(byte &(1<<i))
            psxsendb |= PS_CMD;
        else
            psxsendb &= PS_CMD_OFF;
        WRITE_PORT_UCHAR(base, psxsendb);
        KeStallExecutionProcessor(delay); // Clock lenght
		data |= ((READ_PORT_UCHAR(base+1)&dat)?(1<<i):0);
		psxsendb |= PS_CLK; // turn on clock
        WRITE_PORT_UCHAR(base, psxsendb);
        KeStallExecutionProcessor(delay);
	}
	// Wait for ACK;
    for(i=0; wait && (i<30) && (READ_PORT_UCHAR(base+1)&ack); i++);
	return data;
}


void PSXInit(int psx2mode, int allanalog, int noFF)
{
        unsigned char i, j;
        const short * pString;
        const short ShockString[5][11]= {
                {0x1, 0x43, 0x00, 0x01, 0x00, 0x01, -1},
                {0x1, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, -1},
                {0x1, 0x4d, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x01, -1},
                {0x1, 0x4f, 0x00, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x01, -1},
                {0x1, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, -1} };        

        WRITE_PORT_UCHAR(base, PS_PWR | PS_CLK | PS_ATT); // First power "ever"
        KeDelayExecutionThread(KernelMode, FALSE, &tres_usec); // Give it some time to start
        
        for (i=0; i<5; i++)
	    {
                if((i==1)&&(!allanalog))
                  i++;
                if((i==2)&&(noFF))
                  i++;
                if((i==3)&&(!psx2mode))
                  i++;
                
                WRITE_PORT_UCHAR(base, psxsendb); // psxsendb is loaded with the right SEL
                KeStallExecutionProcessor(delay*4); // Sel to first bit / clock delay

                pString = ShockString[i];
                TRAP();
                for (j=0; pString[j+1]!=-1; j++)
                    SendByte((unsigned char)pString[j],1);
                SendByte((unsigned char)pString[j],0);

                WRITE_PORT_UCHAR(base, PS_PWR | PS_CLK | PS_ATT); // Leave it powered and running
                KeDelayExecutionThread(KernelMode, FALSE, &tres_usec); // Wait beetween commands
	    }
}


int ScanPSX(CPSX *psx, int i)
{
        static unsigned char buffer[40];
        const unsigned char attary[] = { 0, 0x02, 0x08, 0x10, 0x20, 0x40 };
        unsigned char attval;
        char *data;
        data = buffer;
        id = (unsigned char)i;
        base = psx->base;
        delay = psx->Delay;
        switch(psx->type)
        {
          case 0:
          case 1:  // All PSX types
          case 2:
             break;             
          case 3:
             ScanSNES(psx, 1);
             return 1;
          case 4:
             ScanSNES(psx, 0);
             return 1;
          case 5:
             ScanAtari(psx);
             return 1;
          case 6:
             ScanGenesis(psx, 0);
             return 1;
          case 7:
             ScanGenesis(psx, 1);
             return 1;
          case 8:
             ScanSaturn(psx, 0);
             return 1;
          case 9:
             ScanSaturn(psx, 1);
             return 1;
          case 10:
             return ScanN64(psx);
        };

	    if (psx->type == 1) // is a multitap (up to 5 pads interface)
        {
		    attval = attary[id];
            dat = PS_DAT_PAD1;
            ack  = PS_ACK_PAD1;
        }
	    else // regular or megatap directly conected
        {
		    attval = PS_ATT;
		    if (id==1)
            {
                dat = PS_DAT_PAD1;
                ack  = PS_ACK_PAD1;
            } else {
                dat = PS_DAT_PAD2;
                ack  = PS_ACK_PAD2;
            }
        }

        psxsendb = (PS_PWR | PS_CLK | PS_ATT) ^ attval;  // Turn off sel bit (it is inverted)

        if(!psx->Initialized)
        {
            PSXInit(psx->Psx2, psx->allanalog, psx->noFF);
            psx->Initialized = 1;
        }

        WRITE_PORT_UCHAR(base, psxsendb);
        KeStallExecutionProcessor(delay*4); // Sel to first bit / clock delay

        if(psx->type != 2) // is not megatap directly connected
        {
          data[0]=SendByte(0x01,1);
          data[1]=SendByte(0x42,1);
          data[2]=SendByte(0x00,1);
          data[3]=SendByte(psx->motorc,1);
          switch (data[1])
          {
             case 0x53:
                  data[4]=SendByte(psx->motorg,1);
                  data[5]=SendByte(0x00,1);
                  data[6]=SendByte(0x00,1);
                  data[7]=SendByte(0x00,1);
                  data[8]=SendByte(0x00,0);
                  break;
              case 0x73:
              case 0x23:
              case 0xf3:
                  data[4]=SendByte(psx->motorg,1);
                  data[5]=SendByte(0x00,1);
                  data[6]=SendByte(0x00,1);
                  data[7]=SendByte(0x00,1);
                  data[8]=SendByte(0x00,1);
                  data[9]=SendByte(0x00,0);
                  break;
              case 0x79:
                  data[4]=SendByte(psx->motorg,1);
                  data[5]=SendByte(0x00,1);
                  data[6]=SendByte(0x00,1);
                  data[7]=SendByte(0x00,1);
                  data[8]=SendByte(0x00,1);
                  data[9]=SendByte(0x00,1);
                  data[10]=SendByte(0x00,1);
                  data[11]=SendByte(0x00,1);
                  data[12]=SendByte(0x00,1);
                  data[13]=SendByte(0x00,1);
                  data[14]=SendByte(0x00,1);
                  data[15]=SendByte(0x00,1);
                  data[16]=SendByte(0x00,1);
                  data[17]=SendByte(0x00,1);
                  data[18]=SendByte(0x00,1);
                  data[19]=SendByte(0x00,1);
                  data[20]=SendByte(0x00,1);
                  data[21]=SendByte(0x00,1);
                  break;
              case 0x41:
              default:
                  data[4]=SendByte(psx->motorg,0);
                  break;
          };   
        } else {
          if(id==1)
          {
            SendByte(0x01,1);
            SendByte(0x42,1);
            SendByte(0x01,1);
            for(i=0;i<4;i++)
            {
              data = buffer + (i * 10);
              data[1] = SendByte(0x42,1);
              data[2] = SendByte(0x00,1);
              data[3] = SendByte(psx->motorc,1);
              data[4] = SendByte(psx->motorg,1);
              data[5] = SendByte(0x00,1);
              data[6] = SendByte(0x00,1);
              data[7] = SendByte(0x00,1);
              data[8] = SendByte(0x00,1);
            };
          } else {
            data = buffer + ((id - 1) * 10);
          };
        };
        
        WRITE_PORT_UCHAR(base, PS_PWR | PS_CLK | PS_ATT); // Leave it powered and running

        if(data[1] == 0xff) return 0;
        psx->start = data[3]&0x08?0:1;
        psx->select = data[3]&0x01?0:1;
        psx->r3 = data[3]&0x04?0:1;
        psx->l3 = data[3]&0x02?0:1;
        if(data[1]!=0x79)
        {
          psx->lf = data[3]&0x80?0:1;
          psx->dn = data[3]&0x40?0:1;
          psx->rt = data[3]&0x20?0:1;
          psx->up = data[3]&0x10?0:1;
          psx->cross = data[4]&0x40?0:255;
          psx->circle = data[4]&0x20?0:255;
          psx->l2 = data[4]&0x01?0:1;
        };
	switch (data[1]) {
		case 0x73: //analog red mode
		case 0x53: //analog green mode
                        psx->l1 = data[4]&0x04?0:1;
                        psx->r1 = data[4]&0x08?0:1;
                        psx->triangle = data[4]&0x10?0:1;
                        psx->r2 = data[4]&0x02?0:1;
                        psx->box = data[4]&0x80?0:1;
			break;
                case 0x79:
                        psx->lf = data[BTN_LEFT];
                        psx->dn = data[BTN_DOWN];
                        psx->rt = data[BTN_RIGHT];
                        psx->up = data[BTN_UP];
                        psx->cross = data[BTN_CRS];
                        psx->circle = data[BTN_CLR];
                        psx->l2 = data[BTN_L_2];
                        psx->box = data[BTN_SQU];
                        psx->triangle = data[BTN_TRA];
                        psx->r1 = data[BTN_R_1];
                        psx->l1 = data[BTN_L_1];
                        psx->r2 = data[BTN_R_2];
                        break;
                default:
                        psx->box = data[4]&0x80?0:255;
                        psx->triangle = data[4]&0x10?0:255;
                        psx->r1 = data[4]&0x08?0:1;
                        psx->l1 = data[4]&0x04?0:1;
                        psx->r2 = data[4]&0x02?0:1;
			break;
	}
	if (data[1]!=0x41) {
                psx->rx = data[5];
                psx->ry = data[6];
                psx->lx = data[7];
                psx->ly = data[8];
	} else {
                psx->rx = 128;
                psx->ry = 128;
                if(!psx->noAx)
                {
                  psx->lx = (psx->lf>0)?(0):((psx->rt>0)?(255):(128));
                  psx->ly = (psx->up>0)?(0):((psx->dn>0)?(255):(128));
                  psx->up = 0; psx->dn = 0; psx->lf = 0; psx->rt = 0;
                } else {
                  psx->lx = 128;
                  psx->ly = 128;
                };

	}
        return 1;
};

VOID ScaningThread(PDEVICE_OBJECT DeviceObject)
{
  PDEVICE_EXTENSION DeviceExtension;
  PVOID objetos[2];
  NTSTATUS status;
  LARGE_INTEGER autoscan;
  DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);
  objetos[0] = &(DeviceExtension->FinishThread);
  objetos[1] = &(DeviceExtension->DoScan);
  tres_usec.QuadPart = -200;
  N64_response_delay.QuadPart = -1500;
  autoscan.QuadPart = -160000;
  while(1)
  {
        if(DeviceExtension->psx.ScanMode == 0)
          status = KeWaitForMultipleObjects(2,objetos, WaitAny, Executive, UserMode, TRUE, NULL, NULL);
        else
          status = KeWaitForMultipleObjects(2,objetos, WaitAny, Executive, UserMode, TRUE, &autoscan, NULL);
        if((status != STATUS_ALERTED) & (status != STATUS_USER_APC))
        {
          if(status == 0)
            PsTerminateSystemThread(STATUS_SUCCESS);
          if((status == STATUS_TIMEOUT) && (DeviceExtension->querystill == 0))
            continue;
          DeviceExtension->querystill = 0;
          ExAcquireFastMutex(&Global.Mutex);
          if(ScanPSX(&(DeviceExtension->psx), DeviceExtension->SerialNo))
          {
                  DeviceExtension->psx.errors = 0;
                  DeviceExtension->psx.laststatus = STATUS_SUCCESS;
          }
          else
          {
                  if(DeviceExtension->psx.errors < 100)
                  {
                    DeviceExtension->psx.errors++;
                    DeviceExtension->psx.Initialized = 0;
                    DeviceExtension->psx.laststatus = STATUS_SUCCESS;
                  }
                  else
                    DeviceExtension->psx.laststatus = STATUS_DEVICE_NOT_CONNECTED;                   
          };                
          ExReleaseFastMutex(&Global.Mutex);
          KeSetEvent(&(DeviceExtension->ScanReady), 0, FALSE);
        };
  };
};

