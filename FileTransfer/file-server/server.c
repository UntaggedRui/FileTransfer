#include "../common/common.h"
Para paramters;
struct sockaddr_in cli_addr;
socklen_t cli_len;

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
    int recv_count;
    pin_1thread_to_1core(thread_ctx->coreid);

    lseek(thread_ctx->fileid, thread_ctx->start, SEEK_SET);
    while ((recv_count = read(thread_ctx->sockid, buffer, thread_ctx->packetsize)) > 0)
    {
        write(thread_ctx->fileid, buffer, recv_count);
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
void handle_connection_at_server(Para *para, int sockfd)
{
    char msg[1024];
    Thread_ctx *thread_ctx;
    int i;
    char filename[256];
    size_t each_thread_size;
    read(sockfd, msg, 1024);
    // 接收到的是文件名:文件大小:线程数
    printf("msg is %s\n", msg);
    sscanf(msg, "%zu:%d:%s", &para->filesize, &para->threads, filename);
    printf("file name is %s, size is %zu, threads is %d\n", filename, para->filesize, para->threads);
    para->filepath = strdup(filename);
    strcat(para->filepath, "recv");
    close(sockfd);

    thread_ctx = (Thread_ctx *)malloc(para->threads * sizeof(Thread_ctx));

    each_thread_size = para->filesize / para->threads;
    for (i = 0; i < para->threads; i++)
    {
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
    printf("the last i is %d\n", i);
    thread_ctx[i - 1].end = para->filesize;
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
    free(thread_ctx);
    printf("%s get complete\n", para->filepath);
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