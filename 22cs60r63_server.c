#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#define PORT 8082
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

typedef struct {
    int id;
    int socket_fd;
    char name[100];
    struct sockaddr_in address;
    int addrlen;
    int isAdmin; //no-> 0 ..............yes->1
} client_t;

struct requestInfo {
    int Gid;
    int totCli;
    int pos_ptr;
    int neg_ptr;
    client_t admin;
    client_t members[5];
    client_t posMem[5];
    client_t negMem[5];
};

struct groupInfo {
    int Gid;
    int ids[50];
    int admins[50];
    int admin_ptr;
    client_t members[50];
    int num_members;
    int isActive; //no-> 0 ..............yes->1
    int isBroadcast;
};

// struct RequestGroup {
//     struct requestInfo req[50];
// };

int generateId(int lower,int upper) {
    return (rand()%(upper - lower + 1)) + lower;
}

struct requestInfo requestedCli[50];
int num_of_groups = 0;
int Groupidx = 0;
int ReqIdx = 0;

int getClient_id(client_t clients[],int client_fds[],int i) {
    int reqID=-1;
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(client_fds[i]==clients[j].socket_fd) {
            reqID = clients[j].id;
            // temp = clients[j];
            break;
        }
    }
    return reqID;
}

client_t getClient(client_t clients[],int client_fds[],int i) {
    int reqID=-1;
    client_t temp;
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(client_fds[i]==clients[j].socket_fd) {
            reqID = clients[j].id;
            temp = clients[j];
            break;
        }
    }
    return temp;
}

void activeGroups(int client_fds[],struct groupInfo Groups[],int i) {
    char m3[1024];
    int adminFD = client_fds[i];
    int j = 0;
    while(j<Groupidx) {
        if(Groups[j].num_members > 0 && Groups[j].num_members < 5) {
            for(int k=0;k<Groups[j].num_members;k++) {
                if(Groups[j].members[k].socket_fd == adminFD) {
                    sprintf(m3,"Group-%d\n",Groups[j].Gid);
                    send(client_fds[i],m3,strlen(m3),0);
                    bzero(m3,BUFFER_SIZE);
                }
            }
        }
        j++;
    }
}

void sendGroup(client_t clients[],int client_fds[],char buffer[],struct groupInfo Groups[],int i) {
    char m4[1024];
    char *Gid = strtok(buffer+11," ");
    char *message = strtok(NULL," ");
    char senderSname[50];
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(clients[j].socket_fd == client_fds[i]) {
            strcpy(senderSname,clients[j].name);
            break;
        }
    }
    int grId = atoi(Gid);
    for(int j=0;j<Groupidx;j++) {
        if(Groups[j].Gid==grId) {
            if(Groups[j].isBroadcast==0) {
                for(int k=0;k<Groups[j].num_members;k++) {
                    //if(client_fds[i]==Groups[j].members[k].socket_fd) continue;
                    sprintf(m4,"[%s]: %s",senderSname,message);
                    send(Groups[j].members[k].socket_fd,m4,strlen(m4),0);
                    bzero(m4,BUFFER_SIZE);
                }
            }
            else {
                send(client_fds[i],"This is Broadcast Group and Only admins can send",256,0);
                break;
            }
        }

    }
}

void active(char buffer[],int client_fds[],client_t clients[],int i) {
    sprintf(buffer, "Active clients:\n");
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_fds[j] != -1) {
            sprintf(buffer + strlen(buffer), "%s\n", clients[j].name);
        }
    }
    send(client_fds[i], buffer, strlen(buffer), 0);
}

void sendMessage(char buffer[],client_t clients[],int client_fds[],int i) {
    char buf[BUFFER_SIZE];
    char msg[BUFFER_SIZE];
    strcpy(msg,buffer);
    //bzero(buf,0);
    char *dest_id_str = strtok(buffer + 6, " ");
    char *message = msg+12;
    printf("%s\n",message);
    if (dest_id_str != NULL && message != NULL) {
        int dest_id = atoi(dest_id_str);
        int dest_fd = 0;
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients[j].id == dest_id) {
                dest_fd = clients[j].socket_fd;
                break;
            }
        }
        bzero(buf,BUFFER_SIZE);
        if (dest_fd != 0) {
            sprintf(buf, "%s : %s",clients[i].name,message);
            printf("%s",buf);
            send(dest_fd, buf, BUFFER_SIZE, 0);
        }
        else {
            sprintf(buf, "Client %d not found\n", dest_id);
            send(client_fds[i], buf, BUFFER_SIZE, 0);
        }
        bzero(buffer,BUFFER_SIZE);
    }
}

