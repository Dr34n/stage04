/*******************************************************************************
 * Copyright (c) 2022 Gerasimenko Evgeniy (evgenyger.work@gmail.com)           *
 *                                                                             *
 * The MIT License (MIT):                                                      *
 * Permission is hereby granted, free of charge, to any person obtaining a     *
 * copy of this software and associated documentation files (the "Software"),  *
 * to deal in the Software without restriction, including without limitation   *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell   *
 * copies of the Software, and to permit persons to whom the Software is       *
 * furnished to do so, subject to the following conditions:                    *
 * The above copyright notice and this permission notice shall be included     *
 * in all copies or substantial portions of the Software.                      *
 *                                                                             *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,             *
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR       *
 * OTHER DEALINGS IN THE SOFTWARE.                                             *
 ******************************************************************************/

#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <ads1115.h>
#include <time.h>
#include <stdio.h>

#define AD_BASE 122
#define AD_ADDR 0x48 //i2c address

#define ADC_PIN 2


void help()
{
	printf("    Use this application for reading from rangefinder\n");
	printf("    execute format: ./range TIME \n");
	printf("    return: length in cm\n");
	printf("    TIME - pause between writing in ms\n");
	printf("    -h - help\n");
	printf("    -q - quiet\n");
}

int clamp(int x, int min, int max)
{
	return (x < min) ? min : ((x > max) ? max : x);
}

int main(int argc, char *argv[])
{
	//?????????????? ???????????? ?????????? + ????????????????????
	int idc;
	int t_hours, t_minutes, t_seconds, t_ms;
	struct timespec stop_t;
	idc = clock_gettime(CLOCK_REALTIME, &stop_t);
	//?????????????? ????????????????????
	int quiet = 0;
	if (argc > 1) {
		if ((strcmp(argv[1], "-h") == 0)) {
			help();
			return 0;
		} else {
			if ((strcmp(argv[1], "-q") == 0)) {
				quiet = 1;
			}
		}
	}

	if ((quiet && argc != 4)) {
		help();
		return 0;
	}

	if (!quiet)
		printf("\nThe rangefinder application was started\n\n");

	int argument = 1;
	if (quiet)
		argument++;
	int delay_ms = atoi(argv[argument]);
	//channel - ???????????????? ????????????
	char* channel = argv[argument + 1];
	
	//?????????????????????????? ??????????
	char res[6]; //???????????? ?????? ????????????????????; 5 ???????????? + ????????????????????
	wiringPiSetup();
	ads1115Setup(AD_BASE, AD_ADDR);
	digitalWrite(AD_BASE, 0);
	//?????????? ???????? ????????????????????????????
	int channel_fd = open(channel, O_WRONLY);
	if (channel_fd <= 0){
		printf("Failed to open channel, exiting");
		exit(-1);
	}
	close(channel_fd);
	while (1) {
		//?????????????????? ??????????????????
		int ADC_VAL =analogRead(AD_BASE + ADC_PIN);
		//???????????????????? ??????????
		idc = clock_gettime(CLOCK_REALTIME, &stop_t);
		t_hours = (stop_t.tv_sec/3600)%24;
		t_minutes = (stop_t.tv_sec/60)%60;
		t_seconds = (stop_t.tv_sec)%60;
		t_ms	  = (stop_t.tv_nsec)/1000000;
		//?????????????? ?????? ?????? ????????
		if (!quiet){
			printf("%d:%d:%d.%3d\tADC: %5d \n", t_hours, t_minutes, t_seconds, t_ms, ADC_VAL);
			fflush(stdout);
		}
		else{
			sprintf(res, "%5d", ADC_VAL);
			//printf("FROM PRINTF %5d\n", ADC_VAL);
			channel_fd = open(channel, O_WRONLY);
			write(channel_fd, res, strlen(res));
			close(channel_fd);
		}
		//????????
		usleep(1000 * delay_ms);
	}
	return 0;
}
