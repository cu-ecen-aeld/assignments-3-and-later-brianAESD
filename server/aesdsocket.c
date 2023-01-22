// aesdsocket.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <sys/queue.h>

#define USE_THREAD

#define PORT_TARGET "9000"
#if 1
#define DATA_FILE "/var/tmp/aesdsocketdata"
#else
#define DATA_FILE "mytest.txt"
#endif
#define MAX_NUM_CONNECTION 10

const uint8_t printf_enabled = 0;

struct addrinfo* addrinfo_res;
struct addrinfo hints;

// Thread
typedef struct {                    /* Used as argument to thread_start() */
    pthread_t   thread_id;          /* ID returned by pthread_create() */
    int         thread_num;         /* Application-defined thread # */
    bool        connection_done;
    int         conn_socket;
} thread_info_t;

pthread_mutex_t mutexLock;
pthread_t timestamp_threadId;

// SLIST.
typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    thread_info_t* threadInfo;
    SLIST_ENTRY(slist_data_s) entries;
};

bool caught_sig = false;
int sockfd = -1;
FILE* fp = 0;
unsigned int sendSizeTotal = 0;
int bwanTest1 = 0;
char* conn_ip;
bool callTimeStampthread = false;

SLIST_HEAD(slisthead, slist_data_s) head;

// void connection_closed_procedure(void)
// {
//     //remove(DATA_FILE);
// }

static void cleaup_slist(void)
{
    slist_data_t *datap;
    while (!SLIST_EMPTY(&head))
    {
        datap = SLIST_FIRST(&head);
        if (datap->threadInfo && datap->threadInfo->conn_socket > 0)
        {
            close(datap->threadInfo->conn_socket);
            datap->threadInfo->conn_socket = 0;
            pthread_join(datap->threadInfo->thread_id, NULL);
            free(datap->threadInfo);
        }
        SLIST_REMOVE_HEAD(&head, entries);
        free(datap);
    }
}

void exit_procedure(void)
{
    //if (printf_enabled) printf("Running exit_procedure.\n");

    if (addrinfo_res)
    {
        freeaddrinfo(addrinfo_res);
        addrinfo_res = 0;
    }

    remove(DATA_FILE);

    // if (sockfd != -1)
    // {
    //     close(sockfd);
    // }

    if (fp)
    {
        fclose(fp);
    }

    cleaup_slist();
    pthread_cancel(timestamp_threadId);
    pthread_join(timestamp_threadId, NULL);

    exit(-1);
}

static void signal_handler_child(int signal_number)
{
    if (signal_number == SIGCHLD)
    {
        int saved_errno = errno;
        if (printf_enabled) printf("In signal_handler_child\n");
        syslog(LOG_ERR, "In signal_handler_child..........................");
        while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
        errno = saved_errno;
    }
}

static void signal_handler(int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        caught_sig = true;
        if (printf_enabled) printf("Caught signal, exiting..........................\n");
        syslog(LOG_ERR, "Caught signal, exiting..........................");

        if (addrinfo_res)
        {
            freeaddrinfo(addrinfo_res);
            addrinfo_res = 0;
        }

        remove(DATA_FILE);

        if (fp)
        {
            fclose(fp);
        }

        cleaup_slist();
        pthread_cancel(timestamp_threadId);
        pthread_join(timestamp_threadId, NULL);

        exit(0);
    }
}

static void timestamp_thread(void* thread)
{
    while (1)
    {
        // #define BILLION 1000000000L
        // uint64_t diff;
        // struct timespec start, end;
        /* measure monotonic time */
        // clock_gettime(CLOCK_MONOTONIC, &start);	/* mark start time */
        sleep(10);      // in seconds
        // clock_gettime(CLOCK_MONOTONIC, &end);	/* mark the end time */
        // diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
        // printf("elapsed time = %llu nanoseconds\n", (long long unsigned int) diff);

        time_t rawtime;
        struct tm *info;
        time( &rawtime );
        info = localtime( &rawtime );
        // printf("Current local time and date: %s", asctime(info));
        char buffer1[128] = " ";
        char buffer2[128] = "timestamp: ";
        strftime(buffer1, sizeof(buffer1), "%a, %d %b %Y %T %z", info);
        buffer1[strlen(buffer1)] = '\n';
        strcat( buffer2, buffer1 );
        //printf("timestamp: %s", buffer);
        printf("%s", buffer2);

        pthread_mutex_lock(&mutexLock);
        // Seek to the end of the file
        fseek(fp, 0, SEEK_END);
        fwrite(buffer2, sizeof(char), strlen(buffer2), fp);
        pthread_mutex_unlock(&mutexLock);
    }
}

