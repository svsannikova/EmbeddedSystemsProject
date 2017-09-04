/*
 * LimitSwitch.cpp
 *
 *  Created on: 30 Sep 2016
 *      Author: Svetlana
 */

#include "Pin.h"

Pin::Pin(){

}
Pin::Pin(int portno, int pinno) {
	pin=pinno;
	port=portno;
}

Pin::~Pin() {
	// TODO Auto-generated destructor stub
}