void BroadcastMsg(client_t clients[],int client_fds[],char buffer[],int i) {
    // broadcast message to all clients
    char m1[BUFFER_SIZE];
    bzero(m1,BUFFER_SIZE);
    sprintf(m1, "[%s]: %s", clients[i].name, buffer + 11);
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_fds[j] != -1 && j != i) {
            send(client_fds[j], m1, BUFFER_SIZE, 0);
            //memset(buffer,0,BUFFER_SIZE);
        }
    }
}

void makegroupreq(int client_fds[],client_t clients[],char buffer[],struct groupInfo Groups[],int i,struct requestInfo requestedCli[]) {
    requestedCli[ReqIdx].totCli=1;
    client_t temp;
    int reqID;
    int genId = generateId(10000,99999);
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(client_fds[i]==clients[j].socket_fd) {
            reqID = clients[j].id;
            temp = clients[j];
            break;
        }
    }
    char m3[1024];
    sprintf(m3,"You are now Temporary Admin and Member of Group-%d",genId);
    send(temp.socket_fd,m3,1024,0);
    int p=1;
    char *id = strtok(buffer+14," ");
    while(id!=NULL) {
        if(atoi(id)==reqID) {
            requestedCli[ReqIdx].members[0] = temp;
            id = strtok(NULL," ");
        }
        else {
            char m1[1024];
            sprintf(m1,"Do you want to be added in Group-%d, Please respond with **/joingroup <groupId>** OR **/declinegroup <groupId>**",genId);
            for(int j=0;j<MAX_CLIENTS;j++) {
                if(atoi(id)==clients[j].id) {
                    requestedCli[ReqIdx].members[p] = clients[j];
                    p++;
                    send(clients[j].socket_fd,m1,1024,0);
                    break;
                }
            }
            requestedCli[ReqIdx].totCli++;
            id = strtok(NULL," ");
        }
        
    }
    requestedCli[ReqIdx].pos_ptr = 1;
    requestedCli[ReqIdx].neg_ptr = 0;
    requestedCli[ReqIdx].Gid = genId;
    requestedCli[ReqIdx].admin = temp;
    ReqIdx++;
}

bool check(struct requestInfo requestedCli[],client_t clients[],int idx,int i) {
    if((requestedCli[idx].pos_ptr + requestedCli[idx].neg_ptr) == requestedCli[idx].totCli) {
        return true;
    }
    return false;
}

bool joingroup(int client_fds[],client_t clients[],char buffer[],struct groupInfo Groups[],int i,struct requestInfo requestedCli[]) {
    char *id = strtok(buffer+11," ");
    client_t temp;
    int gid = atoi(id);
    int idx=-1;
    int cid = getClient_id(clients,client_fds,i);
    for(int j=0;j<ReqIdx;j++) {
        if(requestedCli[j].Gid == gid) {
            idx = j;
            requestedCli[j].members[requestedCli[j].pos_ptr] = getClient(clients,client_fds,i);
            temp = getClient(clients,client_fds,i);
            requestedCli[j].pos_ptr++;
            break;
        }
    }
    // if(idx==-1) send(temp.socket_fd,"Invalid Group ID",100,0);
    
    if(check(requestedCli,clients,idx,i)) {
        return true;
    }
    return false;

}


bool declinegroup(int client_fds[],client_t clients[],char buffer[],struct groupInfo Groups[],int i,struct requestInfo requestedCli[]) {
    char *id = strtok(buffer+14," ");
    int gid = atoi(id);
    int idx=-1;
    client_t temp;
    int cid = getClient_id(clients,client_fds,i);
    for(int j=0;j<ReqIdx;j++) {
        if(requestedCli[j].Gid == gid) {
            idx = j;
            requestedCli[j].members[requestedCli[j].neg_ptr] = getClient(clients,client_fds,i);
            requestedCli[j].neg_ptr++;
            break;
        }
    }
    // if(idx==-1) send(temp.socket_fd,"Invalid Group ID",100,0);
    
    if(check(requestedCli,clients,idx,i)) {
        return true;
    }
    return false;

}

