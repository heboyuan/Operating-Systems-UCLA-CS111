#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/wait.h>
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include<signal.h>

/*reference
https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
https://stackoverflow.com/questions/27541910/how-to-use-execvp
*/
struct termios my_attributes;
pid_t child_pid = -1;
int shell = 0;
int to_child_pip[2];
int to_parent_pip[2];

void restore_terminal_mode (void){
	tcsetattr (STDIN_FILENO, TCSANOW, &my_attributes);
	if(shell){
		int ex;
		if (waitpid(child_pid, &ex, 0) < 0){
			fprintf(stderr, "Error: cannot wait for child %s \n", strerror(errno));
			exit(1);
		}
		if(WIFEXITED(ex)){
			fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(ex), WEXITSTATUS(ex));
			exit(0);
		}
	}
}

void set_terminal_mode(){
	struct termios tattr;
	
	if (!isatty (STDIN_FILENO)){
		fprintf (stderr, "Error: not a terminal\n");
		exit (1);
	}

	if (tcgetattr (STDIN_FILENO, &my_attributes) < 0){
		fprintf(stderr, "Error: cannot get attribute %s \n", strerror(errno));
		exit(1);
	}

	atexit (restore_terminal_mode);

	
	if (tcgetattr (STDIN_FILENO, &tattr) < 0){
		fprintf(stderr, "Error: cannot get attribute %s \n", strerror(errno));
		exit(1);
	}
	tattr.c_iflag = ISTRIP;
	tattr.c_oflag = 0;
	tattr.c_lflag = 0;
	tattr.c_cc[VMIN] = 1;
  	tattr.c_cc[VTIME] = 0;
	if (tcsetattr (STDIN_FILENO, TCSANOW, &tattr) < 0){
		fprintf(stderr, "Error: cannot set attribute %s \n", strerror(errno));
		exit(1);
	}
}

void read_write(){
	char rw_buffer[256];
	int size;
	while ((size = read(0, &rw_buffer, 256))){
		if(size < 0){
			fprintf(stderr, "Error: cannot read stdin %s \n", strerror(errno));
			exit(1);
		}
		int ind;		
		for (ind = 0; ind < size ; ind ++){
			if (rw_buffer[ind] == '\004'){
				exit(0);
			}else if(rw_buffer[ind] == '\r' || rw_buffer[ind] == '\n'){
				if (write(1, "\r\n", 2*sizeof(char)) < 0){
					fprintf(stderr, "Error: cannot write to stdout %s \n", strerror(errno));
					exit(1);
				}
			}else{
				if (write(1, &rw_buffer[ind], sizeof(char)) < 0){
					fprintf(stderr, "Error: cannot write to stdout %s \n", strerror(errno));
					exit(1);
				}
			}
		}
	}
}

void handle(int sig){
	if(sig == SIGPIPE){
		exit(1);
	}
}

