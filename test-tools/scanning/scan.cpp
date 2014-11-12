#include "NTPAD.h"
#include <conio.h>
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



void CPSX::Clk(int i)
{
const char clk=0x04;            // Bit 3 Base+0 (parallel port)

	if (i)
		p0 |= clk;
	else
		p0 &= ~clk;

        _outp(base+0, p0);
}

void CPSX::Sel(int i)
{
const unsigned char power=0xf8;          // Bits 3-7 Base+0 (parallel port)
const char att=0x02;            // Bit 2 Base+0 (parallel port)
unsigned char attary[] = { 0, 0x02, 0x08, 0x10, 0x20, 0x40 };
unsigned char attval;

	if (multitap)
		attval = attary[id];
	else
		attval = att;

	p0 |= power;

	if (i)
		p0 |= attval;
	else
		p0 &= ~attval;

       _outp(base+0, p0);
}

void CPSX::Cmd(int i)
{
const char cmd=0x01;            // Bit 1 Base+0 (parallel port)
	if (i)
		p0 |= cmd;
	else
		p0 &= ~cmd;

        _outp(base+0, p0);
}


int CPSX::Dat()
{
	unsigned char data;
	if (multitap)
		data = 0x40;
	else
	{
		if (id==1) data = 0x40;
		else data = 0x10;
	}
	
        if (_inp(base+1)&data)
		return 1;
	else
		return 0;
}

int CPSX::Ack()
{
	unsigned char ack;
	if (multitap)
		ack = 0x20;
	else
	{
		if (id==1) ack = 0x20;
		else ack = 0x08;
	}

        if (_inp(base+1)&ack)
		return 1;
	else
		return 0;
}

void CPSX::Slow()
{
	int i;
	if (delay==3)
	{
		for (i=0; i<3; i++)
                        _outp(base+0, p0);
	} else
	{
		for (i=0; i<delay; i++)
                        _outp(base+0, p0);
	}
}

unsigned char CPSX::SendByte(unsigned char byte, int wait)
{
	int i,j; //,k;
	unsigned char data;

	data=0;
	for (i=0; i<8; i++)
	{
		Slow();
		Cmd(byte&(1<<i));
		Clk(0);
		Slow();
		data |= (Dat()?(1<<i):0);  // Moved prior to clock rising edge
		Clk(1);
	}
	// Wait for ACK;
	for(j=0; wait && j<30 && Ack(); j++);
//	for(k=0; wait && k<30 && !Ack(id, base); k++);
//	if (j==300 || k==300) data=0;
	return data;
}


void CPSX::SendPSXString(int string[])
{
	int i;
	
	Sel(1);
 	Clk(1);
	Cmd(1);
    Slow();
	Slow();
	Slow();
	Slow();
	Sel(0);
	Slow();
	Slow();
	Slow();

  for (i=0; string[i+1]!=-1; i++)
		SendByte((unsigned char)string[i],1);
	SendByte((unsigned char)string[i],0);

	Slow();
	Sel(1);
	Cmd(1);
	Clk(1);
	Slow();
	Slow();

}

void CPSX::Shock()
{
	int i,j;
	static int ShockString[3][12]= {
		{0x01, 0x43, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, -1},
		{0x01, 0x4d, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x01, -1},
		{0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, -1}};

	for (i=0; i<3; i++)
	{
		Slow();
		Slow();
		Slow();
		SendPSXString( ShockString[i] );
		
	}

}


int CPSX::ScanPSX()
{
	unsigned char data[10];
	multitap = 1;
	id = 2;
    Shock();
	Sel(1);
	Clk(1);
	Cmd(1);
	Slow();
	Slow();
	Slow();
	Slow();
	Slow();
	Sel(0);
	Slow();
	Slow();
	Slow();
	Slow();
    data[0]=SendByte(0x01,1);
	data[1]=SendByte(0x42,1);
	data[2]=SendByte(0x00,1);
	data[3]=SendByte(motorc,1);
	switch (data[1]) {
		case 0x23:
		case 0xf3:
		case 0x73: //analog red mode
			data[4]=SendByte(motorg,1);
			data[5]=SendByte(0x00,1);
			data[6]=SendByte(0x00,1);
			data[7]=SendByte(0x00,1);
			data[8]=SendByte(0x00,1);
			data[9]=SendByte(0x00,0);
			break;
		case 0x53: //analog green mode
			data[4]=SendByte(motorg,1);
			data[5]=SendByte(0x00,1);
			data[6]=SendByte(0x00,1);
			data[7]=SendByte(0x00,1);
			data[8]=SendByte(0x00,0);
		default:
			data[4]=SendByte(motorg,0);
			break;
	};
	Slow();
	Slow();
	Sel(1);
	Cmd(1);
	Clk(1);
	Slow();
	if (data[1] == 0xff) return 0;
	lf = data[3]&0x80?0:1;
	dn = data[3]&0x40?0:1;
	rt = data[3]&0x20?0:1;
	up = data[3]&0x10?0:1;
    start = data[3]&0x08?0:1;
	select = data[3]&0x01?0:1;
	cross = data[4]&0x40?0:1;
	circle = data[4]&0x20?0:1;
	l2 = data[4]&0x01?0:1;
	r3 = data[3]&0x04?0:1;
	l3 = data[3]&0x02?0:1;
	if (data[1]!=0x41) {
		rx = data[5];
		ry = data[6];
		lx = data[7];
		ly = data[8];
	} else {
		rx = 128;
		ry = 128;
		lx = lf?(0):(rt?(255):(128));
		ly = up?(0):(dn?(255):(128));
	}
	switch (data[1]) {
		case 0x73: //analog red mode
		case 0x53: //analog green mode
			l1 = data[4]&0x04?0:1;
			r1 = data[4]&0x08?0:1;
			triangle = data[4]&0x10?0:1;
			r2 = data[4]&0x02?0:1;
			box = data[4]&0x80?0:1;
			break;
		default:
			box = data[4]&0x80?0:1;
			triangle = data[4]&0x10?0:1;
			r1 = data[4]&0x08?0:1;
			l1 = data[4]&0x04?0:1;
			r2 = data[4]&0x02?0:1;
			break;
	}
	return 1;
};


