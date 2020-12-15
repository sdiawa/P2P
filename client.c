//
// Created by dsk on 04/12/2020.
//

//Client

#include <crypt.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include "functions.h"


int main(int argc, char *argv[]) {
    int socfd,peerfd;    //socket file descriptor
    int slen, clen,servlen;    //length of  object
    struct sockaddr_in ser, cli,peer;    //object declaration of socket family
    struct User user = {.name ="", .password=""};
    time_t t;    //to store time value
    slen = sizeof(ser);
    clen = sizeof(cli);
    servlen = sizeof(peer);
    socfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socfd < 0) {
        printf("Erreur: socket non disponible.\n");
        return 0;
    }
    peerfd = socket(AF_INET, SOCK_STREAM, 0);
    if (peerfd < 0) {
        printf("Erreur: socket non disponible.\n");
        return 0;
    }

    //initailizing the server detail
    cli.sin_family = AF_INET;       //family type

    //assigning a random port and ip to client
    srand((unsigned) time(&t));
    cli.sin_port = htons(8000 + rand() % 100);
    cli.sin_addr.s_addr = INADDR_ANY;
    if (bind(socfd, (struct sockaddr *) &cli, clen) < 0) {
        printf("Erreur:  sync des sockets.\n");
        return 0;
    }

    peer.sin_family = AF_INET;	//type
    peer.sin_port = htons(8000 + rand() % 100);;
    peer.sin_addr.s_addr = INADDR_ANY;
    //bind socket descriptor to address for communication
    if (bind(peerfd, (struct sockaddr *) &peer, servlen) < 0) {
        printf("Error de Sync des sockect .\n");
        return 0;
    }

    //if ip address and port number is provided or not
    if (argc > 1) {
        ser.sin_addr.s_addr = inet_addr(argv[1]);    //inet_addr convers string into dotted decimal ip
        ser.sin_port = htons(atoi(argv[2]));
    } else {
        ser.sin_addr.s_addr = inet_addr("0.0.0.0");    //inet_addr convers string into dotted decimal ip
        ser.sin_port = htons(2020);
    }

    //to store the server details
    ser.sin_family = AF_INET;

    //connect to server
    if (connect(socfd, (struct sockaddr *) &ser, slen) < 0) {
        printf("Erreur: Le serveur centralié ne repond pas.\n");
        return 0;
    }

    printf("Client se connecte au serveur centralisé...\n");
    if (clientJoinCentral(socfd, cli, ser, slen, user) != 0) {
        printf("Erreur: username ou mot de passe incorrect\n");
        return -1;
    }
    printf("%s %hu/%hu\n",inet_ntoa(cli.sin_addr),ntohs(cli.sin_port), ntohs(peer.sin_port));

    //Recherche et partage de fichiers
    signal(SIGCHLD, SIG_IGN);
    int check;
    char key[50];
    while (1) {
bzero(key,50);
        printf("Voulez vous rechercher un fichier ? (Oui 1 / Non 0):\n");
        fflush(stdin);
        scanf("%d", &check);
        if (check == 1) {
            printf("Entrer un mot clé ou un nom de fichier :\n");
            scanf("%s", key);
            //recherche en fonction du mot clé
            search(socfd, ser, slen, key);
        } else {
            printf("Voulez Téléchargé un ficher (Oui 1 / Non 0):\n");
            fflush(stdin);
            scanf("%d", &check);
            if (check == 1) {
                printf("Entrer le nom d'un fichier:\n");
                fflush(stdin);
                scanf("%s", key);
                //telechargement du fichier
                fetch(socfd, cli, clen, key);
            } else {
                printf("A bientôt.\n");
                break;
            }
        }

        //Attendre 10 secondes puis reposer les questions
        clock_t new_time;
        clock_t cur_time = clock();
        do {
            new_time = clock();
        } while ((new_time - cur_time) / 1000 <= 20000);

    }
    close(socfd);
    return 1;
}