void send_to_all_requested_clients(struct requestInfo requestedCli[],struct groupInfo Groups[],int client_fds[],client_t clients[],int i,int gid) {
    int idx = -1;
    for(int j=0;j<ReqIdx;j++) {
        if(requestedCli[j].Gid == gid) {
            idx = j;
            break;
        }
    }
    char m1[1024];
    sprintf(m1,"You have been added to Group-%d",gid);
    if(requestedCli[idx].pos_ptr >= requestedCli[idx].neg_ptr) {
        Groups[Groupidx].admins[Groups[Groupidx].admin_ptr] = requestedCli[idx].admin.id;
        Groups[Groupidx].members[0] = requestedCli[idx].admin;
        Groups[Groupidx].num_members=1;
        for(int j=0;j<requestedCli[idx].pos_ptr;j++) {
            Groups[Groupidx].members[Groups[Groupidx].num_members]=requestedCli[idx].posMem[j];
            // Groups[Groupidx].num_members++;
            send(Groups[Groupidx].members[Groups[Groupidx].num_members].socket_fd,m1,1024,0);
            Groups[Groupidx].num_members++;
        }
        Groupidx++;
    }
    else {
        char m2[1024];
        sprintf(m2,"Group-%d not created because of lack of members agreement",gid);
        for(int j=0;requestedCli[idx].totCli;j++) {
            send(requestedCli[idx].members[j].socket_fd,m2,1024,0);
        }
    }
}


void makeGroupBroadcast(int client_fds[],client_t clients[],char buffer[],int i,struct groupInfo Groups[]) {
    int reqID;
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(client_fds[i]==clients[j].socket_fd) reqID = clients[j].id;
    }
    int isad = 0;
    char *G = strtok(buffer+20," ");
    int Gi = atoi(G);
    for(int j=0;j<Groupidx;j++) {
        if(Groups[j].Gid == Gi) {
            for(int k=0;k<Groups[j].num_members;k++){
                if(reqID==Groups[j].admins[k]) isad = 1;
            }
            if(isad==0) {
                char m9[100];
                sprintf(m9,"You are not admin of Group-%d",Gi);
                send(client_fds[i],m9,100,0);
            }
            else {
                Groups[j].isBroadcast=1;
                char m9[100];
                sprintf(m9,"Group-%d is now Broadcast Group",Gi);
                send(client_fds[i],m9,100,0);
            }
        }
    }
    if(isad==0) send(client_fds[i],"Invalid Group ID",16,0);
        
}

void makeGroup(char buffer[],client_t clients[],int client_fds[],int i,struct groupInfo Groups[]) {
    char idsofReq[1024];
    num_of_groups++;
    int adminset = 0;
    //struct groupInfo *group = (struct groupInfo*)malloc(sizeof(struct groupInfo));
    char *tok = strtok(buffer+11," ");
    int k = 0;
    Groups[Groupidx].num_members = 0;
    Groups[Groupidx].isBroadcast = 0;
    Groups[Groupidx].Gid = generateId(10000,99999);
    for(int j=0;j<5;j++) Groups[Groupidx].admins[j] = -1;
    Groups[Groupidx].admins[k] = atoi(tok);
    Groups[Groupidx].admin_ptr=1;
    while(tok!=NULL) {
        //printf("%s\t",tok);
        strcat(idsofReq,tok);
        strcat(idsofReq," ");
        int id = atoi(tok);
        Groups[Groupidx].ids[k] = id;
        Groups[Groupidx].num_members++;

        for(int j =0;j<MAX_CLIENTS;j++) {
            if(id==clients[j].id) {
                if(adminset==0) {
                    clients[j].isAdmin = 1;
                    adminset=1;

                }
                Groups[Groupidx].members[k] = clients[j];
                //printf("%s\t",Groups[Groupidx].members[k].name);
            }
            else continue;
        }
        k++;
        //Groups[Groupidx] = group;
        tok = strtok(NULL, " ");
    }
    char m2[1024];
    for(int j=0;j<Groups[Groupidx].num_members;j++) {
        for(int p=0;p<MAX_CLIENTS;p++) {
            if(Groups[Groupidx].members[j].id == clients[p].id) {
                sprintf(m2,"You have been added to Group-%d",Groups[Groupidx].Gid);
                send(clients[p].socket_fd,m2,BUFFER_SIZE,0);
                bzero(m2,BUFFER_SIZE);
                break;
            }
        }
    }
    Groupidx++;
    bzero(buffer,BUFFER_SIZE);
    // sprintf(buffer,"Made group of %s",idsofReq);
    // send(client_fds[i],buffer,strlen(buffer),0);
}

