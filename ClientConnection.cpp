//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"




ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;

    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
   
  
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) {
     // Implement your code to define a socket here

	struct sockaddr_in sin;
	struct hostent *hent;
	int s;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr=address;

		//DUDA!
		//errexit("No puedo resolver el nombre \ "%s \" \n", host);

		s = socket (AF_INET, SOCK_STREAM ,0);

		if (s < 0)
			errexit ("No se puede crear el socket: %s\n" , strerror(errno));

		if(connect (s, (struct sockaddr *) &sin, sizeof(sin)) < 0)
			errexit ("No se puede conectar con: %s\n", strerror(errno));


    return s; // You must return the socket descriptor.

}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {


 
      fscanf(fd, "%s", command);

//****************USER*******************//
        
      if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    if (strcmp(arg, "trolazo") == 0)
	    	fprintf(fd, "331 User name ok, need password\n");
	    else fprintf(fd, "530 Incorrect user\n");
      }

//****************PWD*******************//
        
      else if (COMMAND("PWD")) {
	   
    	char cwd[1024];
    	getcwd(cwd, sizeof(cwd));
    	fprintf(fd, "Current working dir: %s\n", cwd);

    	fprintf (fd, "257 PATHNAME created.");

      
      }
//****************PASS*******************//    
        else if (COMMAND("PASS")) {
	  fscanf(fd, "%s", arg);
	  if (strcmp(arg, "soy") == 0) {
	      fprintf(fd, "230 User logged in\n");
	    
	  }
	  else {
	      fprintf(fd, "530 Not logged in,\n");
	  }
//****************PORT*******************//	   
      }
      else if (COMMAND("PORT")) {
	int ip[4];
	int p[2];
	fscanf(fd, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &p[0], &p[1]);
	uint32_t address = ip[3] << 24 | ip[2] << 16 | ip[1] << 8 | ip[0];
	uint16_t port = p[0] << 8 | p[1];
	
	data_socket = connect_TCP(address, port);
	if (data_socket>=0) {
	    fprintf(fd, "200 ok.\n");
	}
	else {
	    fprintf(fd, "421 fail\n.");
	}

//****************PASV*******************//
      }
      else if (COMMAND("PASV")) {
	  	
        pasivo=true;
          
            struct sockaddr_in sin, sa;
            socklen_t sa_len = sizeof(sa);
            int s;

            s = socket(AF_INET, SOCK_STREAM, 0);

            if (s<0)
                errexit("No se ha podido crear el socket: %s\n", strerror(errno));

            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = server_address;
            sin.sin_port = 0;

            if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
                errexit("No se ha podido hacer bind con el puerto: %s\n", strerror(errno));

            if (listen(s,5) < 0)
                errexit("Fallo en listen: %s\n", strerror(errno));

            getsockname(s, (struct sockaddr *)&sa, &sa_len);

            fprintf(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n",
                (unsigned int)(server_address & 0xff),
                (unsigned int)((server_address >> 8) & 0xff),
                (unsigned int)((server_address >> 16) & 0xff),
                (unsigned int)((server_address >> 24) & 0xff),
                (unsigned int)(sa.sin_port & 0xff),
                (unsigned int)(sa.sin_port >> 8));

            data_socket = s;
				
      }
        
//****************CWD*******************//
        
      else if (COMMAND("CWD")) {
        char *path;
        fscanf(fd,"%s",arg);  
        
        if (getcwd(path, sizeof(path)) != NULL){
                strcat(path,"/");
                strcat(path,arg);
                if(chdir(path)<0)
                    fprintf(fd,"550. Failed to change the current working directory...\n");
                else    
	  	            fprintf(fd, "250. Working directory succesfully changed...\n");
        }
      }
//****************STOR*******************//    
    
      else if (COMMAND("STOR") ) {
	        fscanf(fd, "%s", arg);
            printf("STOR:%s\n", arg);

            FILE* file = fopen(arg,"wb");

            if (!file){
                fprintf(fd, "450 Unavaible to open file...\n");
                close(data_socket);
            }

            else{

                fprintf(fd, "150 File status okay; about to open data connection.\n");
                fflush(fd);

                struct sockaddr_in sa;
                socklen_t sa_len = sizeof(sa);
                char buffer[MAX_BUFF];
                int n;

                if (pasivo)
                    data_socket = accept(data_socket,(struct sockaddr *)&sa, &sa_len);

                do{
                    n = recv(data_socket, buffer, MAX_BUFF, 0);
                    fwrite(buffer, sizeof(char) , n, file);

                } while (n == MAX_BUFF);
            
                fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
                fclose(file);
                close(data_socket);
	       }
      }
       
//****************SYST*******************// 
        
      else if (COMMAND("SYST")) {
          #ifdef _WIN32
		      fprintf(fd, "This is a Windows Operating System\n");
          #endif
	      #ifdef __linux__
		      fprintf(fd, "This is a Linux Operating System\n");
	       #endif
      }
        
//****************TYPE*******************// 
        
      else if (COMMAND("TYPE")) {
	    fscanf(fd, "%s", arg);	
	    printf("Type:%s\n",arg);
          
        if (!strcmp(arg,"A"))
                fprintf(fd, "200 Switching to ASCII mode.\n");

            else if (!strcmp(arg,"B"))
                fprintf(fd, "200 Switching to Binary mode.\n");
            else
                fprintf(fd, "501 Syntax error in parameters or arguments.\n");
        }        
    
//****************RETR*******************// 
    
      else if (COMMAND("RETR")) {
	     
            fscanf(fd, "%s", arg);
            printf("RETR:%s\n", arg);

            FILE* file = fopen(arg,"rb");

            if (!file){
                fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
                close(data_socket);
            }

            else{

                fprintf(fd, "150 File status okay; about to open data connection.\n");

                struct sockaddr_in sa;
                socklen_t sa_len = sizeof(sa);
                char buffer[MAX_BUFF];
                int n;

                if (pasivo)
                    data_socket = accept(data_socket,(struct sockaddr *)&sa, &sa_len);

                do{
                    n = fread(buffer, sizeof(char), MAX_BUFF, file); 
                    send(data_socket, buffer, n, 0);

                } while (n == MAX_BUFF);
                          
                fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
                fclose(file);
                close(data_socket);
           }
        }    

//****************QUIT*******************// 
    
      else if (COMMAND("QUIT")) {
	
		fprintf(fd, "221. Service closing conection.\n Logged out if appropiate.\n");
	 	fflush (fd);
		stop();
      }

//****************LIST*******************// 

      else if (COMMAND("LIST")) {
		
  DIR *dir;
  struct dirent *ent;
  char buffer[1024];
  size_t sz;
  dir = opendir (".");

  if (dir == NULL){
    fprintf (fd, "421. SErvice nor avaliable, closing contrl conection\n");
	}else{
		fprintf (fd, "125 List started OK\n");
	}
	while(1){

		ent = readdir(dir);
		if(ent==NULL)
			break;

		if((strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, ".")!=0)){

			sz = sprintf(buffer, "%s\n", ent-> d_name);
			send(data_socket, buffer, strlen(buffer), 0);
		}
	}

	close (data_socket);
	closedir(dir);
	fprintf(fd, "250 List completed successfully\n");
    }
}
    
  
};
