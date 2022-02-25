#include "../common/common.h"
static int numa_core[2][96] = {{0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94},
                               {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95}};

int connect_server(Para *para)
{
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(para->port);
    if ((server = gethostbyname(para->address)) == NULL)
    {
        perror("server is NULL");
        exit(EXIT_FAILURE);
    }
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}
void *thread_task(void *ctx)
{
    Thread_ctx *thread_ctx = (Thread_ctx *)ctx;
    int bytes_read = 0;
    size_t start = thread_ctx->start;
    char *buffer;
    int bytes_send;
    char *send_ptr;
    pin_1thread_to_1core(thread_ctx->coreid);
    buffer = (char *)malloc(sizeof(char)*thread_ctx->packetsize);
    lseek(thread_ctx->fileid, thread_ctx->start, SEEK_SET);
    while (start < thread_ctx->end)
    {
        bytes_read = read(thread_ctx->fileid, buffer, thread_ctx->packetsize);
        if (bytes_read > 0)
        {
            start += bytes_read;
            send_ptr = buffer;
            while ((bytes_send = write(thread_ctx->sockid, buffer, bytes_read)) != 0)
            {
                thread_ctx->count += bytes_send;
                // printf("bytes_send is %d\n", bytes_send);
                if ((bytes_send == -1) && (errno != EINTR))
                    break;
                if (bytes_send == bytes_read)
                    break;
                else if (bytes_send > 0)
                {
                    send_ptr += bytes_send;
                    bytes_read -= bytes_send;
                }
            }
            if (bytes_send == -1)
                break;
        }
    }
    free(buffer);
    close(thread_ctx->sockid);
    close(thread_ctx->fileid);

    return NULL;
}
void handle_connection_at_client(Para *para, int sockfd)
{

    int i;
    char msg[1024];
    size_t each_thread_size;
    pthread_t monitor_thread;
    Monitor_ctx monitor_ctx;
    struct timeval start, end;
    unsigned long timer;
    double avgspeed;

    para->filesize = getfilesize(para->filepath);
    sprintf(msg, "%zu:%u:%d:%s", para->filesize, para->size, para->threads, para->filepath);
    printf("msg is %s\n", msg);
    write(sockfd, msg, 1024); //发送文件名
    close(sockfd);

    each_thread_size = para->filesize / para->threads;
    Thread_ctx *thread_ctx = (Thread_ctx *)malloc(para->threads * sizeof(Thread_ctx));
    for (i = 0; i < para->threads; ++i)
    {
        thread_ctx[i].count = 0;
        thread_ctx[i].sockid = connect_server(para);
        thread_ctx[i].coreid = numa_core[para->numa][i];
        thread_ctx[i].packetsize = para->size;
        thread_ctx[i].start = i * each_thread_size;
        thread_ctx[i].end = thread_ctx[i].start + each_thread_size;
        thread_ctx[i].fileid = open(para->filepath, O_RDONLY);
        if (thread_ctx[i].fileid == 0)
        {
            printf("open file error\n");
            exit(EXIT_FAILURE);
        }
    }
    // the last thread
    thread_ctx[i - 1].end = para->filesize;

    // for bandwidth monitor
    monitor_ctx.thread_ctx = thread_ctx;
    monitor_ctx.threads = para->threads;
    monitor_ctx.run = 1;
    pthread_create(&monitor_thread, NULL, monitor_throughput, (void *)&monitor_ctx);

    gettimeofday(&start, NULL);
    for (i = 0; i < para->threads; ++i)
    {
        if (pthread_create(&thread_ctx[i].serv_thread, NULL, thread_task, (void *)&thread_ctx[i]) < 0)
        {
            perror("Error: create pthread");
        }
    }
    for (i = 0; i < para->threads; ++i)
    {
        pthread_join(thread_ctx[i].serv_thread, NULL);
    }
    
    gettimeofday(&end, NULL);

    monitor_ctx.run = 0;
    timer = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    avgspeed = 1.0*para->filesize/1024/1024/(timer/1000000);
    printf("\n%s send complete, avg speed is %.2lfMB/s, %.2lf Gbps\n", para->filepath, avgspeed, avgspeed*8/1024);
    free(thread_ctx);
}
int main(int argc, char *argv[])
{
    Para paramters;

    // read command line arguments
    parse_args(&paramters, argc, argv);

    int socket_fd = connect_server(&paramters);
    handle_connection_at_client(&paramters, socket_fd);

    free(paramters.address);
    free(paramters.filepath);
    return 0;
}