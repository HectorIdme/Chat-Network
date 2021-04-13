  /* Server code in C */
 //g++ server.cpp -o server.exe -lpthread
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <iostream>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <utility>

  #include <thread>
  #include <vector>

  using namespace std;
 
//vector<int> clients;
vector<pair<int,vector<string>>> clientes;

////////////////////////////////////////////
////////////////////////////////////////////

string getUser(int id){
  string user;
  for(int i=0;i<clientes.size();i++){
    if(clientes[i].first == id){
        user = clientes[i].second[0];
        break;
    }
  }
  return user;
}


int getSize(int s){
    string tam = "9";
    int res;
    for(int i=1;i<s;i++){tam += "9";}
    res = stoi(tam);
    return res;
}

int len(char* w){
    int count =0;
    while(*w != '\0'){
        count++;
        w++;
    }
    return count;
}

string formatNumbers(int range,int number){
    string numb = to_string(number);
    if(numb.size() < range){
        for(int i=1;i<range;i++){
            numb.insert(0,"0");
        }
    }
    return numb;
}

bool getDataFromPacket_Login(string msg,vector<string> &data){
  string u,pssw;
  string tamU_str,tamPSSW_str;
  int tam_u,tam_pssw;

  if(msg[0] != 'l'){return false;}

  msg.erase(0,1);

  tamU_str = msg.substr(0,2);
  tam_u = stoi(tamU_str);
  
  msg.erase(0,2);

  tamPSSW_str = msg.substr(0,2);
  tam_pssw = stoi(tamPSSW_str);
  
  msg.erase(0,2);

  u = msg.substr(0,tam_u);
  msg.erase(0,tam_u);

  pssw = msg.substr(0,tam_pssw);
  msg.erase(0,tam_pssw);

  data.push_back(u);
  data.push_back(pssw);

  if(msg.size() == 0){return true;}
  else{return false;}

}

struct list{
  char accion;
  char num_users[2];
  char tamanio_user_name[2];
  char* user_name;

  list(){
    accion = 'I';
  }

  string make_packet(){
    string packet = "";
    int num_clientes = clientes.size();
    size_t num_users_tam = sizeof(num_users)/sizeof(num_users[0]);
    string cant_clients = formatNumbers(num_users_tam,num_clientes);

    size_t tam_user_name;
    string tam_user_name_format;
    string user_names="";

    packet += accion;
    packet += cant_clients;

    for(int i=0;i<num_clientes;i++){
      tam_user_name = sizeof(tamanio_user_name)/sizeof(tamanio_user_name[0]);
      user_names += clientes[i].second[0];
      tam_user_name_format = formatNumbers(tam_user_name,clientes[i].second[0].size());
      packet += tam_user_name_format;
    }
    packet += user_names;

    return packet;
  }
};


struct okey{
  char accion;
  char ok[3];

  okey(){
    accion = 'L';
    strcpy(ok,"ok");
  }

  string make_packet(){
    string packet = "";

    packet += accion;
    packet += ok;

    return packet;
  }
};

struct error{
  char accion;
  char error_msg[20];
/*
  void set_msg(string msg){
    strcpy(error_msg,msg);
  }  */

  string make_packet(){
    string packet = " ";
    
    packet = accion;
    packet += error_msg;

    return packet;
  }
};

struct salir{
  char accion;

  salir(){
    accion = 'X';
  }

  char make_packet(){
    return accion;
  }

  void deleteSocket(int id){
    int pos=0;
    for(;pos<clientes.size();pos++){
      if(clientes[pos].first == id){break;}
    }

    //shutdown(id, SHUT_RDWR);
    //close(id);
    clientes.erase(clientes.begin()+pos);
  }
};


////////////////////////////////////////////
////////////////////////////////////////////



void Broadcast(int ConnectFD, string message) 
{
  //write(ConnectFD, message.c_str(),message.length());
  /*
  cout<<"Clients: ";
  for(int i=0;i<clients.size();i++){
    cout<<clients[i]<<",";
  }cout<<endl;

  cout<<"Clientes: ";
  for(int i=0;i<clientes.size();i++){
    cout<<"["<<clientes[i].first<<"-("<<clientes[i].second[0]<<"|"<<clientes[i].second[1]<<")] ,";
  }cout<<endl;
  */

  for (int i = 0; i < clientes.size(); i++) {
    if(clientes[i].first != ConnectFD){
      write(clientes[i].first, message.c_str(),message.length());
    }
  }
}


void Connection( int &SocketFD , int port)
{
    struct sockaddr_in stSockAddr;
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if(-1 == SocketFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }
    
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
   
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;
 
    if(-1 == bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("error bind failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    
    if(-1 == listen(SocketFD, 10))
    {
      perror("error listen failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
   
}


void listenClient(int ConnectFD)
{
    char buffer[256];
    int n;

    string message;

    bzero(buffer,256); 
    n = read(ConnectFD,buffer,255);
    if (n < 0) perror("ERROR reading from socket");

    vector<string> datos_login;
    bool validate_login= getDataFromPacket_Login(buffer,datos_login);

    if(validate_login){

      auto par = make_pair(ConnectFD,datos_login);
      clientes.push_back(par);
      
      okey k;
      string msg = k.make_packet();
      write(ConnectFD, msg.c_str(),msg.size());

      while(1)
      {

        bzero(buffer,256); 
        n = read(ConnectFD,buffer,255);
        if (n < 0){
           //perror("ERROR reading from socket");
           break;
        }
         string user = getUser(ConnectFD);

        if(strcmp(buffer, "i") == 0){
          list users;
          msg = users.make_packet();
          write(ConnectFD,msg.c_str(),msg.size());
        }

        else if(strcmp(buffer, "x") == 0){
          salir e;
          msg = e.make_packet();
          write(ConnectFD,msg.c_str(),msg.size());
          e.deleteSocket(ConnectFD);
          close(ConnectFD);
          if(clientes.size()){
            ConnectFD = clientes[0].first;
          }
        }

        else{
          //cout<<user<<": "<<buffer<<endl;
          //cout<<"Type a message: ";
          message= user + ": " + buffer;
          //getline(cin,message);
          Broadcast(ConnectFD, message);
        }
      }
    }
    
    /*
    cout<<"borrar server"<<endl;
    for(int i = 0; i < clientes.size(); ++i) 
    {
      if (clientes[i].first == ConnectFD) 
      {
        shutdown(ConnectFD, SHUT_RDWR);
        close(ConnectFD);
        clientes.erase(clientes.begin() + i);
      }
    }
    */
}

int main(void)
{
  struct sockaddr_in stSockAddr;
  int SocketFD;
  char buffer[256];
  int n;
  cout<<"Beginning the server"<<endl;
  Connection( SocketFD, 45000);
  
  cout<<"Connection initialized"<<endl;
  
  while(1)
  {
    int ConnectFD = accept(SocketFD, NULL, NULL);

    if(0 > ConnectFD)
    {
      perror("error accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    //clients.push_back(ConnectFD);

    thread (listenClient, ConnectFD).detach();

    
  }

  close(SocketFD);
  cout<<"Bye"<<endl;
  return 0;
}