int main(int argc, char **argv){

	set_terminal_mode();

	static struct option my_options[] = {
		{"shell", no_argument, 0, 's'},
		{0,0,0,0}
	};
	
	int op = 0;
	while(1){
		op = getopt_long(argc, argv, "s", my_options, NULL);
		if(op == -1)
			break;
		switch(op){
			case 's':
				signal(SIGPIPE, handle);
				shell = 1;
				break;
			default:
				fprintf(stderr, "Error: Unrecognized argument\n");
				exit(1);
		}
	}


	if(shell){
		if (pipe(to_child_pip) == -1){
			fprintf(stderr, "Error: pipe() error %s\n", strerror(errno));
			exit(1);
		}
		if (pipe(to_parent_pip) == -1){
			fprintf(stderr, "Error: pipe() error %s\n", strerror(errno));
			exit(1);
		}

		child_pid = fork();

		if(child_pid > 0){
			if (close(to_child_pip[0]) == -1){
			fprintf(stderr, "Error: cannot close file descriptor to child %s\n", strerror(errno));
			exit(1);
			}
			if (close(to_parent_pip[1]) == -1){
			fprintf(stderr, "Error: cannot close file descriptor to parent %s\n", strerror(errno));
			exit(1);
			}

			char buffer[256];
			int poll_err = 0;

			struct pollfd my_fd[2];
			my_fd[0].fd = 0;
			my_fd[1].fd = to_parent_pip[0];
			my_fd[0].events = POLLIN | POLLHUP | POLLERR;
			my_fd[1].events = POLLIN | POLLHUP | POLLERR;


			while(1){
				poll_err = poll(my_fd, 2, -1);
				if(poll_err < 0){
					fprintf(stderr, "Error: polling error %s\n", strerror(errno));
					exit(1);
				}

				if ((my_fd[0].revents & POLLIN)== POLLIN) {
					
					int size = read(0, buffer, 256);
					if(size < 0){
						fprintf(stderr, "Error: cannot read stdin %s \n", strerror(errno));
						exit(1);
					}

					int ind;
					for(ind = 0; ind < size; ind++){
						switch (buffer[ind]){
							case 0x04:
								close(to_child_pip[1]);
								break;
							case 0x03:
								kill(child_pid, SIGINT);
								break;
							case '\r':
							case '\n':
							{
								if (write(1, "\r\n", 2*sizeof(char)) < 0){
									fprintf(stderr, "Error: cannot write to stdout %s \n", strerror(errno));
									exit(1);
								}
								if (write(to_child_pip[1], "\n", sizeof(char)) < 0){
									fprintf(stderr, "Error: cannot write to child %s \n", strerror(errno));
									exit(1);
								}
								break;
							}
							default:
								if (write(1, &buffer[ind], 1) < 0){
									fprintf(stderr, "Error: cannot write to stdout %s \n", strerror(errno));
									exit(1);
								}
								if (write(to_child_pip[1], &buffer[ind], sizeof(char)) < 0){
									fprintf(stderr, "Error: cannot write to child %s \n", strerror(errno));
									exit(1);
								}
						}

					}
				}

				if((my_fd[1].revents & POLLIN)== POLLIN){

					int size = read(to_parent_pip[0], buffer, 256);
					if(size < 0){
						fprintf(stderr, "Error: cannot read from child %s \n", strerror(errno));
						exit(1);
					}
					int ind;
					for(ind = 0; ind < size; ind++){
						if(buffer[ind] == '\n'){
							if(write(1, "\r\n", 2*sizeof(char))==-1){
								fprintf(stderr, "Error: can't write to stdout %s\n", strerror(errno));
								exit(1);
							}
						}else{
							if(write(1, &buffer[ind], sizeof(char))==-1){
								fprintf(stderr, "Error: can't write to stdout %s\n", strerror(errno));
								exit(1);
							}
						}
					}
				}

				if ((my_fd[1].revents & POLLHUP)) {
					exit(1);
				}
				if ((my_fd[1].revents &  POLLERR)) {
					exit(1);
				}
			}
		}else if(child_pid == 0){
			if (close(to_child_pip[1]) == -1){
				fprintf(stderr, "Error: cannot close file descriptor to child %s\n", strerror(errno));
				exit(1);
			}
			if (close(to_parent_pip[0]) == -1){
				fprintf(stderr, "Error: cannot close file descriptor to parent %s\n", strerror(errno));
				exit(1);
			}
			if (dup2(to_child_pip[0], STDIN_FILENO) == -1){
				fprintf(stderr, "Error: cannot dup %s\n", strerror(errno));
				exit(1);
			}
			if (dup2(to_parent_pip[1], STDOUT_FILENO) == -1){
				fprintf(stderr, "Error: cannot dup %s\n", strerror(errno));
				exit(1);
			}
			if (dup2(to_parent_pip[1], STDERR_FILENO) == -1){
				fprintf(stderr, "Error: cannot dup %s\n", strerror(errno));
				exit(1);
			}
			if (close(to_child_pip[0]) == -1){
				fprintf(stderr, "Error: cannot close file descriptor to child %s\n", strerror(errno));
				exit(1);
			}
			if (close(to_parent_pip[1]) == -1){
				fprintf(stderr, "Error: cannot close file descriptor to parent %s\n", strerror(errno));
				exit(1);
			}


			char *file = "/bin/bash";
			char *arguments[2];
			arguments[0] = "/bin/bash";
			arguments[1] = NULL;

			if(execvp(file, arguments) == -1){
				fprintf(stderr, "Error: cannot execute shell %s\n", strerror(errno));
				exit(1);
			}

		}else{
			fprintf(stderr, "Error: cannot fork %s\n", strerror(errno));
			exit(1);
		}
	}

	read_write();
	exit(0);
}