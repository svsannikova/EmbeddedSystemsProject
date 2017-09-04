#include <Gcode.h>
#include <stdio.h>
#include <board.h>
#include <stdio.h>
#include <string.h>

Gcode::Gcode(){

	endofcommand = false;
	i = 0;
}

int Gcode::readfromUart(){
	while(endofcommand == false){
		ch = Board_UARTGetChar();
		if(ch != 255){
			command[i] = ch;
			i++;
			if(ch == '\n'){
				endofcommand = true;
			}
		}
	}
	endofcommand = false;
	i = 0;
	if(command[0] == 'G' && command[1] == '1'){
		char tempg1stuff[5];
		sscanf(command,"%s %c%f %c%f %s",tempg1stuff, &xstuff, &fp.x, &ystuff, &fp.y, a0stuff);
		memset(command,' ', sizeof(command));
		return 1;
	}
	else if(command[0] == 'G' && command[1] == '2' && command[2] == '8'){
		//go home
		memset(command,' ', sizeof(command));
		return 2;
	}
	else if(command[0] == 'M' && command[1] == '1' && command[2] == '0'){
		memset(command,' ', sizeof(command));
		return 3;
		}
	else if(command[0] == 'M' && command[1] == '1'){
		char tempwhatever[5];
		sscanf(command,"%s %d",tempwhatever, &m1param);
		memset(command,' ', sizeof(command));
		return 4;
	}
	else if(command[0] == 'M' && command[1] == '4'){
		char tempwhatev[5];
		sscanf(command,"%s %d",tempwhatev, &m4param);
		memset(command,' ', sizeof(command));

		return 5;
	}
	else{
		memset(command,' ', sizeof(command));
		return 0;
	}
}

void Gcode::sendM10(){
	Board_UARTPutSTR("M10 XY 310 370 0.00 0.00 A0 B0 H0 S80 U160 D90 \n");
	return;
}

void Gcode::sendOk(){
	Board_UARTPutSTR("OK \n");
	return;

}

FloatPoint Gcode::returng1(){
	return fp;
}
int Gcode::returnm1(){
	return m1param;
}

int Gcode::returnm4(){
	return m4param;
}

