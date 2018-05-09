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
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h> 
#include <zlib.h>

int comp = 0;
int port = 0;
int to_child_pip[2];
int to_parent_pip[2];
pid_t child_pid = -1;
z_stream client_to_server;
z_stream server_to_client;

void handle(int sig){
	if(sig == SIGPIPE){
		exit(1);
	}
}

int main(int argc, char **argv){
  int sockfd, newsockfd, portno;
  unsigned int clilen;
  struct sockaddr_in serv_addr, cli_addr;

  static struct option my_options[] = {
		{"port", required_argument, NULL, 'p'},
    {"compress", no_argument, NULL, 'c'},
		{0,0,0,0}
	};

  int op = 0;
  while(1){
		op = getopt_long(argc, argv, "p:cd", my_options, NULL);
		if(op == -1)
			break;
		switch(op){
			case 'p':
				portno = atoi(optarg);
        port = 1;
        break;
      case 'c':
        comp = 1;
        server_to_client.zalloc = Z_NULL;
        server_to_client.zfree = Z_NULL;
        server_to_client.opaque = Z_NULL;
        if (deflateInit(&server_to_client, Z_DEFAULT_COMPRESSION) != Z_OK) {
          fprintf(stderr,"Error: Cannot initialize server to client\n");
          exit(1);
        }
        client_to_server.zalloc = Z_NULL;
        client_to_server.zfree = Z_NULL;
        client_to_server.opaque = Z_NULL;
        if (inflateInit(&client_to_server) != Z_OK) {
          fprintf(stderr, "Error: Cannot initialize client to server\n");
        }
        break;
			default:
				fprintf(stderr, "Error: Unrecognized argument\n");
				exit(1);
		}
	}
  if(port == 0){
    fprintf(stderr,"Error: No port number supplied\n");
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Error: Cannot open socket\n");
    exit(1);
  }

  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) <0 ){
    fprintf(stderr, "Error: Cannot bind server\n");
    exit(1);
  }
  
  //????????????????????????????????????????????????????????????????????????8888888888
  listen(sockfd, 8);
  //???????????????????????????????????????????????????????????????????????8888888888
  clilen = sizeof(cli_addr);
  
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0) {
      fprintf(stderr, "Error: pipe() error");
      exit(1);
  }


  if (pipe(to_child_pip) == -1){
      fprintf(stderr, "Error: pipe() error %s\n", strerror(errno));
      exit(1);
  }
  if (pipe(to_parent_pip) == -1){
      fprintf(stderr, "Error: pipe() error %s\n", strerror(errno));
      exit(1);
  }
  signal(SIGPIPE, handle);



  child_pid = fork();

  if(child_pid > 0){
    if (close(to_child_pip[0]) == -1){
        fprintf(stderr, "Error: cannot close file descriptor %s\n", strerror(errno));
        exit(1);
    }
    if (close(to_parent_pip[1]) == -1){
        fprintf(stderr, "Error: cannot close file descriptor %s\n", strerror(errno));
        exit(1);
    }

    int poll_err = 0;
    int status;

    struct pollfd my_fd[2];
    my_fd[0].fd = newsockfd;
    my_fd[1].fd = to_parent_pip[0];
    my_fd[0].events = POLLIN | POLLHUP | POLLERR;
    my_fd[1].events = POLLIN | POLLHUP | POLLERR;


    while(1){
        if (waitpid(child_pid, &status, WNOHANG) != 0){
        close(sockfd);
        close(newsockfd);
        close(to_parent_pip[0]);
        close(to_child_pip[1]);
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(status), WEXITSTATUS(status));
        exit(0);
        }
        
        poll_err = poll(my_fd, 2, 0);
        
        if(poll_err < 0){
          fprintf(stderr, "Error: polling error %s\n", strerror(errno));
          exit(1);
        }else if(poll_err > 0){

          if ((my_fd[0].revents & POLLIN)== POLLIN) {
            char buffer[2048];
            int size = read(newsockfd, buffer, 2048);
            if(size < 0){
              fprintf(stderr, "Error: cannot read %s \n", strerror(errno));
              exit(1);
            }
            
            if(comp){
              char comp_buffer[2048];
              client_to_server.avail_in = size;
              client_to_server.next_in = (unsigned char *) buffer;
              client_to_server.avail_out = 2048;
              client_to_server.next_out = (unsigned char *) comp_buffer;
              do{
                inflate(&client_to_server, Z_SYNC_FLUSH);
              }while(client_to_server.avail_in > 0);
              
              unsigned int ind;
              for(ind = 0; ind < 2048-client_to_server.avail_out; ind++){
                switch (comp_buffer[ind]){
                  case 0x04:
                    close(to_child_pip[1]);
                    break;
                  case 0x03:
                    kill(child_pid, SIGINT);
                    break;
                  case '\r':
                  case '\n':
                    {
                      if (write(to_child_pip[1], "\n", sizeof(char)) < 0){
                        fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                        exit(1);
                      }
                    }
                    break;
                  default:
                    if (write(to_child_pip[1], &comp_buffer[ind], sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                }
              }
            }else{
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
                    if (write(to_child_pip[1], "\n", sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                  }
                  break;
                  default:
                    if (write(to_child_pip[1], &buffer[ind], sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                }
              }
            }
          }
          if ((my_fd[0].revents & POLLHUP)) {
            exit(1);
          }
          if ((my_fd[0].revents & POLLERR)) {
            exit(1);
          }


          if ((my_fd[1].revents & POLLIN)== POLLIN) {
            char buffer[2048];
            int size = read(my_fd[1].fd, buffer, 2048);
            if(size < 0){
              fprintf(stderr, "Error: cannot read %s \n", strerror(errno));
              exit(1);
            }
            
            if(comp){
              char comp_buffer[2048];
              server_to_client.avail_in = size;
              server_to_client.next_in = (unsigned char *) buffer;
              server_to_client.avail_out = 2048;
              server_to_client.next_out = (unsigned char *) comp_buffer;
              do{
                deflate(&server_to_client, Z_SYNC_FLUSH);
              }while(server_to_client.avail_in > 0);
              unsigned int ind;
              for(ind = 0; ind < 2048-server_to_client.avail_out; ind++){
                switch (comp_buffer[ind]){
                  case 0x04:
                    close(to_child_pip[1]);
                    break;
                  case 0x03:
                    kill(child_pid, SIGINT);
                    break;
                  case '\r':
                  case '\n':
                  {
                    if (write(newsockfd, "\n", sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                  }
                  break;
                  default:
                    if (write(newsockfd, &comp_buffer[ind], sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                }
              }
            }else{
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
                    if (write(newsockfd, "\n", sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                  }
                  break;
                  default:
                    if (write(newsockfd, &buffer[ind], sizeof(char)) < 0){
                      fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                      exit(1);
                    }
                }
              }
            }
          }
          if ((my_fd[1].revents & POLLHUP)) {
            break;
          }
          if ((my_fd[1].revents & POLLERR)) {
            break;
          }
        }
    }
    if (close(sockfd) == -1){
        fprintf(stderr, "Error: cannot close file descriptor %s\n", strerror(errno));
        exit(1);
    }
    if (close(newsockfd) == -1){
        fprintf(stderr, "Error: cannot close file descriptor %s\n", strerror(errno));
        exit(1);
    }
    if (close(to_parent_pip[0]) == -1){
        fprintf(stderr, "Error: cannot close file descriptor %s\n", strerror(errno));
        exit(1);
    }
    if (close(to_child_pip[1]) == -1){
        fprintf(stderr, "Error: cannot close file descriptor %s\n", strerror(errno));
        exit(1);
    }

    waitpid(child_pid, &status, 0);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(status), WEXITSTATUS(status));
  
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

  if(comp){
    inflateEnd(&client_to_server);
    deflateEnd(&server_to_client);
  }
  exit(0);
}