static void connection_thread(void* thread)
{
    thread_info_t* threadInfo = thread;

    if (printf_enabled) printf("Start connection_thread %ld\n", threadInfo->thread_id);
    syslog(LOG_DEBUG, "Start connection_thread %ld\n", threadInfo->thread_id);


    while (threadInfo->connection_done == 0)
    {
        int receiveResult = 0;
        const unsigned int receiveDataSize = 130;
        //char receiveData[80000];
        char* receiveData = malloc(receiveDataSize+1);
        memset(receiveData, 0, receiveDataSize+1);

        // receive
        pthread_mutex_lock(&mutexLock);
        bool newlineFound = false;
        receiveResult = recv(threadInfo->conn_socket, receiveData, receiveDataSize, 0);
        if (receiveResult == -1)
        {
            // unsuccessful
            if (printf_enabled) printf("Receive is unsuccessful.\n");
            //connection_closed_procedure();
        }
        else if (receiveResult == 0)
        {
            // connection is closed
            syslog(LOG_DEBUG, "Closed connection from %s\n", conn_ip);

            // Receive is done.
            if (printf_enabled) printf("Receive is done.\n");
            threadInfo->connection_done = true;
        }
        else
        {
            // Received data

            //if (printf_enabled) printf("Received (%d): %s", receiveResult, receiveData);
            bwanTest1++;
            // Seek to the end of the file
            fseek(fp, 0, SEEK_END);
            fwrite(receiveData, sizeof(char), receiveResult, fp);
        }

        if ( strchr(receiveData, '\n') )
        {
            newlineFound = true;
        }

        free(receiveData);
        fflush(fp);
        pthread_mutex_unlock(&mutexLock);

        if (newlineFound)
        {


            // timestamp thread
            if (callTimeStampthread == false)
            {
                callTimeStampthread = true;
                int error = pthread_create(&timestamp_threadId,
                            NULL,//&attr,
                            (void*) &timestamp_thread, 
                            NULL);
                if (error != 0)
                {
                    // error handle_error_en(s, "pthread_create");
                    // TODO
                    exit(-1);
                }    
            }

            
            threadInfo->connection_done = true;

            // send

            FILE * fp_send;
            uint8_t byte;
            bool isNewlineThere = false;
            printf("send start >>>>>>-------===================\n");
            fp_send = fopen(DATA_FILE, "r");
            while (!feof(fp_send))
            {
                byte = fgetc(fp_send);
                if (byte == '\n') isNewlineThere = true;
                send(threadInfo->conn_socket, &byte, 1, 0);
                printf("%c",byte);
            }
            fclose(fp_send);
            if (isNewlineThere) printf("Send isNewlineThere = true <<<<<<<<< ............\n");
            printf("send end ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

        }
    }

    if (printf_enabled) printf("Closing connection_thread %ld\n", threadInfo->thread_id);
    syslog(LOG_DEBUG, "Closing connection_thread %ld\n", threadInfo->thread_id);
    close(threadInfo->conn_socket);
    threadInfo->connection_done = true;
    threadInfo->conn_socket = 0;
    //remove(DATA_FILE);

    if (printf_enabled) printf("Done with connection_thread %ld\n", threadInfo->thread_id);
    syslog(LOG_DEBUG, "Done with connection_thread %ld\n", threadInfo->thread_id);
}

int main(int argc, char **argv)
{
    syslog(LOG_DEBUG, "--------------START----------------------");
    syslog(LOG_DEBUG, "--------------START----------------------");
    pthread_mutex_init(&mutexLock, NULL);
    SLIST_INIT(&head);

    if (printf_enabled) printf("Starting aesdsocket\n");
    openlog("aesdsocket", 0, LOG_USER);

    // Signal - for SIGTERM and SIGINT signals

    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGINT, &new_action, NULL);

    // Socket

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(NULL, PORT_TARGET, &hints , &addrinfo_res);  // remember to call freeaddrinfo later
    if (status != 0)
    {
        syslog(LOG_ERR, "Failed getAddrStatus");
        exit_procedure();
    }

    // socket - create endpoint for communication
    sockfd = socket(hints.ai_family, hints.ai_socktype, 0);
    if (sockfd == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        exit_procedure();
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        syslog(LOG_ERR, "Failed SO_REUSEADDR");
        exit_procedure();
    }

    // bind
    status = bind(sockfd, addrinfo_res->ai_addr, addrinfo_res->ai_addrlen);
    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to bind");
        exit_procedure();
    }

    if (addrinfo_res)
    {
        freeaddrinfo(addrinfo_res);
        addrinfo_res = 0;
    }

    // -D: run the aesdsocket application as a daemon

    if (argc >= 2 && strcmp(argv[1], "-d") == 0)
    {
        // fork
        pid_t childPid = fork();
        if (childPid == -1)
        {
            if (printf_enabled) printf("Failed to fork\n");
            exit(-1);
        }
        else if (childPid)
        {
            if (printf_enabled) printf("Fork %d\n", childPid);
            exit(0);
        }
    }

    // // timestamp thread
    // int error = pthread_create(&timestamp_threadId,
    //             NULL,//&attr,
    //             (void*) &timestamp_thread, 
    //             NULL);
    // if (error != 0)
    // {
    //     // error handle_error_en(s, "pthread_create");
    //     // TODO
    //     exit(-1);
    // }    

    syslog(LOG_DEBUG, "--------------Listen--------------------");
    syslog(LOG_DEBUG, "--------------Listen--------------------");

    // Signal - for SIGTERM and SIGINT signals

    //struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGINT, &new_action, NULL);
    new_action.sa_handler = signal_handler_child;
    sigaction(SIGCHLD, &new_action, NULL);

    // listen
    status = listen(sockfd, MAX_NUM_CONNECTION);
    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to listen");
        exit_procedure();
    }

    // open file
    fp = fopen(DATA_FILE,"a+");
    if (fp == NULL)
    {
        if (printf_enabled) printf("Failed: Unable to open file.\n");
        syslog(LOG_ERR, "Failed: Unable to open file.\n");
        exit(-1);
    }
    if (printf_enabled) printf("Opened: %s\n", DATA_FILE);

    // int returnState = 0;
    int new_conn_socket = 0;

    slist_data_t *datap = NULL;
    
    sendSizeTotal = 0;

    while(1)
    {

        // accept - accept a connection on a socket
        struct sockaddr_in conn_socketaddr;
        int addrlen = sizeof(conn_socketaddr);
        new_conn_socket = accept(sockfd, (struct sockaddr*) &conn_socketaddr, (socklen_t*) &addrlen);
        if (new_conn_socket == -1)
        {
            syslog(LOG_ERR, "Failed to accept");
            exit_procedure();
        }
        conn_ip = inet_ntoa(conn_socketaddr.sin_addr);
        if (printf_enabled) printf("Accepted connection from %s\n", conn_ip);
        syslog(LOG_DEBUG, "Accepted connection from %s\n", conn_ip);


        // Create connection thread with slist

        if (printf_enabled) printf("datap malloc start\n");
        datap = malloc(sizeof(slist_data_t));
        datap->threadInfo = malloc(sizeof(thread_info_t));
        if (printf_enabled) printf("datap malloc end\n");
        //datap->threadInfo.thread_id = malloc(sizeof(pthread_t));
        datap->threadInfo->conn_socket = new_conn_socket;
        datap->threadInfo->connection_done = false;
        SLIST_INSERT_HEAD(&head, datap, entries);

        int error = pthread_create(&datap->threadInfo->thread_id,
                    NULL,//&attr,
                    (void*) &connection_thread, 
                    datap->threadInfo);//&tinfo[tnum]);
        if (error != 0)
        {
            // error handle_error_en(s, "pthread_create");
            // TODO
            exit(-1);
        }

        // pthread_join.
        SLIST_FOREACH(datap, &head, entries)
        {
            //if (printf_enabled) printf("Read1: %d\n", datap->value);
            pthread_join(datap->threadInfo->thread_id, NULL);

            //datap = SLIST_FIRST(&head);
            //if (printf_enabled) printf("Read2: %d\n", datap->value);
            //SLIST_REMOVE_HEAD(&head, entries);
            // free(datap);
            free(datap->threadInfo);
            SLIST_REMOVE(&head, datap, slist_data_s, entries);

        }

    }   // while

    if (printf_enabled) printf("Ending \n");
    syslog(LOG_DEBUG, "Ending \n");
    pthread_mutex_destroy(&mutexLock);
    pthread_cancel(timestamp_threadId);
    pthread_join(timestamp_threadId, NULL);

    if (fp)
    {
        fclose(fp);
    }

    remove(DATA_FILE);

    if (printf_enabled) printf("End.\n");
    return 0;
}