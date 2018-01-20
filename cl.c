#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <zconf.h>

#define COLOR_GREEN   "\x1b[32m"
#define RESET   "\033[0m"
#define STYLE_BOLD         "\033[1m"
#define STYLE_NO_BOLD      "\033[22m"
#define BOLDYELLOW  "\033[1m\033[33m"

#define USERNAME_SIZE 32
#define MESSAGE_SIZE_IN_BYTES 2112
#define COMMUNICATION_KEY 0x0001997
#define AMOUNT_OF_USERS 9
#define AMOUNT_OF_GROUPS 3
#define WELCOME_MESSAGE "You have logged in, welcome to the community"
#define USER_ALREADY_LOGGED_IN_MESSAGE "This user is already logged in"
#define NO_USER_FOUND "Cannot find that user in the database"

int ppid;
int pid;

typedef struct
{
    long mtype;
    char recieverName[32];
    char senderName[32];
    char message[2048];
} Message;

void nullifyMessage(Message msg)
{
    memset(&msg, 0, sizeof(Message));
}

void shutdownServer()
{
    printf("\nClient shutting down... \n");
    kill(pid, -9);
    exit(0);
}

int launchLoginActivity(int id, char nickname[], char password[])
{
    Message msg, temp1;
    msg.mtype=1001;
    int priv=0;
    strncpy(msg.senderName, nickname, 2048);
    strcpy(msg.message, password);
    msgsnd(id, &msg, MESSAGE_SIZE_IN_BYTES, 0);
    msgrcv(id, &temp1, MESSAGE_SIZE_IN_BYTES, 0, 0);
    if(temp1.mtype==1011){
        long key;
        printf("%s\n", WELCOME_MESSAGE);
        key = strtol(temp1.message, NULL, 10);
        priv = msgget(key, IPC_CREAT|0644);
        nullifyMessage(temp1);
    }
    else if(temp1.mtype==1013){
        if (strcmp(temp1.message,"0")==0) printf("%s\n", NO_USER_FOUND);
        else if (strcmp(temp1.message,"-1")==0) printf("Wrong password\n");
        else if (strcmp(temp1.message, "-2")==0) printf("Account blocked!\n");
        else if (strcmp(temp1.message, "-3")==0) printf("%s\n",USER_ALREADY_LOGGED_IN_MESSAGE);
    }

    return priv;
}

int launchLogoutActivity(long id, char name[])
{
    Message msg, msg2;
    msg.mtype=1002;
    strcpy(msg.senderName, name);
    msgsnd(id, &msg, MESSAGE_SIZE_IN_BYTES, 0);
    msgrcv(id, &msg2, MESSAGE_SIZE_IN_BYTES, 1012, 0);
    printf("%s", msg2.message);
    if(msg2.mtype==1012) return 0;
    else return 1;
}

void launchSendingActivity(int id, char name[])
{
    Message msg;
    char test;
    scanf("%s", msg.recieverName);
    scanf("%c", &test);
    fgets(msg.message, sizeof(msg.message), stdin);
    msg.mtype = 1031;
    strcpy(msg.senderName, name);
    msgsnd(id, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

Message getMessage(int privId)
{
    Message msg={0};
    if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1012, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1030, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1033, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1034, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1042, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1043, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1046, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1081, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1082, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1083, IPC_NOWAIT)>0) return msg;
    else if(msgrcv(privId, &msg, MESSAGE_SIZE_IN_BYTES, 1084, IPC_NOWAIT)>0) return msg;
    else return msg;
}

void sendConfirmationMessage(int privId, char name[])
{
    Message conf;
    conf.mtype=1020;
    strcpy(conf.recieverName,name);
    msgsnd(privId, &conf, MESSAGE_SIZE_IN_BYTES, 0);
}

void displayUserMessage(int privateQue, Message recievedMessage, char name[])
{
    printf(STYLE_BOLD COLOR_GREEN "%s " RESET STYLE_NO_BOLD, recievedMessage.senderName);
    printf("whispers: %s", recievedMessage.message);
}

void launchGroupSendingActivity(char name[], int privateQue)
{
    char wat;
    Message msg;
    msg.mtype=1032;
    scanf("%s", (char*)&msg.recieverName);              //nazwa grupy do ktorej wyslana bedzie wiadomosc
    scanf("%c", &wat);
    fgets(msg.message, 2048, stdin);                    //sama wiadomosc
    strcpy(msg.senderName, name);                       //dolaczenie nazwy wysylajacego
    msgsnd(privateQue, &msg, MESSAGE_SIZE_IN_BYTES, 0); //wyslij prywatna kolejka do serwera
}

void launchJoinActivity(char name[], int privateQue)
{
    Message msg;
    msg.mtype=1071;
    strcpy(msg.senderName, name);
    scanf("%s", msg.message);
    msgsnd(privateQue, &msg, MESSAGE_SIZE_IN_BYTES, 0);
    nullifyMessage(msg);
}

