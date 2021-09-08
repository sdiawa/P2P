//serveur centralisé
//
//
// Created by dsk on 04/12/2020.
//
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include "functions.h"

#define PORT 2020
int id = 1;

int main(int argc, char *argv[]) {
    FILE *fp, *fpClient, *fpServer;    // liste des fichiers sur le serveur
    int socfd;    //socket du fichier
    int confd;  //conf de la connection
    int sLen;   //nbr serveur
    int cLen;   //nbr client
    int pid;    //process id
    struct sockaddr_in serv, cli;    //Sockets
    char user[256];
    int typeClient;

    //supp anciens clients et fichiers
    remove("partage/file_list.txt");
    remove("client_list.txt");

    printf("Démarrage du serveur centralisé...\n");

    /* Creatation du socket */
    socfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socfd < 0) {
        printf("Erreur de connection.\n");
        return 0;
    }

    //initialise le server
    serv.sin_family = AF_INET;    //type
    serv.sin_port = htons(PORT);
    serv.sin_addr.s_addr = INADDR_ANY;
    sLen = sizeof(serv);
    cLen = sizeof(cli);

    if (bind(socfd, (struct sockaddr *) &serv, sLen) < 0) {
        printf("Error de Sync des Serveurs.\n");
        return 0;
    }

    printf("Le serveur a bien demarer sur IP: %s et Port: %hd\n", inet_ntoa(serv.sin_addr), ntohs(serv.sin_port));

    listen(socfd, 10);        // écoute des clients.
    signal(SIGCHLD, SIG_IGN);
    while (1) {
        confd = accept(socfd, (struct sockaddr *) &cli, &cLen);        //respond to the request made by client
        printf("Connection en cours...\n");
        if (confd < 0) {
            printf("Connection refuser.\n");
            return -1;
        }
        recv(confd, &typeClient, sizeof(int), 0);
        printf(!typeClient ? "server\n" : "client\n");
        //Vérifie le username et mp côté client.
        recv(confd, user, 1024, 0);
        char *name;
        char *password;
        int match;

        if (user[0]) {
            char *token = strtok(user, "\n");
            name = token;
            while (token != NULL) {
                password = token;
                token = strtok(NULL, "\n");
            }
            //write peer ip and port to file
            fpClient = fopen("client_list.txt", "a");
            fp = fopen("client_list.txt", "r");
            match = -1;
            if (!fpClient || !fp) {
                printf("Erreur serveur de fichier clients\n");
            } else {
                char line[2048];
                char sepName[100];
                char sepPass[100];
                while (fgets(line, sizeof(line), fp) != NULL) {
                    strcpy(sepName, name);
                    strcpy(sepPass, password);
                    strcat(sepName, ":;");
                    strcat(sepPass, ":;");
                    if (strstr(line, sepName)) {
                        printf("%s trouver \n", name);
                        match = 403;
                        if (strstr(line, sepPass)) {
                            printf("mp corecte\n");
                            match = 200;
                        } else {
                            printf("mp incorecte\n");
                        }
                        break;
                    }
                }
                if (!typeClient && match == -1) {// nouvel utilisateur
                    fprintf(fpClient, "%s:;%s:;%d\n", name, password, id);
                    match = 201;
                    id++;
                }
                fclose(fpClient);
                fclose(fp);
            }

        } else {
            match = -1;
        }
        //repond le client pour son etat de connexion
        send(confd, &(match), sizeof(int), 0);
        if (match == 200 || match == 201)
            printf("%s connecter IP: %s Port: %hu\n", name, inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
        else {
            printf("Connection refuser pour %s.\n", name);
            continue;
        }

        if (!typeClient) {//requete serveur distant partager son fichier
            char filename[50];
            char fileDesc[256];
            int status;
            DIR *d;
            while (1){
                bzero(filename,50);
                bzero(fileDesc,256);
                status = 1;
                d = opendir("partage/");
                if (!d){
                    printf("Erreur impossible d'acceder au dossier partage.\n");
                    status = 0;
                    send(confd, &(status), sizeof (int), 0);
                    close(confd);
                    break;
                } else{
                    fpServer = fopen("partage/file_list.txt","a");
                    printf("un partage est en cours...\n");
                    if(!fpServer)
                    {
                        printf("Erreur serveur de fichier partagé.\n");
                        status = 0;
                        send(confd, &(status), sizeof (int), 0);
                        close(confd);
                        closedir(d);
                        break;
                    } else{
                        int portPartage;
                        status = 1;
                        send(confd, &(status), sizeof (int), 0);
                        recv(confd,fileDesc,256,0);
                        recv(confd,filename,50,0);
                        recv(confd, &portPartage, sizeof(int), 0);
                        recv(confd, &(status), sizeof (int), 0);
                        if (status == 200 ){
                            status = 1;
                            filename[strcspn(filename,"\n")]=0;
                            fileDesc[strcspn(fileDesc,"\n")]=0;
                            fprintf(fpServer, "%s\t%hu\t%u\t%s\t%s\n", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port),
                                    portPartage, filename, fileDesc);
                            send(confd, &(status), sizeof (int), 0);
                            fclose(fpServer);
                            printf("le fichier %s est bien partagé sur le serveur distant %s %hu.\n", filename,
                                   inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
                            break;
                        }
                        printf("Erreur de fichier status %d\n",status);
                        send(confd, &(status), sizeof (int), 0);
                        fclose(fpServer);
                    }

                    closedir(d);
                }


            }

        } else { // requete client rechercher un fichier ou telecharger un fichier partagé.
            pid = fork();
            int code;
            if (pid == 0) {
                //recherche/ou download
                recv(confd, &code, sizeof(int), 0);
                char key[50], f_name[50], f_desc[256];
                char cliIps[25];
                char result[1024];
                char results[4096];
                char line[4096];
                char *check;

                int port, port2, status = 0;
                while (1) {
                    if (code == 2) { // code = 2 pour la recherche
                        printf("Recherche d'un fichier\n");
                        bzero(results, 4096);
                        bzero(cliIps, 25);
                        recv(confd, key, 50, 0);
                        status = 0;
                        printf("%s en cours de recherche...\n", key);
                        fp = fopen("partage/file_list.txt", "r+");
                        if (!fp) {
                            printf("Erreur serveur de fichier introuvable\n");
                            return 1;
                        }
                        //check for the file in the list of files
                        while (fgets(line, sizeof(line), fp) != NULL) {
                            bzero(f_name, 50);
                            bzero(f_desc, 256);
                            bzero(result, 1024);
                            sscanf(line, "%s %d %d %s %[^\n]", cliIps, &port, &port2, f_name, f_desc);
                            check = strstr(f_name, key);
                            if (!check)
                                check = strstr(f_desc, key);
                            if (check) {
                                //If file name matches
                                status = 1;
                                sprintf(result, "%s \t %u \t %s \t %s\n", cliIps, port, f_name, f_desc);
                                strcat(results, result);
                            }
                        }
                        fclose(fp);
                        printf("stat = %d\n", status);
                        if (status != 1) {
                            //status = 0;
                            printf("Aucun résultat trouvé :%d\n", status);
                            send(confd, &status, sizeof(int), 0);
                        } else {
                            printf("un fichier trouvé\n");
                            send(confd, &status, sizeof(int), 0);
                            send(confd, results, sizeof(results), 0);
                        }
                    } else if (code == 3) {
                        //code = 3 pour le telechargement.
                        printf("Recherche d'un fichier\n");
                        bzero(cliIps, 25);
                        bzero(key, 50);
                        recv(confd, key, 50, 0);
                        status = 0;
                        printf("%s en cours de recherche...\n", key);
                        fp = fopen("partage/file_list.txt", "r+");
                        if (!fp) {
                            printf("Erreur serveur de fichier introuvable\n");
                            return 1;
                        }
                        //check for the file in the list of files
                        int founded = -1;
                        while (fgets(line, sizeof(line), fp) != NULL) {
                            bzero(f_name, 50);
                            bzero(f_desc, 256);
                            sscanf(line, "%s %d %d %s %[^\n]", cliIps, &port, &port2, f_name, f_desc);
                            founded = strcmp(f_name, key);
                            if (founded == 0) {
                                //If file name matches
                                status = 1;
                                break;
                            }
                        }
                        fclose(fp);
                        if (status == 1) {
                            printf("un fichier trouver .\n");
                            send(confd, &status, sizeof(int), 0);
                            send(confd, cliIps, 25, 0);
                            send(confd, &port2, sizeof(port2), 0);
                        } else {
                            //status = 0;
                            printf("Aucun résultat trouvé :%d\n", status);
                            send(confd, &status, sizeof(int), 0);
                        }
                    }
                    recv(confd, &code, sizeof(int), 0);
                }
            } else {

                close(confd);
            }
        }


    }
    close(socfd);
    return 0;
}