void AddtoGroup(char buffer[],int client_fds[],client_t clients[],struct groupInfo Groups[],int i) {
    char m7[1024];
    strcpy(m7,buffer);
    char *tok = strtok(buffer+12," ");
    int Gid = atoi(tok);
    int index;
    int clientID = -1;
    // int p;
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(client_fds[i]==clients[j].socket_fd){
            clientID = clients[j].id;
            break;
        }
    }
    int adminsPr = 0;
    for(int j=0;j<Groupidx;j++) {
        for(int k=0;k<Groups[j].num_members;k++) {
            if(Gid==Groups[j].Gid && Groups[j].admins[k]==clientID) {
                index = j;
                adminsPr = 1;
                break;
            }
        }
    }
    if(adminsPr==0) {
        send(client_fds[i],"You are not admin",22,0);
    }
    else if(Groups[index].num_members > 5) {
        send(client_fds[i],"Member limit Exceeded",21,0);

    }
    else {
        for(int j=0;j<Groupidx;j++) {
            if(Gid==Groups[j].Gid) {
                char *cid = strtok(NULL," ");
                int cintd = atoi(cid);
                for(int k=0;k<MAX_CLIENTS;k++) {
                    if(cintd==clients[k].id) {
                        Groups[j].members[Groups[j].num_members] = clients[k];
                        char m8[100];
                        sprintf(m8,"You have been added to Group-%d",Gid);
                        send(clients[k].socket_fd,m8,100,0);
                        Groups[j].num_members++;

                    }
                }
            }
        }
    }
}

void RemoveFromGroup(char buffer[],int client_fds[],client_t clients[],struct groupInfo Groups[],int i) {
    int reqID;
    for(int j=0;j<MAX_CLIENTS;j++) {
        if(client_fds[i]==clients[j].socket_fd) reqID = clients[j].id;
    }
    
    char *gi = strtok(buffer+17," ");
    int g = atoi(gi);
    int idx;
    for(idx=0;idx<Groupidx;idx++) if(g==Groups[idx].Gid) break;
    int isAdmin = 0;
    for(int p=0;p<Groups[idx].admin_ptr;p++) if(Groups[idx].admins[p] == reqID) isAdmin = 1;

    if(isAdmin==0) {
        send(client_fds[i],"You are not admin",22,0);
    }
    else{
        char *iD = strtok(NULL," ");
        while(iD!=NULL) {
            for(int k=0;k<Groups[idx].num_members;k++) {
                if(Groups[idx].members[k].id == atoi(iD)){
                    for(int j=0;j<Groups[idx].admin_ptr;j++) if(Groups[idx].admins[j]==atoi(iD)) Groups[idx].admins[j] = 0;
                    for(int j=k+1;j<Groups[idx].num_members;j++) Groups[idx].members[j-1] = Groups[idx].members[j];
                    //Groups[idx].members[Groups[idx].num_members-1] = NULL;
                    Groups[idx].num_members--;
                    send(Groups[idx].members[k].socket_fd,"You have been Removed from the group",100,0);
                }
            }
            iD = strtok(NULL," ");
        }
    }
}


void Quit(client_t clients[],int client_fds[],struct sockaddr_in address,int addrlen,char buffer[],int i) {
    send(client_fds[i],"CONNECTION TERMINATED",21,0);
    getpeername(client_fds[i], (struct sockaddr*)&address, (socklen_t*)&addrlen);
    printf("%s disconnected\n", clients[i].name);
                        // remove client from client list and file descriptor set
    close(client_fds[i]);
    client_fds[i] = 0;
    // send notification to remaining clients
    char m2[1024];
    sprintf(m2, "%s has left the chat\n", clients[i].name);
    memset(&clients[i], 0, sizeof(client_t));
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_fds[j] != -1 && j != i) {
            send(client_fds[j], m2, 1024, 0);
        }
    }
}