void launchLeaveActivity(char name[], int privateQue)
{
    Message msg;
    msg.mtype=1072;
    strcpy(msg.senderName, name);
    scanf("%s", msg.message);
    msgsnd(privateQue, &msg, MESSAGE_SIZE_IN_BYTES, 0);
    nullifyMessage(msg);
}

void launchFingerActivity(char name[], int publicQue)
{
    Message msg;
    msg.mtype=1040;
    strcpy(msg.senderName, name);
    msgsnd(publicQue, &msg, MESSAGE_SIZE_IN_BYTES, 0);
    nullifyMessage(msg);
}

void launchGingerActivity(char name[], int privateQue)
{
    Message msg;
    msg.mtype=1041;
    strcpy(msg.senderName, name);
    scanf("%s", msg.message);
    printf(COLOR_GREEN "Users in group (%s)\n" RESET, msg.message);
    msgsnd(privateQue, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

void showHelp()
{
    printf("whisper <username> <message> - sends a message to specified user\n");
    printf("join <groupname> - adds currently logged user to given group\n");
    printf("leave <groupname> - removes currently logges user from the given group\n");
    printf("finger - shows all active users\n");
    printf("ginger <groupname> - show all users belonging to given group\n");
    printf("broadcast <groupname> <message> - send a message to all members of the given group\n");
    printf("logout - logs the user out of server\n");
}

void launchGroupsList(char name[], int privateQue)
{
    Message msg;
    msg.mtype=1045;
    strcpy(msg.senderName, name);
    printf(BOLDYELLOW "Available groups:\n" RESET);
    msgsnd(privateQue, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

int main()
{
    signal(SIGINT, shutdownServer);
    int privateQueueId = 0;
    char name[32], comm[20], password[32];
    int logId = msgget(COMMUNICATION_KEY, IPC_CREAT|0644);
    do {
        printf("Login:\n");
        fflush(stdin);
        fflush(stdout);
        scanf("%s", (char*)&name);
        printf("Password:\n");
        scanf("%s",(char*)&password);
        privateQueueId = launchLoginActivity(logId, name, password);                // pod privateQueueId podstaw id kolejki prywatnej
    } while(privateQueueId==0 );
    printf("Write help to view available commands\n");
   if ((pid = fork())) {                                                           // proces rodzic do obslugi komend
        while (privateQueueId > 0)
        {
            scanf("%s", (char*)&comm);
            if (strcmp(comm, "whisper")==0) {
                launchSendingActivity(privateQueueId, name);
            }
            else if (strcmp(comm, "logout")==0) {
                privateQueueId = launchLogoutActivity(logId, name);
            }
            else if(strcmp(comm, "finger")==0){
                printf(COLOR_GREEN "Online users:\n" RESET);
                launchFingerActivity(name, logId);
            }
            else if(strcmp(comm, "ginger")==0){
                launchGingerActivity(name, privateQueueId);
            }
            else if(strcmp(comm, "broadcast")==0){
                launchGroupSendingActivity(name, privateQueueId);
            }
            else if(strcmp(comm, "join")==0){
                launchJoinActivity(name, privateQueueId);
            }
            else if(strcmp(comm, "leave")==0){
                launchLeaveActivity(name, privateQueueId);
            }
            else if(strcmp(comm, "groups")==0){
                launchGroupsList(name, privateQueueId);
            }
            else if(strcmp(comm, "help")==0){
                showHelp();
            }
            else printf("Command not recognised\n");
        }
    }

    else {                                                                          // proces potomny do odbioru wiadomosci z kolejki prywatnej
       Message recievedMessage;
       ppid = getppid();
       while(1){
           recievedMessage = getMessage(privateQueueId);
           if(recievedMessage.mtype == 1030){
                displayUserMessage(privateQueueId, recievedMessage, name);
           }
           else if(recievedMessage.mtype == 1081 || recievedMessage.mtype == 1082
                || recievedMessage.mtype == 1083 || recievedMessage.mtype == 1084
                || recievedMessage.mtype == 1034 || recievedMessage.mtype == 1042
                || recievedMessage.mtype == 1043 || recievedMessage.mtype == 1046){
               printf("%s\n", recievedMessage.message);
           }
           else if(recievedMessage.mtype == 1033){
               printf(BOLDYELLOW "Group message from %s (group %s): " RESET , recievedMessage.senderName,  recievedMessage.recieverName);
               printf("%s", recievedMessage.message);
           }
           else if(recievedMessage.mtype == 1012){
               kill(pid, SIGKILL);
               _exit(0);
           }
           nullifyMessage(recievedMessage);
       }
   }
    return 0;
}