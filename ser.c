#include <stdio.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <zconf.h>

#define USERNAME_SIZE 32
#define MESSAGE_SIZE_IN_BYTES 2112
#define COMMUNICATION_KEY 0x0001997
#define AMOUNT_OF_USERS 9
#define AMOUNT_OF_GROUPS 3
#define WELCOME_MESSAGE "You have logged in, welcome to the community"
#define USER_ALREADY_LOGGED_IN_MESSAGE "This user is already logged in"
#define NO_USER_FOUND "Cannot find that user in the database"
#define WRONG_PASSWORD "Wrong password!"


int pid;
int logId;

typedef struct
{
    long mtype;
    char recieverName[32];
    char senderName[32];
    char message[2048];
} Message;

typedef struct
{
    short online;
    int qId;
    long key;
    char userName[32];
    char password[32];
    short ifBlocked;
    short loginAttempts;
} User;

typedef struct
{
    User users[AMOUNT_OF_GROUPS];
    char name[USERNAME_SIZE];
    short usersInGroup;
} Group;

User allUsers[AMOUNT_OF_USERS];
Group groups[AMOUNT_OF_GROUPS];

int findUser(char name[])
{
    int i;
    for(i=0; i<AMOUNT_OF_USERS; i++){
        if(strcmp(name, allUsers[i].userName)==0) return i;
    }
    return -1;
}

int findGroup(char name[])
{
    int i;
    for(i=0; i<AMOUNT_OF_GROUPS; i++){
        if(strcmp(name, groups[i].name)==0) return i;
    }
    return -1;
}

int isUserInGroup(char username[], char groupname[])
{
    int i = findGroup(groupname), j;
    for(j=0; j<groups[i].usersInGroup; j++){
        if(strcmp(username, groups[i].users[j].userName)==0) return j;
    }
    return -1;
}

int isUserLogged(char name[])
{
    int i = findUser(name);
    if(allUsers[i].online == 1) return 1;
    else return 0;
}

void setDatabase()
{
    int i;
    FILE *fd = fopen("config", "r");
    for(i=0; i<AMOUNT_OF_USERS; i++){
        fscanf(fd, "%s", (char *)&allUsers[i].userName);
        fscanf(fd, "%ld", &allUsers[i].key);
        fscanf(fd, "%s", allUsers[i].password);
        allUsers[i].online=0;
    }
    for(i=0; i<AMOUNT_OF_GROUPS; i++){
        fscanf(fd, "%s", groups[i].name);
        groups[i].usersInGroup=0;
    }

}

void nullifyMessage(Message msg)
{
    memset(&msg, 0, sizeof(Message));
}

void showDatabase()
{
    int i;
    for(i =0; i<AMOUNT_OF_USERS; i++){
        printf("%s %ld   Online: %d   Queue id: %d\n", allUsers[i].userName, allUsers[i].key, allUsers[i].online, allUsers[i].qId);
    }
    printf("\n");
}

