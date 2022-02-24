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

#include <sched.h> // pin thread
#include <pthread.h>
#include <sys/io.h> //tell
#include <errno.h>

#define DEFAULT_PORT 9091
#define DEFAULT_PAYLOAD_SIZE 16
#define BUF_SIZE 1024
#define BASE_DIR "/nvme/"

static int numa_core[2][96] = {{0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94},
                               {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95}};

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