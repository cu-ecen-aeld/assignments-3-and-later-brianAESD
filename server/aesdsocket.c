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


#define PORT_TARGET "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
//#define DATA_FILE "mytest.txt"
#define MAX_NUM_CONNECTION 10

struct addrinfo* addrinfo_res;
struct addrinfo hints;

bool caught_sig = false;
int sockfd = -1;
FILE* fp = 0;

// void connection_closed_procedure(void)
// {
//     //remove(DATA_FILE);
// }

void exit_procedure(void)
{
    //printf("Running exit_procedure.\n");

    if (addrinfo_res)
    {
        freeaddrinfo(addrinfo_res);
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

    exit(-1);
}

static void signal_handler(int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        caught_sig = true;
        printf("Caught signal, exiting\n");
        syslog(LOG_ERR, "Caught signal, exiting");

        if (addrinfo_res)
        {
            freeaddrinfo(addrinfo_res);
        }

        remove(DATA_FILE);

        if (fp)
        {
            fclose(fp);
        }
        exit(0);
    }
}

int main(int argc, char **argv)
{
    printf("Starting aesdsocket\n");
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
            printf("Failed to fork\n");
            exit(-1);
        }
        else if (childPid)
        {
            printf("Fork %d\n", childPid);
            exit(0);
        }
    }

    // listen
    status = listen(sockfd, MAX_NUM_CONNECTION);
    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to listen");
        exit_procedure();
    }

    // int returnState = 0;
    int new_conn_socket = 0;
    int bwanTest1 = 0;
    unsigned int sendSizeTotal = 0;

    while(1)
    {
        // if (new_conn_socket != 0)
        // {
        //     printf("new_conn_socket is not 0\n");
        // }

        // accept - accept a connection on a socket
        struct sockaddr_in conn_socketaddr;
        int addrlen = sizeof(conn_socketaddr);
        new_conn_socket = accept(sockfd, (struct sockaddr*) &conn_socketaddr, (socklen_t*) &addrlen);
        if (new_conn_socket == -1)
        {
            syslog(LOG_ERR, "Failed to accept");
            exit_procedure();
        }
        char* conn_ip = inet_ntoa(conn_socketaddr.sin_addr);
        printf("Accepted connection from %s\n", conn_ip);
        syslog(LOG_DEBUG, "Accepted connection from %s\n", conn_ip);

        // Process data

        fp = fopen(DATA_FILE,"a+");
        if (fp == NULL)
        {
            printf("Failed: Unable to open file.\n");
            syslog(LOG_ERR, "Failed: Unable to open file.\n");
            exit(-1);
        }
        printf("Opened: %s\n", DATA_FILE);

        bool connectionDone = 0;
        unsigned int receivedByteCount = 0;

        while (connectionDone == 0)
        {
            int receiveResult = 0;
            const unsigned int receiveDataSize = 20000;
            char receiveData[80000];
            memset(receiveData, 0, 80000);

            // receive
            receiveResult = recv(new_conn_socket, receiveData, receiveDataSize, 0);
            if (receiveResult == -1)
            {
                // unsuccessful
                printf("Receive is unsuccessful.\n");
                //connection_closed_procedure();
            }
            else if (receiveResult == 0)
            {
                // connection is closed
                syslog(LOG_DEBUG, "Closed connection from %s\n", conn_ip);

                // Receive is done.
                printf("Receive is done.\n");
                connectionDone = 1;
            }
            else
            {
                // Received data
                receivedByteCount += receiveResult;
                printf("Received (%d): %s", receiveResult, receiveData);
                bwanTest1++;
                //fwrite(receiveData, receiveResult, 1, fp);
                // Seek to the end of the file
                fseek(fp, 0, SEEK_END);
                fwrite(receiveData, sizeof(char), receiveResult, fp);
            }

            // send
            if (receivedByteCount)
            {
                char sendData[80000];
                memset(sendData, 0, 80000);
                // Seek to the beginning of the file
                fseek(fp, 0, SEEK_SET);
                fread(sendData, 1, sendSizeTotal + receivedByteCount, fp);
                if (sendData[sendSizeTotal + receivedByteCount - 1] != '\n')
                {
                    printf("No newline.\n");
                    continue;
                }
                printf("Sending  (%d): %s\n", sendSizeTotal + receivedByteCount, sendData);
                int sendResult;
                sendResult = send(new_conn_socket, sendData, sendSizeTotal + receivedByteCount, 0);
                sendSizeTotal += receivedByteCount;
                (void) sendResult;
            }

        }
        printf("new connection is done.\n");
        close(new_conn_socket);
        //remove(DATA_FILE);
        fclose(fp);
        fp = 0;

    }


    printf("End.\n");
    return 0;
}
