/*
This code primarily comes from
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>


char* t;
int cORf = 0;
double historyT[3600];
int countT=0;
int over1hour=0;


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

        cORf++;

        int i;
        double max=-100.0;
        double min=100.0;
        double avg=0.0;
        double sum=0.0;
        pthread_mutex_lock(&lock);
        if(over1hour==0)
        {
            for(i=0; i<countT; i++)
            {
                if(historyT[i]>max)
                {
                    max=historyT[i];
                }
                if(historyT[i]<min)
                {
                    min=historyT[i];
                }
                sum+=historyT[i];
                avg=sum/countT;
            }
            printf("max is %f\n",max);
            printf("min is %f\n",min);
            printf("avg is %f\n",avg);
        }
        else
        {
            for(i=countT; i<3600; i++)
            {
                if(historyT[i]>max)
                {
                    max=historyT[i];
                }
                if(historyT[i]<min)
                {
                    min=historyT[i];
                }
                sum+=historyT[i];
            }
            for(i=0; i<countT; i++)
            {
                if(historyT[i]>max)
                {
                    max=historyT[i];
                }
                if(historyT[i]<min)
                {
                    min=historyT[i];
                }
                sum+=historyT[i];
            }
            avg=sum/3600;
            printf("max is %f\n",max);
            printf("min is %f\n",min);
            printf("avg is %f\n",avg);
        }
        pthread_mutex_unlock(&lock);

        // this is the message that we'll send back
        /* it actually looks like this:
          {
             "name": "cit595"
          }
        */
        //char *reply = "{\n\"name\": \"cit595\"\n}\n";
        pthread_mutex_lock(&lock);
        char *reply = malloc(sizeof(char)*(strlen("{\n\"name\": \"")+strlen(t)+strlen("\"\n}\n")+1));
        strcpy(reply,"{\n\"name\": \"");
        strcat(reply,t);
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
    /*
    	1. Open the file
    */
    int fd = open("/dev/ttyACM0", O_RDWR);//88888888888888888888888888888888888888888888888888888888888888888888888
    if (fd == -1)
        printf("unsuccessfully opened the usb port\n");

    /*
    	2. Configure fd for USB
    */
    struct termios options; // struct to hold options
    tcgetattr(fd, &options); // associate with this fd
    cfsetispeed(&options, 9600); // set input baud rate
    cfsetospeed(&options, 9600); // set output baud rate
    tcsetattr(fd, TCSANOW, &options); // set options

    /*
    	3. Read & Print
    */
    char buf[100];
    int bytes_read = read(fd, buf, 100);
    char tem[100];
    char tem2[100];
    strcpy(tem,buf);



    char* c = "c";
    char* f = "f";
    char* C = "C";
    char* F = "F";
    int localcORf=0;

    char* token;
    double tempT;
    int stableCount = 0;


    /* make sure the output of arduino is updated (removing "the temperature is" and "\n"), so that no strtoke is used here*/
    while(1)
    {

        if(localcORf!=cORf)
        {
            if(cORf%2==0)
            {
                write(fd,c,strlen(c)+1);
                printf("c is sent to urduino\n");
            }
            else
            {
                write(fd,f,strlen(f)+1);
                printf("f is sent to urduino\n");
            }
            localcORf=cORf;
        }

        if(tem[strlen(tem) - 1] == '\n')
        {

            // add temperature into historyT array
            strcpy(tem2, tem);
            token = strtok(tem2, " \n");
            tempT = atof(token);
            //printf("tempT is %f\n", tempT);
            token = strtok(NULL, " \n");
            //printf("token is--%s--\n", token);

            //put temperature recodes into the array and count the number of records
            if(strcmp(token,F)==0)
            {
                tempT=(tempT-32)*(5.0/9.0);
            }

            // the first temperature is always unstable and it will casue problems for calculating max and min,
            //the first five records will be discarded.
            if(stableCount>5)
            {
                pthread_mutex_lock(&lock);
                historyT[countT]=tempT;
                countT++;
                if(countT==3600)
                {
                    over1hour=1;
                }
                countT=countT%3600;
                pthread_mutex_unlock(&lock);
            }
            stableCount++;

            pthread_mutex_lock(&lock);
            free(t);
            t=malloc(sizeof(char)*(strlen(tem)+1));
            strcpy(t,tem);
            printf("t is %s\n", t);
            pthread_mutex_unlock(&lock);

            memset(buf,0,sizeof(buf));
            memset(tem,0,sizeof(tem));
        }
        else
        {
            strcat(tem, buf);
            memset(buf,0,sizeof(buf));
        }

        bytes_read = read(fd, buf, 100);
    }

    /*
    	4. Last step
    */
    close(fd);

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

