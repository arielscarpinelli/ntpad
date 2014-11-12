typedef struct
{
        unsigned char box, cross, circle, triangle;
        unsigned char r1, l1, r2, l2;
        unsigned char start, select;
        unsigned char r3, l3;
	unsigned char lf;
        unsigned char dn;
	unsigned char rt;
	unsigned char up;
	unsigned char lx, ly, rx, ry;
        unsigned char motorc, motorg;
        unsigned char buttonmap[16];
        unsigned char min, max;
        NTSTATUS laststatus;
        PUCHAR base;
        int noFF, Initialized, type, noAx, Psx2, ScanMode, Delay, errors;
        int allanalog;
        long Ejes;

} CPSX;
int ScanPSX(CPSX *psx, int i);
void Shock();
