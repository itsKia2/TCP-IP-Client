#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <ctype.h>

int createSocket();
void connectSocket(int fd_socket, struct sockaddr_in sockaddr);
void encryptMessage(char* plaintext, char* encrypted);

int main(int argc, char* argv[]) {
    int port;
    char* hostIP;
    char* myNETID;
    struct sockaddr_in address;
    char MsgToSend[7000];
    char receivedMsg[7000];
    int statusChecker;
    memset(&address, '\0', sizeof(address)); // fill address with \0

    // Initial error handling
    if (argc != 4) {
        printf("Improper Usage\n");
        printf("Usage: ./client [NETID@umass.edu] [port] [host IP]\n");
        return 0;
    }

    // init command line arguments
    hostIP = argv[3];
    port = atoi(argv[2]);
    myNETID = argv[1];

    int fd_socket = -100;
    fd_socket = createSocket(); // we assume it is successful

    // error handling for this included in createSocket()
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(hostIP);

    // connect to socket (we dont need return val)
    connectSocket(fd_socket, address); // assume successful

    // okay first format initial message saying HELLO
    // then send message to server with error handling
    sprintf(MsgToSend, "cs230 HELLO %s\n", myNETID);
    statusChecker = send(fd_socket, MsgToSend, strlen(MsgToSend), 0);
    if (!(statusChecker > 0)) {
        close(fd_socket);
        printf("Error sending HELLO message, EXITING\n");
        exit(-1);
    }
    printf("SENT: %s", MsgToSend);

    // now we will receive message (with error handling)
    memset(receivedMsg, 0, 7000);
    statusChecker = recv(fd_socket, receivedMsg, 7000, 0);
    if (!(statusChecker > 0)) {
        close(fd_socket);
        printf("Error receiving STATUS message, EXITING\n");
        exit(-1);
    }
    printf("RECEIVED: %s", receivedMsg);

    int shouldLoopRun = 1;
    while (shouldLoopRun) {

        // check if we got BYE
        char statusMsg[100];
        int k = 2;
        for (int i = strlen(receivedMsg) - 2; i > (strlen(receivedMsg) - 5); i--) {
            statusMsg[k] = receivedMsg[i];
            k--;
        }
        // printf("CHECKING STATUS: %s\n", statusMsg);

        if (strcmp(statusMsg, "BYE") == 0) {
            break;
        }

        //remove initial part from receivedMsg
        char toEncrypt[1000];
        int o = 0;
        for (int i = 13; i < strlen(receivedMsg); i++) {
            toEncrypt[o] = receivedMsg[i];
            o++;
        }
        // printf("CHECKING ENCRYPTABLE MSG OUTFUNCTION (SIZE : %ld): %s\n", sizeof(toEncrypt), toEncrypt);

        //encrypt the message and send it off
        char encryptedMsg[5000];
        // char cipher[26];
        memset(encryptedMsg, 0, sizeof(encryptedMsg));
        // memset(cipher, 0, sizeof(cipher));
        encryptMessage(toEncrypt, encryptedMsg);
        // reset MsgToSend array to use again
        memset(MsgToSend, '\0', sizeof(MsgToSend));
        sprintf(MsgToSend, "cs230 %s\n", encryptedMsg);
        statusChecker = send(fd_socket, MsgToSend, strlen(MsgToSend), 0);
        if (!(statusChecker >= 0)) {
            close(fd_socket);
            printf("Error sending ENCRYPTED message, EXITING\n");
            exit(-1);
        }
        printf("SENT: %s", MsgToSend);

        // now get next message
        memset(receivedMsg, 0, 7000);
        statusChecker = recv(fd_socket, receivedMsg, 7000, 0);
        if (!(statusChecker > 0)) {
            close(fd_socket);
            printf("Error receiving STATUS message, EXITING\n");
            exit(-1);
        }
        printf("RECEIVED: %s", receivedMsg);

    }

    printf("Successfully encrypted everything. SUCCESS, EXITING\n");
    close(fd_socket);
    return 1;
}

int createSocket() {
    int fd_socket = -1;
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (!(fd_socket > 0)) {
        printf("Error creating socket, EXITING\n");
        exit(-1);
    }
    printf("Successfully created socket\n");
    return fd_socket;
}

void connectSocket(int fd_socket, struct sockaddr_in sockaddr) {
    int socketStat = -1;
    socketStat = connect(fd_socket, (struct sockaddr_in*)&sockaddr, sizeof(sockaddr));
    if (socketStat < 0) {
        close(fd_socket);
        printf("Error connecting to socket, EXITING\n");
        exit(-2);
    }
    printf("Successful connection to socket\n");
    return;
}

void encryptMessage(char* plaintext, char* encrypted) {
    int countingArray[26];
    int highestCount = 0x123;
    int counter;
    // printf("CHECKING ENCRYPTABLE MSG INFUNCTION (SIZE : %ld): %s\n", sizeof(plaintext), plaintext);

    // init the entire counting array to 0
    for (int i = 0; i < 26; i++) {
        countingArray[i] = 0;
    }

    highestCount = 4;
    counter = 73;
    highestCount = 0;
    // go through plaintext and find how many occurances of each letter
    for (int j = 0; j < strlen(plaintext); j++) {
        if ((plaintext[j] >= 'A') && (plaintext[j] <= 'Z')) {
            // printf("CHECKING LETTER: %d\n", plaintext[j] - 'A');
            countingArray[plaintext[j] - 'A']++;
        }
    }

    counter = 0;
    while (counter < 26) {
        if (highestCount < countingArray[counter]) {
            highestCount = countingArray[counter];
        }
        counter = counter + 1;
    }

    // so now we have the highest count
    // now go through plaintext again and add all non-highestCount letters
    // to the new encrypted string
    counter = 0;
    int encryptedCount = 0;
    // printf("SIZE OF PLAINTEXT: %ld\n", sizeof(plaintext));
    while (counter < strlen(plaintext)) {
        // iterate through
        // only add those whos frequencies dont match highest freq
        // printf("CHECKING %c\n", plaintext[i]);
        if ((countingArray[plaintext[counter] - 'A'] != highestCount) && (isalpha(plaintext[counter]))) {
            // printf("NOT EQUAL FREQ FOR %c: %d != %d\n", (plaintext[i]), countingArray[plaintext[i] - 'A'], highestCount);
            encrypted[encryptedCount] = plaintext[counter];
            encryptedCount = encryptedCount + 1;
        }
        counter = counter + 1;
    }
    encrypted[encryptedCount + 1] = '\0';
    return;
}
