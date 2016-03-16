/*
This code primarily comes from
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */
#include <windows.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

char* t;

pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;

void* start_server(void* pn)
{
    int PORT_NUMBER = *(int*)pn;

    // structs to represent the server and client
    struct sockaddr_in server_addr,client_addr;

    int sock; // socket descriptor

    // 1. socket: creates a socket descriptor that you later use to make other system calls
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(1);
    }
    int temp;
    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1)
    {
        perror("Setsockopt");
        exit(1);
    }

    // configure the server
    server_addr.sin_port = htons(PORT_NUMBER); // specify port number
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero),8);

    // 2. bind: use the socket and associate it with the port number
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Unable to bind");
        exit(1);
    }

    // 3. listen: indicates that we want to listn to the port to which we bound; second arg is number of allowed connections
    if (listen(sock, 5) == -1)
    {
        perror("Listen");
        exit(1);
    }

    // once you get here, the server is set up and about to start listening
    printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
    fflush(stdout);


    // 4. accept: wait here until we get a connection on that port
    while(1)
    {
        int sin_size = sizeof(struct sockaddr_in);
        int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
        printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

        // buffer to read data into
        char request[1024];

        // 5. recv: read incoming message into buffer
        int bytes_received = recv(fd,request,1024,0);
        // null-terminate the string
        request[bytes_received] = '\0';
        printf("Here comes the message:\n");
        printf("%s\n", request);



        // this is the message that we'll send back
        /* it actually looks like this:
          {
             "name": "cit595"
          }
        */
        //char *reply = "{\n\"name\": \"cit595\"\n}\n";
        pthread_mutex_lock(&lock);
        char *reply = malloc(sizeof(char)*(strlen("{\n\"name\": \"")+strlen(t)+strlen(" C")+strlen("\"\n}\n")+1));
        strcpy(reply,"{\n\"name\": \"");
        strcat(reply,t);
        strcat(reply," C");
        strcat(reply,"\"\n}\n");
        printf("%s\n",reply);
        pthread_mutex_unlock(&lock);


        // 6. send: send the message over the socket
        // note that the second argument is a char*, and the third is the number of chars
        send(fd, reply, strlen(reply), 0);
        //printf("Server sent message: %s\n", reply);

        // 7. close: close the socket connection
        free(reply);
        close(fd);
    }
    close(sock);
    printf("Server closed connection\n");

    pthread_exit(NULL);
}

void* readTemperature()
{
    DCB dcb;
    HANDLE hCom;
    BOOL fSuccess;
    char *pcCommPort = "COM5";

    hCom = CreateFile( pcCommPort,
                       GENERIC_READ | GENERIC_WRITE,
                       0,    // must be opened with exclusive-access
                       NULL, // no security attributes
                       OPEN_EXISTING, // must use OPEN_EXISTING
                       0,    // not overlapped I/O
                       NULL  // hTemplate must be NULL for comm devices
                     );

    if (hCom == INVALID_HANDLE_VALUE)
    {
        // Handle the error.
        printf ("CreateFile failed with error %d.\n", GetLastError());
        exit(1);
    }

    // Build on the current configuration, and skip setting the size
    // of the input and output buffers with SetupComm.
    fSuccess = GetCommState(hCom, &dcb);

    if (!fSuccess)
    {
        // Handle the error.
        printf ("GetCommState failed with error %d.\n", GetLastError());
        exit(1);
    }

    // Fill in DCB: 57,600 bps, 8 data bits, no parity, and 1 stop bit.

    dcb.BaudRate = CBR_9600;     // set the baud rate
    dcb.ByteSize = 8;             // data size, xmit, and rcv
    dcb.Parity = NOPARITY;        // no parity bit
    dcb.StopBits = ONESTOPBIT;    // one stop bit

    fSuccess = SetCommState(hCom, &dcb);

    if (!fSuccess)
    {
        // Handle the error.
        printf ("SetCommState failed with error %d.\n", GetLastError());
        exit(1);
    }

    printf ("Serial port %s successfully reconfigured.\n", pcCommPort);


    char buf[100];
    char* token;
    ReadFile(hCom, buf, 100, NULL, NULL);

    while(1)
    {
        token = strtok(buf,"\n");
        token = strtok(NULL," ");
        token = strtok(NULL," ");
        token = strtok(NULL," ");
        token = strtok(NULL," ");
        //printf("%s C\n", token);
        pthread_mutex_lock(&lock);
        free(t);
        t=malloc(sizeof(char)*(strlen(token)+1));
        strcpy(t,token);
        pthread_mutex_unlock(&lock);
        ReadFile(hCom, buf, 100, NULL, NULL);
    }

    /*
    	4. Last step
    */
    //close(hCom);

    pthread_exit(NULL);
}




int main(int argc, char *argv[])
{
    // check the number of arguments
    if (argc != 2)
    {
        printf("\nUsage: server [port_number]\n");
        exit(0);
    }

    int PORT_NUMBER = atoi(argv[1]);

    pthread_t t2;
    int ret_val;
    ret_val=pthread_create(&t2, NULL, &readTemperature, NULL);
    if(ret_val!=0)
    {
        printf("thread is not successfully created.\n");
        exit (1);
    }

    pthread_t t1;
    ret_val=pthread_create(&t1, NULL, &start_server, &PORT_NUMBER);
    if(ret_val!=0)
    {
        printf("thread is not successfully created.\n");
        exit (1);
    }
    pthread_join(t1, NULL);
}

