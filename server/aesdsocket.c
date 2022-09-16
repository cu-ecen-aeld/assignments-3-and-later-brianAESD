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



#define MAX_MESSAGE_SIZE 1024

#define PORT_TARGET "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
//#define DATA_FILE "mytest.txt"
#define MAX_NUM_CONNECTION 10

struct addrinfo* addrinfo_res;
struct addrinfo hints;

bool caught_sig = false;
int sockfd = -1;
//int fp;
FILE* fp;

void connection_closed_procedure(void)
{
    //remove(DATA_FILE);
}

void exit_procedure(void)
{
    printf("Running exit_procedure.\n");

    if (addrinfo_res)
    {
        freeaddrinfo(addrinfo_res);
    }

    remove(DATA_FILE);

    if (sockfd != -1)
    {
        close(sockfd);
    }

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
        exit_procedure();
    }
}

int main(int argc, char **argv)
{
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

    // bind
    status = bind(sockfd, addrinfo_res->ai_addr, addrinfo_res->ai_addrlen);
    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to bind");
        exit_procedure();
    }

    // listen
    status = listen(sockfd, MAX_NUM_CONNECTION);
    if (status == -1)
    {
        syslog(LOG_ERR, "Failed to listen");
        exit_procedure();
    }


    // -D: run the aesdsocket application as a daemon
    
    if (argc >= 2 && strcmp(argv[1], "-d") == 0)
    {
        // fork
        pid_t childPid = fork();
        if (childPid == -1)
        {
            exit_procedure();
        }
    }

    // int returnState = 0;
    int new_conn_socket = 0;
    int bwanTest1 = 0;
    unsigned int sendSizeTotal = 0;

    while(1)
    {
        if (new_conn_socket != 0)
        {
            printf("new_conn_socket is not 0\n");
        }

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

#if 1
        fp = fopen(DATA_FILE,"a+");
        //FILE* fp_send = fp;
        //fp = fopen(DATA_FILE,"w+");
        //if (fp == -1)
        if (fp == NULL)
        {
            printf("Failed: Unable to open file.\n");
            syslog(LOG_ERR, "Failed: Unable to open file.\n");
            exit(-1);
        }
        printf("Opened: %s\n", DATA_FILE);
#endif

        bool connectionDone = 0;
        unsigned int receivedByteCount = 0;
        
        //unsigned int readRetryCnt = 0;

        while (connectionDone == 0)
        {


            int receiveResult = 0;
            const unsigned int receiveDataSize = 100;// 128;
            char receiveData[40000];
            memset(receiveData, 0, 40000);
        if (1)
        {
            // receive
            receiveResult = recv(new_conn_socket, receiveData, receiveDataSize, 0);
            //send(new_conn_socket, "abcdefg\n", 8, 0);
            if (receiveResult == -1)
            {
                // unsuccessful
                printf("Receive is unsuccessful.\n");
                connection_closed_procedure();
                //break;
            }
            else if (receiveResult == 0)
            {
                // // connection is closed
                // syslog(LOG_DEBUG, "Closed connection from %s\n", conn_ip);
                // // to accept new connection
                // connection_closed_procedure();

                // Receive is done.
                printf("Receive is done.\n");
                connectionDone = 1;

                //break;
            }
            else
            {
                // ok
                //readRetryCnt = 0;
                receivedByteCount += receiveResult;
                printf("Received (%d): %s", receiveResult, receiveData);
                bwanTest1++;
                //fwrite(receiveData, receiveResult, 1, fp);
                // Seek to the end of the file
                fseek(fp, 0, SEEK_END);
                fwrite(receiveData, sizeof(char), receiveResult, fp);
            }
        }



        //printf("send is next.\n");
        
        //while (1)
        if (receivedByteCount)
        {
            // send
            char sendData[40000];
            memset(sendData, 0, 40000);
            //if (fp <= 0) printf("what");
            // Seek to the beginning of the file
            fseek(fp, 0, SEEK_SET);
            //fread(sendData, 1, receiveResult, fp + sendSizeTotal);
            //fread(sendData, 1, sendSizeTotal + receiveResult, fp);
            fread(sendData, 1, sendSizeTotal + receivedByteCount, fp);
            if (sendData[sendSizeTotal + receivedByteCount - 1] != '\n')
            {
                printf("No newline.\n");
                continue;
            }
            //if (dataSize < 64) { int size = dataSize; printf("Send (%d): %s", size, sendData); }
            //int sendResult = send(new_conn_socket, sendData, 8, 0);
            //printf("Sending  (%d): %s\n", sendSizeTotal + receiveResult, sendData);
            printf("Sending  (%d): %s\n", sendSizeTotal + receivedByteCount, sendData);
            int sendResult;
            //int 
            //sendResult = send(new_conn_socket, receiveData, 8, 0);
            //if (bwanTest1 == 2)
            //sendResult = send(new_conn_socket, "hijklmnop\n", 10, 0);
            //else
            //sendResult = send(new_conn_socket, sendData, sendSizeTotal + receiveResult, 0);
            sendResult = send(new_conn_socket, sendData, sendSizeTotal + receivedByteCount, 0);
            sendSizeTotal += receivedByteCount;
            //printf("sending.\n");
            //while (1)
            {
            //sendResult = send(new_conn_socket, "qwertyu\n", 7, 0);
        //while(1);
            //if (sendResult) break;
            }
            //printf("sent.\n");
            //printf("Sent (%d): %s\n", sendResult, receiveData);
            //int sendResult = 0;//send(new_conn_socket, sendData, 1, 0);
            // if (sendResult == -1)
            // {
            //     // error
            // }
            //printf("Send (%d) end", sendResult);
            (void) sendResult;
            (void) sendData;
            (void) bwanTest1;
            //printf("Send is done\n");
            //send(new_conn_socket, "\n", 1, 0); //bwan del
        } //while (1);
        // else if (connectionDone)
        // {
        //     readRetryCnt++;
        //     if (readRetryCnt < 5)
        //     {
        //         // retry
        //         connectionDone = 0;
        //     }
        // }

        
        }
        printf("new connection is done.\n");
        close(new_conn_socket);
        //remove(DATA_FILE);
        fclose(fp);
        fp = 0;



        (void) new_conn_socket;
    }




#if 0
   if (argc != 3)
   {
      printf("Failed: Number of parameter (%d) not valid\n", argc-1);
      syslog(LOG_ERR, "Failed: Number of parameter (%d) not valid\n", argc-1);
      exit(1);
   }

   // First argument is full path to a file (including filename) on the filesystem
   char* writefile = argv[1];
   // Second argument is a text string which will be written within this file
   char* writestr = argv[2];
   unsigned int writeStrLen = strlen(writestr);

   printf("Writing %s to %s\n", writestr, writefile);
   syslog(LOG_DEBUG, "Writing %s to %s\n", writestr, writefile);
   FILE *fp=fopen(writefile,"wb");
   if (fp == NULL)
   {
      printf("Failed: Unable to open file.\n");
      syslog(LOG_ERR, "Failed: Unable to open file.\n");
      exit(1);
   }

   unsigned int numWritten = fwrite(writestr, 1, writeStrLen, fp);
   if (numWritten != writeStrLen)
   {
      printf("Failed: Unable to write %d bytes. Wrote %d bytes.\n", writeStrLen, numWritten);
      syslog(LOG_ERR, "Failed: Unable to write %d bytes. Wrote %d bytes.\n", writeStrLen, numWritten);
      exit(1);
   }
   printf("Wrote string.\n");

   fflush(fp);
   fclose(fp);
#endif

    //exit_procedure();
    printf("End.\n");
    return 0;
}
