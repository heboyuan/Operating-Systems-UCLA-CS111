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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>


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

char* id;
char* my_host;
int my_port;
int my_sockfd;
SSL_CTX* ctx;
SSL *ssl;


void turn_off(){
	running = 0;
	time_t rawtime;
	struct tm *timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	
	on = 0;
	if(my_log){
		fprintf(my_log_file, "OFF\n");
	}
	strftime(t_buffer, 10, "%X", timeinfo);
	if(my_log){
		fprintf(my_log_file, "%s SHUTDOWN\n", t_buffer);
	}else{
		char write_buffer[50];
		memset(&write_buffer, 0, 50);
		sprintf(write_buffer, "%s SHUTDOWN\n", t_buffer);
		SSL_write(ssl, write_buffer, strlen(write_buffer));
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
			if(SSL_read(ssl, buffer, 2048) == -1){
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

				char write_buffer[50];
				memset(&write_buffer, 0, 50);
				sprintf(write_buffer, "%s %.1f\n", t_buffer, change_temperature(my_temperature));

				if(my_log){
					fprintf(my_log_file ,"%s %.1f\n", t_buffer, change_temperature(my_temperature));
					SSL_write(ssl, write_buffer, strlen(write_buffer));
				}else{
					SSL_write(ssl, write_buffer, strlen(write_buffer));
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
		{"log", required_argument, 0, 'l'},
		{"id", required_argument, 0, 'i'},
		{"host", required_argument, 0, 'h'}
	};

	if(argc < 5){
		fprintf(stderr, "Error: invalid arguement\n");
		exit(1);
	}

	int op;
	while(1){
		op = getopt_long(argc, argv, "p:s:l:i:h:", long_option, NULL);
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
			case 'i':
				if(strlen(optarg) != 9){
					fprintf(stderr, "Error: Unrecognized argument\n");
					exit(1);
				}
				id = optarg;
				break;
			case 'h':
				my_host = malloc(strlen(optarg)+1);
				my_host = optarg;
				break;
			default:
				fprintf(stderr, "Error: Unrecognized argument\n");
				exit(1);
		}
	}

	my_port = atoi(argv[argc - 1]);

	//need to check if all the flags are set????????????????????????????????

	if ((my_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error: Cannot open socket\n");
		exit(2);
	}

	struct hostent* my_server = gethostbyname(my_host);
	if(my_server == NULL){
		fprintf(stderr, "Error: Cannot get host\n");
		exit(2);
	}

	struct sockaddr_in my_addr;
	my_addr.sin_port = htons(my_port);
	my_addr.sin_family = AF_INET;
	memcpy(&my_addr.sin_addr.s_addr, my_server->h_addr, my_server->h_length);

	if(connect(my_sockfd, (struct sockaddr*) &my_addr, sizeof(my_addr)) < 0){
		fprintf(stderr, "Error: Cannot connect to server\n");
		exit(2);
	}

	if(SSL_library_init() < 0){
		fprintf(stderr, "Error: Cannot initialize OpenSSL\n");
		exit(2);
	}

	OpenSSL_add_all_algorithms();

	if((ctx = SSL_CTX_new(SSLv23_client_method())) == NULL){
		fprintf(stderr, "Error: Cannot create context\n");
		exit(2);
	}

	ssl = SSL_new(ctx);

	if(SSL_set_fd(ssl, my_sockfd) == 0){
		fprintf(stderr, "Error: Cannot sockfd\n");
		exit(2);
	}

	if(SSL_connect(ssl) != 1) {
		fprintf(stderr, "Error: Cannot connect\n");
		exit(2);
	}

	char id_buffer[20];
	memset(&id_buffer, 0, 20);
	sprintf(id_buffer, "ID=%s", id);

	if (SSL_write(ssl, id_buffer, sizeof(id_buffer)) < 0 ) {
		fprintf(stderr, "Error: Cannot write\n");
		exit(2);
	}
	
	fprintf(my_log_file, "ID=%s\n", id);

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

	free(my_host);
	return 0;
}