#include "common.h"
void usage(char *program)
{
    printf("Usage: \n");
    printf("%s\tready to connect\n", program);
    printf("Options:\n");
    printf(" -a <addr>      connect to server addr\n");
    printf(" -p <port>      connect to port number(default %d)\n", DEFAULT_PORT);
    printf(" -t <threads>   handle connection with multi-threads\n");
    printf(" -s <size>      send or recv size in each read or write(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -f <filepath>      the file path to save the data\n");
    printf(" -n      numa node ,0 or 1\n");
    printf(" -h             display the help information\n");
}
int parse_args(Para *para, int argc, char *argv[])
{
    para->size = 1024;
    para->port = 8877;
    para->threads = 1;
    para->numa = 0;
    while (1)
    {
        int c;

        static struct option long_options[] = {
            {.name = "port", .has_arg = 1, .val = 'p'},
            {.name = "address", .has_arg = 1, .val = 'a'},
            {.name = "thread", .has_arg = 1, .val = 't'},
            {.name = "size", .has_arg = 1, .val = 's'},
            {.name = "filepath", .has_arg = 1, .val = 'f'},
            {.name = "numa", .has_arg = 1, .val = 'n'},

            {0}};

        c = getopt_long(argc, argv, "p:a:t:s:f:n:l:eg:f:", long_options, NULL);
        if (c == -1)
            break;

        switch (c)
        {
        case 'p':
            para->port = strtol(optarg, NULL, 0);
            if (para->port < 0 || para->port > 65535)
            {
                usage(argv[0]);
                return 1;
            }
            break;

        case 'a':
            para->address = strdup(optarg);
            break;

        case 't':
            para->threads = strtol(optarg, NULL, 0);
            break;

        case 's':
            para->size = strtol(optarg, NULL, 0);
            break;

        case 'f':
            para->filepath = strdup(optarg);
            break;

        case 'n':
            para->numa = strtol(optarg, NULL, 0);
            break;

        default:
            usage(argv[0]);
            return 1;
        }
    }
    return 0;
}

__off_t getfilesize(char *filename)
{
    struct stat statbuf;
    stat(filename, &statbuf);
    return statbuf.st_size;
}
int setfilesize(int fd, __off_t size)
{
    return ftruncate(fd, size) == 0;
}
// each thread pins to one core
void pin_1thread_to_1core(int core_id)
{

    // cpu_set_t cpuset;
    // pthread_t thread;
    // thread = pthread_self();
    // CPU_ZERO(&cpuset);
    // int s;
    // CPU_SET(core_id, &cpuset);
    // s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    // if (s != 0)
    //     fprintf(stderr, "pthread_setaffinity_np:%d", s);
    // s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    // if (s != 0)
    //     fprintf(stderr, "pthread_getaffinity_np:%d", s);
}
