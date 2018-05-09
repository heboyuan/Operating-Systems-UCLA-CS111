// NAME: Boyuan He
// EMAIL: boyuan_heboyuan@live.com
// ID: 004791432

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


void my_handler(){
	fprintf(stderr, "Error: --segfault --catch cause segmentation fault\n");
	exit(4);
}

int main(int argc, char *argv[]){
    
	bool in = false, ou = false, se = false, ca = false;
	int fd0, fd1;
	char* inp = NULL;
	char* out = NULL;

	static struct option my_options[] = {
			{"input",required_argument,0,'i'},
			{"output",required_argument,0,'o'},
			{"segfault",no_argument,0,'s'},
			{"catch",no_argument,0,'c'},
			{0,0,0,0}
	};

	int op = 0;
	while(1){
		op = getopt_long(argc, argv, "i:o:sc", my_options, NULL);
		if(op == -1)
			break;
		switch(op){
			case 'i':
				in = true;
				inp = optarg;
				break;
			case 'o':
				ou = true;
				out = optarg;
				break;
			case 's':
				se = true;
				break;
			case 'c':
				ca = true;
				break;
			default:
				fprintf(stderr, "Error: Unrecognized argument\nCorrect Usage: ./lab0 --input=input_filename --output=output_filename --segfault --catch");
				exit(1);
		}
	}

	if(in){
		fd0 = open(inp, O_RDONLY);
		if(fd0 < 0){
			fprintf(stderr, "Error: --input caused open %s error %s\n", out, strerror(errno));
			exit(2);
		}else{
			close(0);
			dup(fd0);
			close(fd0);
		}
	}

	if(ou){
		fd1 = creat(out, 0666);
		if(fd1 < 0){
			fprintf(stderr, "Error: --output caused open %s error %s\n", out, strerror(errno));
			exit(3);
		}else{
			close(1);
			dup(fd1);
			close(fd1);
		}
	}

	if(ca){
		signal(SIGSEGV, my_handler);
	}

	if(se){
		char *p = NULL;
		*p = 'S';
	}

	int error;
	char temp;
	while (1) {
		error = read(0, &temp, sizeof(char));
		if (error > 0)
			write(1, &temp, sizeof(char));
		else if (error == 0)
			exit(0);
		else{
			fprintf(stderr, "Error: %s\n", strerror(errno));
			exit(errno);
		}

	}
}
