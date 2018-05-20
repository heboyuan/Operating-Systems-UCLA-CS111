#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <mraa.h>
#include <time.h>
#include <poll.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

sig_atomic_t volatile running = 1;
int period = 1;
char scale = 'F';
int my_log = 0;
int my_temp_log;
FILE *my_log_file = NULL;
int scale_change = 0;
int period_change = 0;
int on = 1;
int tid_arr[2] = {0, 1};
char buffer[2048];
char t_buffer[10];
const int B = 4275;
const int R0 = 100000; 


void turn_off(){
	running = 0;
	time_t rawtime;
	struct tm *timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	on = 0;
	strftime(t_buffer, 10, "%X", timeinfo);
	if(my_log){
		fprintf(my_log_file, "%s SHUTDOWN\n", t_buffer);
	}else{
		fprintf(stdout, "%s SHUTDOWN\n", t_buffer);
	}
	
	exit(0);
}

float change_temperature(float a){
	float R = 1023.0/a - 1.0;
	R = R0 * R;
	float temperature = 1.0 / (log(R/R0)/B+1/298.15)-273.15;
	if (scale == 'F') {
		temperature = temperature * 1.8 + 32;
		return temperature;
	}else if (scale == 'C') {
		return temperature;
	}else {
		fprintf(stderr, "Error: Cannot get temperature \n");
		exit(1);
	}	
}

void* runner(void* temp){
	int my_id = *((int*)temp);
	if(my_id == 0){
		
		while(on){
			if(read(STDIN_FILENO, buffer, 2048) == -1){
				fprintf(stderr, "Error: Can't read from stdin \n");
				exit(1);
			}

			int i = 0;
			while(buffer[i] != '\0' && buffer[i] != '\004'){
				int temp_pos = i;
				while(buffer[i] != '\n'){
					i++;
				}
				int len = i - temp_pos;

				char temp_string[len + 1];
				memcpy(temp_string, &buffer[temp_pos], len);
				temp_string[len] = '\0';

				if(strcmp(temp_string, "OFF") == 0){
					turn_off();
				}else if(strcmp(temp_string, "STOP") == 0){
					running = 0;
				}else if(strcmp(temp_string, "START") == 0){
					running = 1;
				}else if(strstr(temp_string, "PERIOD=") && strstr(temp_string, "PERIOD=") == temp_string){
					period = atoi(temp_string + 7);
				}else if(strcmp(temp_string, "SCALE=F") == 0){
					scale='F';
				}else if(strcmp(temp_string, "SCALE=C") == 0){
					scale='C';
				}

				if(my_log||(strstr(temp_string, "LOG ") && strstr(temp_string, "LOG ") == temp_string)){
					fprintf(my_log_file, "%s\n", temp_string);
				}

				i++;
			}
			memset(buffer, 0, sizeof(buffer));
		}


	}else{
		float my_temperature = 0.0;
		
		time_t rawtime;
		struct tm *timeinfo;


		mraa_aio_context t_temp;
		mraa_gpio_context my_button;
		t_temp = mraa_aio_init(1);
		my_button = mraa_gpio_init(60);
		mraa_gpio_dir(my_button, MRAA_GPIO_IN);
		mraa_gpio_isr(my_button, MRAA_GPIO_EDGE_RISING, &turn_off, NULL);

		while(1){
			while(running){
				my_temperature = mraa_aio_read(t_temp);
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(t_buffer, 10, "%X", timeinfo);
				if(my_log){
					fprintf(my_log_file ,"%s %.1f\n", t_buffer, change_temperature(my_temperature));
					fprintf(stdout ,"%s %.1f\n", t_buffer, change_temperature(my_temperature));
				}else{
					fprintf(stdout ,"%s %.1f\n", t_buffer, change_temperature(my_temperature));
				}
				usleep(period * 1000000);
			}
		}

	}
	pthread_exit(NULL);
}

int main(int argc, char **argv){

	static struct option long_option[] = {
		{"period", required_argument, 0, 'p'},
		{"scale", required_argument, 0, 's'},
		{"log", required_argument, 0, 'l'}
	};

	int op;
	while(1){
		op = getopt_long(argc, argv, "p:s:l:", long_option, NULL);
		if(op == -1){
			break;
		}
		switch(op){
			case 'p':
				period_change = 1;
				period = atoi(optarg);
				break;
			case  's':
				scale_change = 1;
				if(optarg[0] == 'C' || optarg[0] == 'F'){
					scale = optarg[0];
				}else{
					fprintf(stderr, "Error: Unrecognized argument\n");
					exit(1);
				}
				break;
			case 'l':
				my_log = 1;
				my_temp_log = open(optarg, O_RDWR | O_CREAT | O_TRUNC, 0666);
				my_log_file = fdopen(my_temp_log, "w");
				break;
			default:
				fprintf(stderr, "Error: Unrecognized argument\n");
				exit(1);
		}
	}

	pthread_t my_threads[2];
	if(my_log == 1){
		//check if the changed from F to C??????????????????
		if(scale_change){
			fprintf(my_log_file, "SCALE=%c\n", scale);
		}
		if(period_change){
			fprintf(my_log_file, "PERIOD=%d\n", period);
		}
	}
	int i;

	for(i = 0; i < 2; i++){
		if(pthread_create(&my_threads[i], NULL, runner,(void *) &tid_arr[i])){
			fprintf(stderr, "Error: Can't create threads\n");
			exit(1);
		}
	}

	for(i = 0; i < 2; i++){
		if(pthread_join(my_threads[i], NULL)){
			fprintf(stderr, "Error: Can't join threads\n");
			exit(1);
		}
	}

	return 0;
}

