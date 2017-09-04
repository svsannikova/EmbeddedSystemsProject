#ifndef XYPLOTTER_H
#define XYPLOTTER_H

#include "GPIO.h"
#include "Pin.h"

class XYPlotter
{
public:
	XYPlotter(Pin &xpos, Pin &xneg, Pin &xdir, Pin &xmove, Pin &ypos, Pin &yneg, Pin &ydir, Pin &ymove);
	void calibrate();
	void receiveCommand();
	int move(int x, int y);
	bool moveTo(int coordinate, int speed);

	Pin &xposLs;
	Pin &xnegLs;

	Pin &xdirPin;
	Pin &xmovePin;

	Pin &yposLs;
	Pin &ynegLs;

	Pin &ydirPin;
	Pin &ymovePin;

	int xSteps;
	int ySteps;

	unsigned long xcurPos, ycurPos, xmaxPos, ymaxPos;

private:


};

#endif // XYPLOTTER_H_INCLUDED


