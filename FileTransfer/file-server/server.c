#include "../common/common.h"
Para paramters;
struct sockaddr_in cli_addr;
socklen_t cli_len;
static int numa_core[2][96] = {{0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94},
                               {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95}};

/*
需要释放的资源
Thread_ctx

埋下的坑：
1. thread_task中的buffer可能太小了。
2. 有哪些资源没有释放
*/

/*
server端是一个后台程序。
1. client端线通过一个socket发送文件名，文件大小，线程数。

*/

void *thread_task(void *ctx)
{
    Thread_ctx *thread_ctx = (Thread_ctx *)ctx;
    char buffer[BUF_SIZE];
    int recv_count, bytes_write;
    pin_1thread_to_1core(thread_ctx->coreid);

    lseek(thread_ctx->fileid, thread_ctx->start, SEEK_SET);
    while ((recv_count = read(thread_ctx->sockid, buffer, thread_ctx->packetsize)) > 0)
    {
        bytes_write = write(thread_ctx->fileid, buffer, recv_count);
        // bytes_write = recv_count;
        thread_ctx->count += bytes_write;
    }
    close(thread_ctx->sockid);
    close(thread_ctx->fileid);
    return NULL;
}
void handle_quit()
{
    printf("程序退出\n");
    close(paramters.listen_fd);
    free(paramters.address);
    free(paramters.filepath);
    printf("关闭成功\n");
    exit(1);
}

// 传过来的是路径，我们只要最后的文件名。
void getfilename(char *filepath, char *filename)
{
    int i, pos=0,start=0;
    // 找到最后一个/，把后面的字符拷贝下来
    for(i=0;filepath[i]!='\0';++i)
    {
        if(filepath[i]=='/')
        {
            pos = i;
        }
    }

    // 如果pos不是0，他就是/,要跳过。如果没有/就不用跳过。
    if(pos!=0)
    {
        i = pos+1;
    }
    else
    {
        i = pos;
    }
    for(start=0;filepath[i]!='\0';++i,++start)
    {
        filename[start] = filepath[i];
    }
    filename[start] = '\0';

}
void handle_connection_at_server(Para *para, int sockfd)
{
    char msg[1024];
    Thread_ctx *thread_ctx;
    int i;
    char recv_filepath[256], filename[256];
    char tmp_path[512];
    size_t each_thread_size;
    pthread_t monitor_thread;
    Monitor_ctx monitor_ctx;
    struct timeval start, end;
    unsigned long timer;
    double avgspeed;




    read(sockfd, msg, 1024);
    // 接收到的是文件名:文件大小:线程数
    printf("msg is %s\n", msg);
    sscanf(msg, "%zu:%d:%s", &para->filesize, &para->threads, recv_filepath);
    getfilename(recv_filepath, filename);

    printf("file path is %s, filename is %s, size is %zu, threads is %d\n", recv_filepath, filename, para->filesize, para->threads);
    sprintf(tmp_path, "%s%s%s", BASE_DIR, filename, "recv" );
    // strcat(para->filepath, "recv");
    para->filepath = strdup(tmp_path);
    printf("para->filepath is %s\n", para->filepath);
    close(sockfd);

    thread_ctx = (Thread_ctx *)malloc(para->threads * sizeof(Thread_ctx));

    each_thread_size = para->filesize / para->threads;
    for (i = 0; i < para->threads; i++)
    {
        thread_ctx[i].count = 0;
        thread_ctx[i].sockid = accept(para->listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
        thread_ctx[i].coreid = numa_core[para->numa][i];
        thread_ctx[i].packetsize = para->size;
        thread_ctx[i].start = i * each_thread_size;
        thread_ctx[i].end = thread_ctx[i].start + each_thread_size;
        thread_ctx[i].fileid = open(para->filepath, O_WRONLY | O_CREAT, 0644);
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

    printf("\n%s get complete, avg speed is %.2lfMB/s, %.2lf Gbps\n", para->filepath, avgspeed, avgspeed*8/1024);

    free(thread_ctx);
}
int main(int argc, char *argv[])
{
    signal(SIGINT, handle_quit); // ctrl+C将产生此信号
    setvbuf(stdout, 0, _IONBF, 0);

    int sockfd;
    struct sockaddr_in address;
    cli_len = sizeof(cli_addr);

    // read command line arguments
    parse_args(&paramters, argc, argv);

    if ((paramters.listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(paramters.address);
    address.sin_port = htons(paramters.port);
    printf("bind to %s:%d\n", paramters.address, paramters.port);
    if (bind(paramters.listen_fd, (struct sockaddr *)&address, sizeof(address)))
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(paramters.listen_fd, SOMAXCONN) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("server listens on %s:%d\n", paramters.address, paramters.port);

    while (1)
    {
        sockfd = accept(paramters.listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
        handle_connection_at_server(&paramters, sockfd);
    }

    return 0;
}