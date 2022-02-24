#include "../common/common.h"

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
    // bzero((char *) &server_addr, sizeof(server_addr));
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
    char buffer[BUF_SIZE];
    int bytes_send;
    char *send_ptr;
    pin_1thread_to_1core(thread_ctx->coreid);
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

    close(thread_ctx->sockid);
    close(thread_ctx->fileid);

    return NULL;
}
void handle_connection_at_client(Para *para, int sockfd)
{
    
    int i;
    char msg[1024];
    size_t each_thread_size;
    
    para->filesize = getfilesize(para->filepath);
    sprintf(msg, "%zu:%d:%s", para->filesize, para->threads, para->filepath);
    printf("msg is %s\n", msg);
    write(sockfd, msg, 1024); //发送文件名
    close(sockfd);

    each_thread_size = para->filesize / para->threads;
    Thread_ctx *thread_ctx = (Thread_ctx *)malloc(para->threads * sizeof(Thread_ctx));
    for (i = 0; i < para->threads; ++i)
    {
        thread_ctx[i].sockid = connect_server(para);
        thread_ctx[i].coreid = numa_core[para->numa][i];
        thread_ctx[i].packetsize = para->size;
        thread_ctx[i].start = i * each_thread_size;
        thread_ctx[i].end = thread_ctx[i].start + each_thread_size;
        thread_ctx[i].fileid =  open(para->filepath, O_RDONLY);
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
    printf("%s send complete\n", para->filepath);
}
int main(int argc, char *argv[])
{
    Para paramters;
    // Thread_ctx *thread_ctx;

    // read command line arguments
    parse_args(&paramters, argc, argv);
    // thread_ctx = (Thread_ctx *)malloc(paramters.threads * sizeof(Thread_ctx));

    int socket_fd = connect_server(&paramters);
    handle_connection_at_client(&paramters, socket_fd);

    free(paramters.address);
    free(paramters.filepath);
    return 0;
}