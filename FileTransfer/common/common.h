#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> // 用于参数解析
#include <sys/socket.h>
#include <netdb.h>     // struct sockaddr_in address, cli_addr;
#include <sys/stat.h>  // getfilesize
#include <fcntl.h>     // O_WRONLY|O_CREAT for open
#include <unistd.h>    // for close
#include <signal.h>    // signal(SIGINT,handle);	//ctrl+C将产生此信号
#include <string.h>    // bcopy
#include <arpa/inet.h> // address.sin_addr.s_addr = inet_addr(paramters.address);
#define __USE_GNU  
#include <sched.h> // pin thread
#include <pthread.h>
#include <sys/io.h> //tell
#include <errno.h>
#include <sys/time.h>

#define DEFAULT_PORT 9091
#define DEFAULT_PAYLOAD_SIZE 16
#define BUF_SIZE 1024
#define BASE_DIR "/nvme/"
// #define BASE_DIR "/mnt/tmpfs/"


typedef struct para
{
    __uint32_t port;
    char *address;
    __uint32_t threads;
    __uint32_t size;
    char *filepath;
    __uint32_t numa;
    size_t filesize;
    int listen_fd;
} Para;

typedef struct global_ctx
{

} global_ctx;

typedef struct Thread_ctx
{
    int coreid;
    int fileid;
    int sockid;
    pthread_t serv_thread;
    int packetsize;
    size_t start;
    size_t end;
    size_t count; // for monitor

} Thread_ctx;

typedef struct Monitor_ctx
{
    Thread_ctx *thread_ctx;
    int threads;
    int run;
}Monitor_ctx;

int parse_args(Para *para, int argc, char *argv[]);
__off_t getfilesize(char *filename);
int setfilesize(int fd, __off_t size);
void pin_1thread_to_1core(int core_id);
void *monitor_throughput(void *Monitor_ctx);