//Le client rejoint le serveur centralisé avec son login et mp.
int clientJoinCentral(int socfd, struct sockaddr_in cli, struct sockaddr_in ser, int slen, User user) {
    int mode = 1;
    int port = ntohs(cli.sin_port);
    char *ip = inet_ntoa(cli.sin_addr);
    int match;
    char toSend[1024];
    printf("Entrer votre pseudo\n");
    fgets(user.name, 128, stdin);
    printf("Entrer votre mot de passe\n");
    fgets(user.password, 128, stdin);
    strcpy(toSend, user.name);
    strcat(toSend, crypt(user.password, "Ex>Tn2O6oBJ=2pmSZeG9zS/59|[GE<*(_&t^VlY$S-W2>!/=2RJp* ?;cpoOr|uy"));
    send(socfd, &(mode), sizeof(int), 0);
    send(socfd, toSend, 1024, 0);
    recv(socfd, &match, sizeof(int), 0);
    if (match == -1 || match == 403) {
        printf("access réfusé :( mot de passe.\n");
        return -1;
    }
    if (match == 200) {
        printf("connecter avec success.\n");
    }
    return 0;
}

/*
 * recuperation d'un fichier distant
 */
int fetch(int socfd, struct sockaddr_in cli, int clen, char *file_name) {
    int code = 3;
    int status = 0;
    printf("Vérification de la disponibilité du fichier...\n");
    send(socfd, &code, sizeof(int), 0);
    send(socfd, file_name, 50, 0);
    recv(socfd, &status, sizeof(int), 0);

    if (status == 1) {
        char ipServ[25];
        int portServ;
        char buff[1024];
        FILE *fd;
        DIR *dir;
        dir = opendir("partage/");
        if (!dir) {
            printf("le dossier partage n'est pas disponible");
            return -1;
        }
        struct dirent *dp;
        while ((dp = readdir(dir)) != NULL) {
            if (strcmp(dp->d_name, file_name) == 0) {
                printf("Ce fichier existe déjà.\n");
                return -1;
            }
        }
        char newFilename[50];
        sprintf(newFilename, "partage/%s", file_name);
        fd = fopen(newFilename, "a");
        if (!fd) {
            printf("impossible de créer ce fichier.\n");
            return -1;
        }
        recv(socfd, ipServ, sizeof(ipServ), 0);
        recv(socfd, &portServ, sizeof(int), 0);
        int peerfd;
        peerfd = socket(AF_INET, SOCK_STREAM, 0);
        if (peerfd < 0) {
            printf("Erreur: socket non disponible.\n");
            return -1;
        }
        struct sockaddr_in temp,server;
        //initialise le client
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(ipServ);
        server.sin_port = htons(portServ);
        //connect to server
        if (connect(peerfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
            printf("Erreur: Le serveur distant ne repond pas.\n");
            fclose(fd);
            remove(newFilename);
            return -1;
        }
        send(peerfd, (const void *) 2, sizeof(int), 0);
        send(peerfd, file_name, 50, 0);
        recv(peerfd, &status, sizeof(int), 0);
        //recieve and write to file
        if (status != 1){
            printf("ce fichier n'est plus disponible chez l'hôte distant.\n");
            fclose(fd);
            remove(newFilename);
            close(peerfd);
            return -1;
        }
        while (status == 1) {
            bzero(buff, 1024);
            recv(peerfd, buff, 1024, 0);
            recv(peerfd, &status, sizeof(int), 0);
            fputs(buff,fd);
        }
        printf("Telechargement terminer\n");
        fclose(fd);
        close(peerfd);
        return 0;
    }
    printf("fichier non disponible au partage status =%d.\n",status);
    return -1;

}

//Recherche d'un fichier spécifique parmis les autres distants
int search(int socfd, struct sockaddr_in ser, int slen, char *key) {
    int search_id = 2;
    int found;
    char file_name[50], file_desc[256];
    char results[4096];
    bzero(file_name, 50);
    bzero(results, 4096);
    send(socfd, &(search_id), sizeof(int), 0);
    send(socfd, key, 50, 0);
    printf("Recherche en cours: %s ...\n", key);

    recv(socfd, &found, sizeof(int), 0);
    if (found == 1) {
        //fichier trouvé
        char ip[25];
        int port;
        bzero(ip, 25);
        recv(socfd, results, sizeof(results), 0);
        char *token = strtok(results, "\n");
        if (!token) {
            printf("Aucun résultat trouvé :0\n");
            return -1;
        }
        sscanf(token, "%s %d %s %[^\n]", ip, &port, file_name, file_desc);
        printf("Resultats trouvés : \n");

        while (token != NULL) {
            sscanf(token, "%s %d %s %[^\n]", ip, &port, file_name, file_desc);
            printf("%s \t %s \t %u \t %s\n", file_name, ip, port, file_desc);
            token = strtok(NULL, "\n");
        }
        printf("Terminer\n\n");
        return 0;
    } else {
        // pas de chance:)
        printf("Aucun résultat trouvé :%d\n", found);
        return -1;
    }
}
