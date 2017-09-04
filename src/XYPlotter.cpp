#include "GPIO.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <cstdio>
#include <string>
#include "string.h"
#include <cstdlib>
#include "ITM_write.h"
#include <cr_section_macros.h>
#include <XYPlotter.h>
#include "RIT.h"

XYPlotter::XYPlotter(Pin &xpos, Pin &xneg, Pin &xdir, Pin &xmove, Pin &ypos, Pin &yneg, Pin &ydir, Pin &ymove) : xposLs(xpos), xnegLs(xneg), xdirPin(xdir), xmovePin(xmove), yposLs(ypos), ynegLs(yneg), ydirPin(ydir), ymovePin(ymove) {
	xcurPos = 0;
	xmaxPos = 0;

	ycurPos = 0;
	ymaxPos = 0;

	xSteps = 0;
	ySteps = 0;
}


void XYPlotter::calibrate()
{
	// X-calibration
	GPIO::setPin(xdirPin, true);
	while(!GPIO::readPin(xposLs) && !GPIO::readPin(xnegLs)){
		RIT_start(10, true, 0, true, 200);
		if(GPIO::readPin(xposLs) || GPIO::readPin(xnegLs)){
			break;
		}
	}

	GPIO::setPin(xdirPin, false);
	while(GPIO::readPin(xposLs) || GPIO::readPin(xnegLs)){
		RIT_start(10, false, 0, true, 200);
	}

	RIT_start(100, false, 0, true, 1000);

	while(!GPIO::readPin(xposLs) && !GPIO::readPin(xnegLs)){
		RIT_start(10, false, 0, true, 200);
		xmaxPos=xmaxPos+10;
		if(GPIO::readPin(xposLs) || GPIO::readPin(xnegLs)){
			break;
		}
	}

	GPIO::setPin(xdirPin, true);
	while(GPIO::readPin(xposLs) || GPIO::readPin(xnegLs)){
		RIT_start(10, true, 0, true, 200);
		xmaxPos=xmaxPos-10;
	}

	RIT_start(100, true, 0, true, 1000);
	xmaxPos=xmaxPos-100;


	// Y-calibration
	GPIO::setPin(ydirPin, true);
	while(!GPIO::readPin(yposLs) && !GPIO::readPin(ynegLs)){
		RIT_start(0, true, 10, true, 200);
		if(GPIO::readPin(yposLs) || GPIO::readPin(ynegLs)){
			break;
		}
	}

	GPIO::setPin(ydirPin, false);
	while(GPIO::readPin(yposLs) || GPIO::readPin(ynegLs)){
		RIT_start(0, false, 10, false, 200);
	}

	RIT_start(0, false, 100, false, 1000);


	while(!GPIO::readPin(yposLs) && !GPIO::readPin(ynegLs)){
		RIT_start(0, true, 10, false, 200);
		ymaxPos=ymaxPos+10;
		if(GPIO::readPin(yposLs) || GPIO::readPin(ynegLs)){
			break;
		}
	}

	GPIO::setPin(ydirPin, true);
	while(GPIO::readPin(yposLs) || GPIO::readPin(ynegLs)){
		RIT_start(0, false, 10, true, 200);
		ymaxPos=ymaxPos-10;
	}

	RIT_start(0, false, 100, true, 1000);
	ymaxPos = ymaxPos-100;

}



int XYPlotter::move(int x, int y)
{
	bool xDirection;
	bool yDirection;

	xSteps = 0;
	ySteps = 0;

	if(x > xcurPos){
		xDirection = true;
		xSteps = x - xcurPos;
		xcurPos = x;
		GPIO::setPin(xdirPin, xDirection);
	}

	if(x < xcurPos){
		xDirection = false;
		xSteps = xcurPos - x;
		xcurPos = x;
		GPIO::setPin(xdirPin, xDirection);
	}

	if(y > ycurPos){
		yDirection = true;
		ySteps = y - ycurPos;
		ycurPos = y;
		GPIO::setPin(ydirPin, yDirection);
	}

	if(y < ycurPos){
		yDirection = false;
		ySteps = ycurPos - y;
		ycurPos = y;
		GPIO::setPin(ydirPin, yDirection);
	}


	RIT_start(xSteps, xDirection, ySteps, yDirection, 1100);

	return 0;
}
