//
// Created by faldji on 11/12/2020.
//

#ifndef P2P_FUNCTIONS_H
#define P2P_FUNCTIONS_H

typedef struct User {
    char name[128];
    char password[128];
} User;
#endif //P2P_FUNCTIONS_H
//HEADER des fonctions utilise dans les serveurs et clients.
int clientJoinCentral(int socfd, struct sockaddr_in cli, struct sockaddr_in ser, int slen, User user);
int serverJoinCentral(int socfd, struct sockaddr_in cli, struct sockaddr_in ser, int slen, User user);
//Search for a particular file in server's database
int search(int socfd, struct sockaddr_in ser, int slen, char* key);
int fetch(int socfd, struct sockaddr_in cli, int clen, char* file_name);
