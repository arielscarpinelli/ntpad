class CPSX
{
public:
	unsigned char dn;
	unsigned char up;
	unsigned char lf;
	unsigned char rt;
	unsigned char start, select, box, cross, circle, triangle;
    unsigned char r1, l1, r2, l2, r3, l3;
	unsigned char lx, ly, rx, ry;
    unsigned char motorc, motorg;
	unsigned char type;
	int ScanPSX();
    void Init(int b, int i, int d, int m)
			 {p0 = 0xf8+4+2+1; base = b; id = i; delay = d; multitap = m;
				motorc = 0; motorg = 30;};
private:
    int delay;
    unsigned int multitap, base, id;
    unsigned char p0;
    void Sel(int i);
	void Clk(int i);
	void Cmd(int i);
    int Ack();
	int Dat();
	void Slow();
	unsigned char SendByte(unsigned char byte, int wait);
	void SendPSXString(int string[]);
	void Shock();
};