void showGroupsWithUsers()
{
    int i,j;
    for(i=0; i<AMOUNT_OF_GROUPS; i++){
        printf("%s: ", groups[i].name);
        for(j=0; j<groups[i].usersInGroup;j++){
            if(strcmp(groups[i].users[j].userName,"")!=0) {
                printf("\n%s", groups[i].users[j].userName);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void closeUserQueues()
{
    int i;
    for(i=0; i<AMOUNT_OF_USERS; i++){
        msgctl(allUsers[i].qId, IPC_RMID, NULL);
    }

}

void launchUserQueues()
{
    int i;
    for(i=0; i<AMOUNT_OF_USERS; i++){
        allUsers[i].qId=msgget(allUsers[i].key, IPC_CREAT|0644);
    }
}

Message setListenersForUsers()
{
    Message msg;
    int i;
    msg.mtype=0;
    for(i=0; i<AMOUNT_OF_USERS; i++){
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1011, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1012, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1013, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1031, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1032, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1040, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1041, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1045, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1071, IPC_NOWAIT)>0) return msg;
        if(msgrcv(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 1072, IPC_NOWAIT)>0) return msg;
    }
    return msg;
}

void loginActivity(char name[], char password[])
{
    Message msg;
    int i= findUser(name);
    if(i>-1) {
        if(strcmp(allUsers[i].password, password)==0 && allUsers[i].ifBlocked==0 && allUsers[i].online==0) {
            allUsers[i].online = 1;
            sprintf(msg.message, "%ld", allUsers[i].key);
            strcpy(msg.senderName, "0");
            strcpy(msg.recieverName, allUsers[i].userName);
            msg.mtype = 1011;
            allUsers[i].loginAttempts=0;
        }
        else if (allUsers[i].ifBlocked==1){
            msg.mtype=1013;
            strcpy(msg.message, NO_USER_FOUND);
        }
        else if (isUserLogged(name)==1){
            msg.mtype=1013;
            strcpy(msg.message, USER_ALREADY_LOGGED_IN_MESSAGE);
        }
        else{
            allUsers[i].loginAttempts++;
            msg.mtype=1013;
            strcpy(msg.message, WRONG_PASSWORD);
            if(allUsers[i].loginAttempts>=5) allUsers[i].ifBlocked=1;
        }
    }
    else {
        msg.mtype=1013;
        strcpy(msg.message, "0");
    }
    msgsnd(logId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

void shutdownServer()
{
    printf("Server shutting down...\n");
    msgctl(logId, IPC_RMID, NULL);
    closeUserQueues();
    kill(pid, -9);
    exit(0);
}

void logoutActivity(char name[])
{
    Message msg;
    int i = findUser(name);
    allUsers[i].online=0;
    msg.mtype=1012;
    strcpy(msg.senderName, "0");
    strcpy(msg.recieverName, allUsers[i].userName);
    strcpy(msg.message, "Logged out succesfully\n");
    msgsnd(allUsers[i].qId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

void sendingActivity(Message msg)
{
    int i = findUser(msg.recieverName);
    int destQue = allUsers[i].qId;
    msg.mtype=1030;
    msgsnd(destQue, &msg, MESSAGE_SIZE_IN_BYTES, 0);
    nullifyMessage(msg);
    printf("Message delivered successfully\n");
}

void groupSendingActivity(Message msg)
{
    int j;
    int i = findGroup(msg.recieverName);
    if(groups[i].usersInGroup>0) {
        for (j = 0; j < groups[i].usersInGroup; j++) {
            msg.mtype = 1033;
            int id = findUser(groups[i].users[j].userName);
            msgsnd(allUsers[id].qId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
        }
    }
}

void sendJoinConfirmation(int userPos)
{
    Message msg;
    msg.mtype=1081;
    strcpy(msg.message, "Added to group!\n");
    msgsnd(allUsers[userPos].qId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

void sendJoinCancellation(int userPos)
{
    Message msg;
    msg.mtype=1083;
    strcpy(msg.message, "An error occured when adding to a group!\n");
    msgsnd(userPos, &msg, MESSAGE_SIZE_IN_BYTES,0);
}

void groupJoinActivity(Message msg)
{
    int i = findGroup(msg.message);
    int user = findUser(msg.senderName);
    if(i>-1 && (isUserInGroup(msg.senderName, msg.message)==-1)) {
        strcpy(groups[i].users[groups[i].usersInGroup].userName, msg.senderName);
        groups[i].usersInGroup++;
        sendJoinConfirmation(user);
    }
    else sendJoinCancellation(user);
    nullifyMessage(msg);
}

void sendLeaveCancellation(int userPos)
{
    Message msg;
    msg.mtype=1084;
    strcpy(msg.message, "You are not logged in this group or no such group!\n");
    msgsnd(userPos, &msg, MESSAGE_SIZE_IN_BYTES,0);
}

void sendLeaveConfirmation(int userPos)
{
    Message msg;
    msg.mtype=1082;
    strcpy(msg.message, "Left the group!\n");
    msgsnd(allUsers[userPos].qId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

void groupLeaveActivity(Message msg)
{
    int i = findGroup(msg.message);
    int user = findUser(msg.senderName);
    int userInGroupArray = isUserInGroup(msg.senderName, msg.message);
    if(i>-1 && (userInGroupArray>=0)) {
        strcpy(groups[i].users[userInGroupArray].userName, "");
        sendLeaveConfirmation(user);
    }
    else sendLeaveCancellation(user);
    nullifyMessage(msg);
}

void fingerActivity(Message msg)
{
    int i;
    int user = findUser(msg.senderName);
    Message resMsg;
    strcpy(resMsg.message, "");
    resMsg.mtype=1042;
    for(i=0; i<AMOUNT_OF_USERS; i++){
        if(allUsers[i].online==1) {strcat(resMsg.message, allUsers[i].userName); strcat(resMsg.message, "\n");}
    }
    msgsnd(allUsers[user].qId, &resMsg, MESSAGE_SIZE_IN_BYTES, 0);
    nullifyMessage(resMsg);
}

void gingerActivity(Message msg)
{
    msg.mtype=1043;
    int user = findUser(msg.senderName);
    int id = findGroup(msg.message), i;
    strcpy(msg.message, "");
    for(i=0; i<groups[id].usersInGroup; i++){
        if(strcmp(groups[id].users[i].userName, "")!=0) {strcat(msg.message, groups[id].users[i].userName); strcat(msg.message, "\n");}
    }
    msgsnd(allUsers[user].qId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

void groupListActivity(Message msg)
{
    int i;
    msg.mtype=1046;
    int id = findUser(msg.senderName);
    strcpy(msg.message,"");
    for(i=0; i<AMOUNT_OF_GROUPS; i++){
        strcat(msg.message, groups[i].name);
        strcat(msg.message, "\n");
    }
    msgsnd(allUsers[id].qId, &msg, MESSAGE_SIZE_IN_BYTES, 0);
}

int main()
{
    signal(SIGINT, shutdownServer);
    Message publicMessage, privateMessage;
    setDatabase();                                                                  //ustawienie bazy uzytkownikow
    launchUserQueues();                                                             //odpalenie kolejek prywatnych
    showDatabase();
    showGroupsWithUsers();
    logId = msgget(COMMUNICATION_KEY, IPC_CREAT|0644);                              //otwarcie kolejki publicznej

    if((pid = fork()))
    {
        while (1)
        {
            privateMessage = setListenersForUsers();                              // ustawienie nasluchu na prywatnych kolejkach userow
            if (privateMessage.mtype == 1002) {
                printf("Recieved logout request from %s ... Processing.. \n", privateMessage.senderName);
                logoutActivity(privateMessage.senderName);
                showDatabase();
            } else if (privateMessage.mtype == 1031) {
                printf("Recieved private message... Processing..\n");
                sendingActivity(privateMessage);
            } else if (privateMessage.mtype == 1032){
                printf("Recieved group message from %s ... Processing..\n", privateMessage.senderName);
                groupSendingActivity(privateMessage);
            } else if (privateMessage.mtype == 1071){
                printf("Recieved group join request... Processing\n");
                groupJoinActivity(privateMessage);
                showGroupsWithUsers();
            } else if (privateMessage.mtype == 1072){
                printf("Recieved group leave request... Processing\n");
                groupLeaveActivity(privateMessage);
                showGroupsWithUsers();
            } else if(privateMessage.mtype == 1041){
                printf("Recieved group members request... Processing\n");
                gingerActivity(privateMessage);
            } else if(privateMessage.mtype == 1045){
                printf("Recieved grop list request... Processing\n");
                groupListActivity(privateMessage);
            }
        }
    }
    else
    {
        while(1){
            msgrcv(logId, &publicMessage, MESSAGE_SIZE_IN_BYTES, 0, 0);           // ustawienie nasluchu na kolejce publicznej do logowania, wylogowywania i spisu online
            if (publicMessage.mtype == 1001) {
                printf("Recieved login request from %s ... Processing.. \n", publicMessage.senderName);
                loginActivity(publicMessage.senderName, publicMessage.message);
                showDatabase();
                nullifyMessage(publicMessage);
            }
            else if (publicMessage.mtype == 1002) {
                printf("Recieved logout request from %s ... Processing.. \n", publicMessage.senderName);
                logoutActivity(publicMessage.senderName);
                showDatabase();
                nullifyMessage(publicMessage);
            }
            else if (publicMessage.mtype == 1040){
                printf("Recieved show logged users request... Processing\n");
                fingerActivity(publicMessage);
            }
        }
    }
    //return 0;
}
