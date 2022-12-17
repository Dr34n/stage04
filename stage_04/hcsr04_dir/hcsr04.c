
/*******************************************************************************
 * Copyright (c) 2022 Sergey Balabaev (sergei.a.balabaev@gmail.com)                      *
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

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

//***************************//
#define TRIG 8 // GPIO PIN TRIG
#define ECHO 11 // GPIO PIN ECHO
//***************************//

void Exiting(int);

int read_pins_file(char *file)
{
	FILE *f = fopen(file, "r");
	if (f == 0) {
		fprintf(stderr, "ERROR: can't open %s file\n", file);
		return -1;
	}

	char str[32];
	while (!feof(f)) {
		if (fscanf(f, "%s\n", str))
			printf("%s\n", str);
		fflush(stdout);
		sleep(1);
	}
	fclose(f);

	return 0;
}

static int GPIOExport(int pin)
{
	#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		Exiting(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

static int GPIOUnexport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		Exiting(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

static int GPIODirection(int pin, int dir)
{
	static const char s_directions_str[] = "in\0out";

	#define DIRECTION_MAX 35
	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		Exiting(-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3],
			IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		Exiting(-1);
	}

	close(fd);
	return (0);
}

static int GPIORead(int pin)
{
	#define VALUE_MAX 30
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		Exiting(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		Exiting(-1);
	}

	close(fd);

	return (atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		Exiting(-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		Exiting(-1);
	}

	close(fd);
	return (0);
}

void Exiting(int parameter)
{
	GPIOUnexport(TRIG);
	GPIOUnexport(ECHO);
	exit(parameter);
}

void Exiting_sig()
{
	GPIOUnexport(TRIG);
	GPIOUnexport(ECHO);
	exit(0);
}

void help()
{
	printf("    Use this application for reading from rangefinder\n");
	printf("    execute format: ./range TIME \n");
	printf("    return: search time in ms\n");
	printf("    TIME - pause between writing in ms\n");
	printf("    -h - help\n");
	printf("    -q - quiet\n");
}

#define TIMEOUT_SEC 2

int main(int argc, char *argv[])
{
	//пробный запуск часов + переменные
	int idc;
	int t_hours, t_minutes, t_seconds, t_ms;
	struct timespec stop_t;
	if (clock_gettime(CLOCK_REALTIME, &stop_t) == -1){
		printf("Failed to get time, exiting\n");
		exit(-1);
	};
	//парсинг аргументов
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
	char *mode = argv[1 + quiet];
	//----------------------------------------
	//незадокументированный ручной режим? *теперь недостижим
	if (strcmp(mode, "-s") == 0) {
		char data[32];
		while (1) {
			scanf("%s", data);
			fflush(stdin);
			printf("%s\n", data);
			fflush(stdout);
		}
	}
	//конец ручного режима
	//----------------------------------------
	//продолжение парсинга аргументов
	
	//s(leep)l(ength)
	int argument = 1;
	if (quiet)
		argument++;
	int sl = atoi(argv[argument]);
	//channel - название канала
	char* channel = argv[argument + 1];
	
	//инициализация всего
	char res[6]; //буффер для результата; 5 знаков + терминатор
	int search_time = 0;
	signal(SIGINT, Exiting_sig);
	GPIOExport(TRIG);
	GPIOExport(ECHO);
	sleep(0.05);
	GPIODirection(TRIG, OUT);
	GPIODirection(ECHO, IN);
	//пробуем открыть канал
	int channel_fd = open(channel, O_WRONLY);
	if (channel_fd <= 0){
		printf("Failed to open channel, exiting");
		exit(-1);
	}
	close(channel_fd);
	//главный цикл
	while (1) {
		//пускаем импульс
		GPIOWrite(TRIG, 1);
		usleep(10);
		GPIOWrite(TRIG, 0);
		//ждем эха (можем не дождаться?)
		while (!GPIORead(ECHO)) {
		}
		//замеряем время высокого аута
		clock_t start_time = clock();
		int flag = 0;
		while (GPIORead(ECHO)) {
			if (clock() - start_time >
			    TIMEOUT_SEC * CLOCKS_PER_SEC) {
				flag = 1;
				break;
			}
		}
		//continue если аут слишком долго высокиий
		if (flag) {
			printf("Timeout reached, sleeping for one second\n");
			sleep(1);
			continue;
		}
		//останавливаем таймер
		clock_t end_time = clock();
		//сразу после записываем время замера
		idc = clock_gettime(CLOCK_REALTIME, &stop_t);
		t_hours = (stop_t.tv_sec/3600)%24;
		t_minutes = (stop_t.tv_sec/60)%60;
		t_seconds = (stop_t.tv_sec)%60;
		t_ms	  = (stop_t.tv_nsec)/1000000;
		search_time = (end_time - start_time)/(CLOCKS_PER_SEC/1000);
		//выводим результаты так или эдак
		if (!quiet){
			printf("%2d:%2d:%2d.%3d\tsignal_delay: %5d ms\n", t_hours, t_minutes, t_seconds, t_ms, search_time);
			fflush(stdout);
		}
		else{
			sprintf(res, "%d", search_time);
			channel_fd = open(channel, O_WRONLY);
			write(channel_fd, res, strlen(res));
			close(channel_fd);
		}
		//спим
		if ((sl > 0) && (sl < 60000))
			usleep(sl * 1000);
		else
			sleep(1);
	}
	return 0;
}
