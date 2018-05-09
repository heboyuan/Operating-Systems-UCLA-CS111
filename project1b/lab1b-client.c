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



int sockfd = -1;
int log_fd = -1;
int portno = 0;
int my_log = 0;
int port = 0;
int comp = 0;
struct termios my_attributes;
z_stream client_to_server;
z_stream server_to_client;

void restore_terminal_mode (void){
  close(log_fd);
  close(sockfd);
  tcsetattr (STDIN_FILENO, TCSANOW, &my_attributes);
  exit(0);
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
  if (tcsetattr (STDIN_FILENO, TCSANOW, &tattr) < 0){
    fprintf(stderr, "Error: cannot set attribute %s \n", strerror(errno));
    exit(1);
  }
}

void write_log(char* wri_buffer, int wri_len, int sending){
  if (sending){
    write(log_fd, "SENT ", 5);
  }else{
    write(log_fd, "RECEIVED ", 9);
  }
  char str_size[50];
  sprintf(str_size, "%d", wri_len);
  write(log_fd, str_size, strlen(str_size));
  write(log_fd, " bytes: ", 8);
  write(log_fd, wri_buffer, wri_len);
  write(log_fd, "\n", 1);
}

int main(int argc, char **argv){
    struct sockaddr_in serv_addr;
    struct hostent *server; 

  static struct option my_option[] = {
    {"port", required_argument, 0, 'p'},
    {"log", required_argument, 0, 'l'},
    {"compress", no_argument, 0, 'c'},
    {0,0,0,0}
  };

  int op;
  while(1){
    op = getopt_long(argc, argv, "p:l:c", my_option, NULL);
    if(op == -1)
      break;
    switch(op){
      case 'p':
        port = 1;
        portno = atoi(optarg);
        break;
      case 'l':
        my_log = 1;
        if( (log_fd = creat(optarg, 0666)) == -1){
          fprintf(stderr, "Error: Cannot create log. \n");
        }
        break;
      case 'c':
        comp = 1;
        client_to_server.zalloc = Z_NULL;
        client_to_server.zfree = Z_NULL;
        client_to_server.opaque = Z_NULL;
        if(deflateInit(&client_to_server, Z_DEFAULT_COMPRESSION) != Z_OK){
          fprintf(stderr, "ERROR: Cannot initialize server to client\n");
          exit(1);
        }
        server_to_client.zalloc = Z_NULL;
        server_to_client.zfree = Z_NULL;
        server_to_client.opaque = Z_NULL;
        if(inflateInit(&server_to_client) != Z_OK ){
          fprintf(stderr, "ERROR: Cannot initialize client to server\n");
          exit(1);
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

  set_terminal_mode();

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Error: Cannot open socket\n");
    exit(1);
  }

  server = gethostbyname("localhost");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *) &serv_addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
    fprintf(stderr,"Error: Cannot connect.\n");
  }

  int poll_err = 0;

  struct pollfd my_fd[2];
  my_fd[0].fd = STDIN_FILENO;
  my_fd[1].fd = sockfd;
  my_fd[0].events = POLLIN | POLLHUP | POLLERR; 
  my_fd[1].events = POLLIN | POLLHUP | POLLERR;

  while (1) {
    poll_err = poll(my_fd, 2, 0);
    if(poll_err < 0){
      fprintf(stderr, "Error: polling error %s\n", strerror(errno));
      exit(1);
    }else if(poll_err>0){

      if ((my_fd[0].revents & POLLIN)== POLLIN) {
        char buffer[1024];
        int size = read(0, buffer, 1024);
        if(size < 0){
          fprintf(stderr, "Error: cannot read %s \n", strerror(errno));
          exit(1);
        }

        int ind;
        for (ind = 0; ind < size ; ind ++){
          if(buffer[ind] == '\r' || buffer[ind] == '\n'){
            if (write(1, "\r\n", 2*sizeof(char)) < 0){
              fprintf(stderr, "Error: cannot write to stdout %s \n", strerror(errno));
              exit(1);
            }
          }else{
            if (write(1, &buffer[ind], sizeof(char)) < 0){
              fprintf(stderr, "Error: cannot write to stdout %s \n", strerror(errno));
              exit(1);
            }
          }
        }

        if(comp){
          char comp_buffer[2048];
          client_to_server.avail_in = size;
          client_to_server.next_in = (unsigned char *) buffer;
          client_to_server.avail_out = 2048;
          client_to_server.next_out = (unsigned char *) comp_buffer;

          do{
            deflate(&client_to_server, Z_SYNC_FLUSH);
          }while(client_to_server.avail_in > 0);

          unsigned int ind;
          for (ind = 0; ind < 2048 - client_to_server.avail_out ; ind ++){
            if(comp_buffer[ind] == '\r' || comp_buffer[ind] == '\n'){
              if (write(sockfd, "\n", sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }else{
              if (write(sockfd, &comp_buffer[ind], sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }
          }

          if(my_log)
            write_log(comp_buffer, 2048 - client_to_server.avail_out, 1);
        }else{
          int ind;
          for (ind = 0; ind < size ; ind ++){
            if(buffer[ind] == '\r' || buffer[ind] == '\n'){
              if (write(sockfd, "\n", sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }else{
              if (write(sockfd, &buffer[ind], sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }
          }

          if(my_log)
            write_log(buffer, size, 1);
        }
      }
      if ((my_fd[0].revents & POLLHUP)) {
        exit(1);
      }
      if ((my_fd[0].revents & POLLERR)) {
        exit(1);
      }

      if((my_fd[1].revents & POLLIN)== POLLIN){
        char buffer[2048];
        int size = read(my_fd[1].fd, buffer, 2048);
        if(size < 0){
          fprintf(stderr, "Error: cannot read %s \n", strerror(errno));
          exit(1);
        }else if(size == 0)
          break;

        if(comp){
          char comp_buffer[2048];
          server_to_client.avail_in = size;
          server_to_client.next_in = (unsigned char *) buffer;
          server_to_client.avail_out = 2048;
          server_to_client.next_out = (unsigned char *) comp_buffer;

          do{
            inflate(&server_to_client, Z_SYNC_FLUSH);
          } while(server_to_client.avail_in > 0);

          unsigned int ind;
          for (ind = 0; ind < 2048-server_to_client.avail_out ; ind ++){
            if(comp_buffer[ind] == '\r' || comp_buffer[ind] == '\n'){
              if (write(1, "\r\n", 2*sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }else{
              if (write(1, &comp_buffer[ind], sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }
          }
        }else{
          int ind;
          for (ind = 0; ind < size ; ind ++){
            if(buffer[ind] == '\r' || buffer[ind] == '\n'){
              if (write(1, "\r\n", 2*sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }else{
              if (write(1, &buffer[ind], sizeof(char)) < 0){
                fprintf(stderr, "Error: cannot write %s \n", strerror(errno));
                exit(1);
              }
            }
          }
          
        }
        if(my_log)
            write_log(buffer, size, 0);
      }
      if ((my_fd[1].revents & POLLHUP)) {
        exit(0);
      }
      if ((my_fd[1].revents & POLLERR)) {
        exit(0);
      }
    }
  }

  if(comp){
    inflateEnd(&server_to_client);
    deflateEnd(&client_to_server);
  }
  exit(0);
}