
struct FloatPoint{
	float x;
	float y;
};

class Gcode
{
	public:
	Gcode();
	int readfromUart();
	void sendOk();
	void sendM10();
	FloatPoint returng1();
	int returnm1();
	int returnm4();

	private:
	char command[100];
	bool endofcommand;
	char xstuff;
	char ystuff;
	char a0stuff[5];
	FloatPoint fp;
	int m1param;
	char ch;
	int i;
	int m4param;



};
