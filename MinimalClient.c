#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <string.h>
#include <signal.h>

char* ServerAddress;
char* Nickname;
char* Realname;
char* Password;

char* Port = "6667";

struct addrinfo* server;

int SocketFd;

#define CRLF "\r\n"

char* AddCRLF(char* CommandBuffer){
    uint16_t len = strlen(CommandBuffer);

    char* CommandString; 
    CommandString = malloc(len + sizeof(CRLF) + 1);
    memcpy(CommandString,CommandBuffer, len);
    memcpy(CommandString + len, CRLF, sizeof(CRLF));
    CommandString[len + sizeof(CRLF)] = '\0';

    return CommandString;
}

void SendCommand(char* Command){
    char TestBuffer[1024];

    send(SocketFd,Command,strlen(Command),0);
}

void ConstantlyRead(){

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    setsockopt(SocketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout , sizeof(timeout));

    char buffer[1024];
    while(1){
        memset(buffer,0,sizeof(buffer));

        recv(SocketFd,buffer,sizeof(buffer) - 1, 0);

        if (strncmp(buffer,"PING ",5) == 0){
            char PongBong[5 + strlen(buffer + 6)];
            memcpy(PongBong , "PONG ", sizeof("PONG "));

            strcat(PongBong, buffer + 6);
            char* PongMsg = AddCRLF(PongBong);
            
            SendCommand(PongMsg);
            free(PongMsg);
        }
        else{
            //PrintFullFilteredString(buffer);
            printf("%s",buffer);
        }
    }

}

void CreateSender(){
    pid_t p = fork();

    if(p < 0){
        perror("fork fail lol");
        exit(1);
    }

    //if p == 0, then it's child
    if (p == 0){
        ConstantlyRead();
    }
}

void SelectAddrinfo(struct addrinfo* info){
    printf("possible address ips:\n");

    struct addrinfo *p;

    for(p = info; p != NULL; p = p->ai_next){
        struct sockaddr_in* in_addr = (struct sockaddr_in* )p->ai_addr;

        if (strcmp(inet_ntoa(in_addr->sin_addr),"0.0.0.0") != 0){
            server = p;
            printf("Selected ip: %s\n",inet_ntoa(in_addr->sin_addr));
            return;
        }
    }

    printf("Couldn't find host or could only find ipv6 adresses (I'm to tired to make it ipv6 compatible)\n");
    exit(EXIT_FAILURE);
}

void InitializeUserInfo(){
    char ArgumentBuffer[255];

    //Server address --
    printf("Please input host: ");

    scanf("%s",ArgumentBuffer);
    uint16_t len = strlen(ArgumentBuffer);

    ServerAddress = malloc(len);

    memcpy(ServerAddress,ArgumentBuffer,len);
    
    if (ServerAddress[0] < 48 || ServerAddress[0] > 57){
        int s = 0;
        printf("Checking ip for server address: %s|\n",ServerAddress);
        if ((s = getaddrinfo(ServerAddress,Port,0,&server)) != 0){
            printf("Could not resolve hostname\n");
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));

            exit(EXIT_FAILURE);
        }
        SelectAddrinfo(server);
    }  
    //Forgot else condition, i'll fix that later
    
}

void SetUpConnection(){
    SocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketFd < 0){
        printf("failed to create socket\n");
    }

    if (connect(SocketFd, server->ai_addr, server->ai_addrlen) < 0){
        printf("Error establishing connection\n");
        
        exit(EXIT_FAILURE);
    }
}

void Cleanup(int signal){
    printf("SIGTERM signal recieved, terminating safely\n");
    SendCommand("QUIT :Unexpected SIGINT");

    close(SocketFd);

    exit(signal);
}

int MainLoop(){
    CreateSender();

    char Buffer[1024];
    uint16_t len;

    printf("First and foremost run: 'NICK <nickname>' and USER <nickname> 0 * :<realname>");

    while(1){
        fgets(Buffer,1024,stdin);
        len = strlen(Buffer);

        if (Buffer[len - 1] == '\n'){
            Buffer[len - 1] = '\0';
            len -= 1;
        }

        char* CommandString = AddCRLF(Buffer);
        SendCommand(CommandString);

        free(CommandString);
    }

    return 0;
}

int main(int argc, char** argv){
    printf("Welcome to the minimal(shit) client\n");
    printf("It basically can't even be classified as a client\n");

    InitializeUserInfo();

    SetUpConnection();
    signal(SIGINT, Cleanup);
    signal(SIGTERM,Cleanup);

    return MainLoop();
}