int main() {
    srand(time(0));
    int lower=10000,upper=99999;
    struct groupInfo Groups[50]; //maximum 50 groups
    //struct requestInfo Request[50];
    //struct requestGroup ReqGroups[50]; //maximum 50 groups
    int server_fd, client_fds[MAX_CLIENTS], max_fd, activity, i, valread, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    client_t clients[MAX_CLIENTS];
    int client_count = 0;
    
    // create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // bind server socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // listen for client connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    // initialize client file descriptors
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    while (1) {

        // clear the set of file descriptors
        FD_ZERO(&readfds);
        
        // add server socket to the set
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;
        
        // add client sockets to the set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &readfds);
                if (client_fds[i] > max_fd) {
                    max_fd = client_fds[i];
                }
            }
        }
        
        // wait for activity on any of the file descriptors
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }
        
        // handle incoming client connections
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            char m10[1024];
            //printf("%d\n",new_socket);
            // add new client to the client list
            clients[client_count].id = generateId(lower,upper);
            clients[client_count].socket_fd = new_socket;
            clients[client_count].address = address;
            clients[client_count].addrlen = addrlen;
            //sprintf(m10,"Your Unique Id is %d",clients[client_count].id)
            sprintf(clients[client_count].name, "Client-%d", clients[client_count].id);
            printf("%s connected\n", clients[client_count].name);
                    // send welcome message to client
            sprintf(buffer, "Welcome to the chat, %s\n", clients[client_count].name);
            send(new_socket, buffer,1024, 0);
        
        // add new client socket to file descriptor set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] == -1) {
                client_fds[i] = new_socket;
                break;
            }
        }
        
        client_count++;
    }
    
    // handle incoming messages from clients
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (FD_ISSET(client_fds[i], &readfds)) {
            valread = recv(client_fds[i], buffer, BUFFER_SIZE,0);
            //printf("%s\n",buffer);
            if (valread <= 0) {
                // client disconnected
                getpeername(client_fds[i], (struct sockaddr *)&address, (socklen_t*)&addrlen);
                printf("%s disconnected\n", clients[i].name);
                
                // remove client from client list and file descriptor set
                close(client_fds[i]);
                client_fds[i] = -1;
                memset(&clients[i], 0, sizeof(client_t));
                
                // send notification to remaining clients
                sprintf(buffer, "%s has left the chat\n", clients[i].name);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_fds[j] != -1 && j != i) {
                        send(client_fds[j], buffer, strlen(buffer), 0);
                    }
                }
            } else {
                // process incoming message
                char msg[BUFFER_SIZE];
                strcpy(msg,buffer);
                buffer[valread] = '\0';

                if(strncmp(buffer, "/activegroups",13) == 0) {
                    activeGroups(client_fds,Groups,i);

                }

                else if (strncmp(buffer, "/sendgroup", 10) == 0) {
                    sendGroup(clients,client_fds,buffer,Groups,i);
                }

                else if (strncmp(buffer, "/active", 7) == 0) {
                    // list all active clients
                    active(buffer,client_fds,clients,i);
                } 
                
                else if (strncmp(buffer, "/send", 5) == 0) {
                    // send message to specific client
                    sendMessage(buffer,clients,client_fds,i);

                } 
                
                else if (strncmp(buffer, "/broadcast ", 11) == 0) {
                    // broadcast message to all clients
                    BroadcastMsg(clients,client_fds,buffer,i);
                } 
                
                else if(strncmp(buffer,"/makegroupreq",13)==0) {
                    makegroupreq(client_fds,clients,buffer,Groups,i,requestedCli);
                    send(client_fds[i],"All the members have been Notified",100,0);
                }

                else if(strncmp(buffer,"/joingroup",10)==0) {
                    char m1[1024];
                    strcpy(m1,buffer);
                    char *tok = strtok(m1+11," ");
                    int gid = atoi(tok);
                    if(joingroup(client_fds,clients,buffer,Groups,i,requestedCli)) {
                        send_to_all_requested_clients(requestedCli,Groups,client_fds,clients,i,gid);
                        // send(client_fds[i],"Successfully executed",50,0);
                    }
                }
                
                else if(strncmp(buffer,"/declinegroup",13)==0) {
                    char m1[1024],m2[100];
                    strcpy(m1,buffer);
                    char *tok = strtok(m1+14," ");
                    int gid = atoi(tok);
                    sprintf(m2,"You are not added to Group-%d",gid);
                    if(declinegroup(client_fds,clients,buffer,Groups,i,requestedCli)) {
                        send_to_all_requested_clients(requestedCli,Groups,client_fds,clients,i,gid);
                        // send(client_fds[i],m2,100,0);
                    }
                }

                else if(strncmp(buffer,"/makegroupbroadcast",19)==0) {
                    makeGroupBroadcast(client_fds,clients,buffer,i,Groups);
                        
                }

                else if(strncmp(buffer,"/makegroup",10)==0) {
                    
                    makeGroup(buffer,clients,client_fds,i,Groups);
                }
                else if(strncmp(buffer,"/addtogroup",11)==0){
                    AddtoGroup(buffer,client_fds,clients,Groups,i);
                    
                }
                
                else if(strncmp(buffer,"/removefromgroup",16)==0) {
                    RemoveFromGroup(buffer,client_fds,clients,Groups,i);
                }

                else if(strncmp(buffer,"/printreq",9)==0) {
                    for(int j=0;j<ReqIdx;j++)   {
                        printf("%d\n",requestedCli[j].Gid);
                    }
                }

                else if(strncmp(buffer,"/makeadmin",10)==0) {
                    int reqID;
                    for(int j=0;j<MAX_CLIENTS;j++) {
                        if(client_fds[i]==clients[j].socket_fd) reqID = clients[j].id;
                    }
                    char *gID = strtok(buffer+11," ");
                    char *cliID = strtok(NULL," ");
                    int GiD = atoi(gID);
                    int CLID = atoi(cliID);
                    bool isAdmin = false;
                    for(int j=0;j<Groupidx;j++) {
                        if(Groups[j].Gid==GiD) {
                            for(int p=0;p<Groups[j].num_members;p++) {
                                if(Groups[j].admins[p]==reqID) isAdmin=true;
                            }
                            if(isAdmin==false) {
                                send(client_fds[i],"Only admins can call this command",100,0);
                                break;
                            }
                            for(int k=0;k<MAX_CLIENTS;k++) {
                                if(CLID==clients[k].id) {
                                    Groups[j].admins[Groups[j].admin_ptr] = CLID;
                                    Groups[j].admin_ptr++;
                                    char m9[100];
                                    sprintf(m9,"You are now admin of Group-%d",GiD);
                                    send(clients[k].socket_fd,m9,100,0);
                                    send(client_fds[i],"Desired member is now Admin",100,0);
                                    break;
                                }
                            }
                        }
                    }send(client_fds[i],"CONNECTION TERMINATED",21,0);
                    getpeername(client_fds[i], (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("%s disconnected\n", clients[i].name);
                                        // remove client from client list and file descriptor set
                    close(client_fds[i]);
                    client_fds[i] = 0;
                    memset(&clients[i], 0, sizeof(client_t));
                    
                    // send notification to remaining clients
                    sprintf(buffer, "%s has left the chat\n", clients[i].name);
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] != -1 && j != i) {
                            send(client_fds[j], buffer, strlen(buffer), 0);
                        }
                    }
                }
                else if(strncmp(buffer,"/admins",7)==0) {
                    char *gid = strtok(buffer+8," ");
                    int j;
                    if(gid!=NULL) {
                        for(j=0;j<Groupidx;j++) {
                            if(Groups[j].Gid==atoi(gid)) break;
                        }
                        for(int k=0;k<Groups[j].admin_ptr;k++) {
                            char m9[100];
                            sprintf(m9,"Client-%d",Groups[j].admins[k]);
                            send(client_fds[i],m9,100,0);
                        }
                    }
                    else {
                        send(client_fds[i],"Please add Group-id",100,0);
                    }
                }

                else if (strncmp(buffer, "/quit", 5) == 0) {
                    // client quits
                    Quit(clients,client_fds,address,addrlen,buffer,i);
                } else {
                    // invalid command
                    bzero(buffer,BUFFER_SIZE);
                    strcat(buffer,"Invalid Command : ");
                    strcat(buffer,msg);
                    //sprintf(msg, "Invalid command: %s", buffer);
                    send(client_fds[i], buffer, strlen(buffer), 0);
                }
            }
        }
    }
}

return 0;
}
