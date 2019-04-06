#include <array>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

namespace bp = boost::process;

using namespace std;
using namespace boost::asio;

extern char **environ;

io_service global_io_service;

class BindSession : public enable_shared_from_this<BindSession>{
  private:
    enum { max_length = 30000 };
    //ip::tcp::socket temp_socket;//send bind  socks reply
    ip::tcp::socket ftp_client_socket;
    ip::tcp::socket ftp_server_socket;
    
    std::string remote_host;
    std::string remote_port;
    
    std::vector<unsigned char> send2client;
    std::vector<unsigned char> send2server;

    ip::tcp::acceptor _acceptor;


  public:
    BindSession(ip::tcp::socket socket):
      _acceptor(global_io_service , ip::tcp::endpoint(ip::tcp::v4(),0)),
      ftp_client_socket(move(socket)),
      ftp_server_socket(global_io_service),
      send2client(max_length),
      send2server(max_length){
    }

    void start(){
      //send socks reply 
      send_socks_reply_to_client();
    }

    void do_read_from_client(){
      auto self(shared_from_this());   
     
      //printf("start do read from client\n"); 
   
      ftp_client_socket.async_read_some(boost::asio::buffer(send2server,max_length),
        [this,self](boost::system::error_code ec , std::size_t length){
          /*if(length==0){
	    ftp_server_socket.close(); 
	    ftp_client_socket.close();
    
	  }else */
	  if(!ec){       
           // printf("after read from client %lu!\n",length);
            do_write_to_server(length); 
            
           // printf("---------------bind----------------\n");

	   // for(int i=0;i<length;i++){
           //   printf("%c ",send2server[i]);
	   // }
	   // printf("\n");
           // printf("---------------end----------------\n");


          }else{
           // printf("do read from client error!\n");
            ftp_server_socket.close(); 
	    ftp_client_socket.close();
          }
        }
      ); 
    }

    void do_read_from_server(){
      auto self(shared_from_this());

      //printf("start read  from server\n");

      ftp_server_socket.async_read_some(boost::asio::buffer(send2client,max_length),
        [this,self](boost::system::error_code ec , std::size_t length){
	  /*if(length==0){
	    ftp_server_socket.close(); 
	    ftp_client_socket.close();
	  }else */
	  if(!ec){
           // printf("after read from server %lu\n",length);
	    do_write_to_client(length);

          //  printf("---------------bind----------------\n");
	  //  for(int i=0;i<length;i++){
           //   printf("%c ",send2client[i]);
	   // }
	   // printf("\n");
           // printf("---------------end----------------\n");


          }else{
           // printf("do read from server error!\n");
            ftp_server_socket.close(); 
	    ftp_client_socket.close();
          }
        }
      );

    }

    void do_write_to_client(std::size_t length){
      auto self(shared_from_this());
  
      //printf("start to write to client\n");
 
      /*ftp_client_socket.async_write_some(boost::asio::buffer(send2client,length),
        [this,self](boost::system::error_code ec , std::size_t length){
	  if(length==0){
            ftp_server_socket.close(); 
	    ftp_client_socket.close();
	  }else if(!ec){
	    do_read_from_server();
            printf("after write to client %lu\n",length);
            printf("------------bind send2client------------");

	    for(int i=0;i<length;i++){

	      printf("%c ",send2client[i]);

	    }
	    printf("\n");
            printf("------------bind send2client end------------");


          }else{
            printf("do write to client error!\n");
            ftp_server_socket.close(); 
	    ftp_client_socket.close();
          }
        }
      );*/
      boost::asio::async_write(ftp_client_socket,boost::asio::buffer(send2client,length),
        [this,self](boost::system::error_code ec , std::size_t length){
	  /*if(length==0){
            ftp_server_socket.close(),ftp_client_socket.close();
	    
	  }else */
	  if(!ec){
            do_read_from_server(); 
 	   // printf("------------bind send2client------------");

	    /*for(int i=0;i<length;i++){

	      printf("%c ",send2client[i]);

	    }
	    printf("\n");*/
           // printf("------------bind send2client end------------");


	  }else{
            ftp_server_socket.close(),ftp_client_socket.close();
	  }
	}		      
      );

    }

    void do_write_to_server(std::size_t length){
      auto self(shared_from_this());
    
      //printf("start to write to server\n");
 
      ftp_server_socket.async_write_some(boost::asio::buffer(send2server,length),
        [this,self](boost::system::error_code ec , std::size_t length){
	  if(length==0){
	    ftp_server_socket.close(); 
	    ftp_client_socket.close();
	  }else if(!ec){         
	    do_read_from_client();
           // printf("after write to server %lu\n",length);
          }else{
	    
           // printf("do write to server error!\n");
            ftp_server_socket.close(); 
	    ftp_client_socket.close();
          }
        }
      );
    }


    void do_accept(){
      auto self(shared_from_this());

     // printf("start to wait accept!\n");


      _acceptor.async_accept(ftp_server_socket , [this,self](boost::system::error_code ec){
        if(!ec){
          do_read_from_client(); 
          do_read_from_server();

         // printf("accept suc!!!!!!!!!!!!!!!!\n");

        }else{
         // printf("do second accept error!\n");
          do_accept();
        }

	//printf("port:%hu\n",_acceptor.local_endpoint().port());
     });

    }

    void send_socks_reply_to_client(){

      auto self(shared_from_this());

      send2client[0]=0;
      send2client[1]=90;
  
      unsigned short portnumber = _acceptor.local_endpoint().port();
      
      //printf("port number : %hu\n",portnumber); 

      send2client[2]=portnumber>>8;
      send2client[3]=portnumber%(1<<8);
      send2client[4]=0;
      send2client[5]=0;
      send2client[6]=0;
      send2client[7]=0;

      /*for(int i=0;i<8;i++){
        printf("%d : %u\n",i,send2client[i]);
      }*/


      ftp_client_socket.async_write_some(
        buffer(send2client,8),
        [this,self](boost::system::error_code ec , std::size_t length){      
          do_accept();
        }
      );
    }
};



//EchoSession
class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
   enum { max_length = 4000 };

   ip::tcp::socket out_socket;
   ip::tcp::resolver resolver;

   std::string remote_host;
   std::string remote_port;
 
   //原本是只有char  
   std::vector<unsigned char> in_buf;
   std::vector<unsigned char> out_buf;
   //array<unsigned char , max_length> in_buf;
   //array<unsigned char , max_length> out_buf;

 public:
   //注意，要給別人用的話，要放public
   ip::tcp::socket in_socket;
   //EchoSession(ip::tcp::socket socket) : _socket(move(socket)){}
   EchoSession(ip::tcp::socket socket) : in_socket(move(socket)),
                                        out_socket(global_io_service),
                                        resolver(global_io_service),
                                        in_buf(max_length),
                                        out_buf(max_length)
	                                {
     //printf("new a session\n");
   }

   //在這個echo server裡，
   void start() {
     
     read_socks_head();

   }

 private:
   void read_socks_head(){
     auto self(shared_from_this());

     //printf("start read socks head\n");

     in_socket.async_read_some(boost::asio::buffer(in_buf,max_length),
       [this,self](boost::system::error_code ec , std::size_t length)
       {
         if(!ec){
       
           //for(int i=0;i<length;i++){
           //  printf("%d : %u\n",i,in_buf[i]);
           //}

	   char tempstr[500];
           unsigned char VN = (unsigned char)in_buf[0];
           unsigned char CD = (unsigned char)in_buf[1];
           unsigned int DST_PORT= ((unsigned int)in_buf[2])<<8 | ((unsigned int)in_buf[3]);
           unsigned int DST_IP  = ((unsigned int)in_buf[4])<<24| ((unsigned int)in_buf[5])<< 16 | ((unsigned int)in_buf[6])<<8 | ((unsigned int)in_buf[7]);
           ip::tcp::endpoint remote_e = in_socket.remote_endpoint();
           string temp = remote_e.address().to_string();
           int len = temp.length(),index;
           for(index=0;index<len;index++){
             tempstr[index]=temp[index];
           }
           tempstr[index]='\x00';

           unsigned short portnumber = remote_e.port();

           printf("<S_IP>:%s\n",tempstr);

           printf("<S_PORT>:%hu\n",portnumber);
           printf("<D_IP>:%u.%u.%u.%u\n",(DST_IP>>24),((DST_IP%(1<<24))>>16),((DST_IP%(1<<16))>>8),((DST_IP%(1<<8))));
           printf("<D_PORT>:%u\n",DST_PORT);
           if(CD==1){
             printf("<Command>:CONNECT\n");
           }else if(CD==2){
             printf("<Command>:BIND\n");
           }


           //unsigned int DST_IP  = ((unsigned int)in_buf[4])<<24| ((unsigned int)in_buf[5])<< 16 | ((unsigned int)in_buf[6])<<8 | ((unsigned int)in_buf[7]);
           char ip_str[20];
	   memset(ip_str,0,20);
           sprintf(ip_str,"%u.%u.%u.%u",(DST_IP>>24),((DST_IP%(1<<24))>>16),((DST_IP%(1<<16))>>8),((DST_IP%(1<<8))));

           //get ip
	   //cout<<"ip address:"<<ip_str<<endl;
	   
           //printf("after read socks head from client\n");


           if(in_buf[1]==2){
             //check socks.conf (Bind mode)
             fstream file;
             file.open("socks.conf",ios::in); 
             char buffer[200];
             file.getline(buffer,sizeof(buffer));
             //printf("--------------buffer--------\n");

	     int pass=0;
             memset(buffer,0,200);
             while(file.getline(buffer,sizeof(buffer))){
               int index=0;
               // test whether tail is \x00
               while(1){
                 if(buffer[index]==0){
                   //printf("\n");
                   memset(buffer,0,200);
                   break;    
                 }else{
			 
	           //只需要看b，所以等到b出現再開始做事
                   if(buffer[index]=='b'){
                     index+=2;
                     int tindex=0;
                     
 		     while(1){
                       if((buffer[index]==0 && ip_str[tindex]!=0) || (buffer[index]!=0 && ip_str[tindex]==0)){
                         break;
		       }else if(buffer[index]=='*'){
                         pass=1;
			 break;
		       }else if(buffer[index]==ip_str[tindex] && buffer[index+1]==0){
                         pass=1;
			 break;
		       }else if(buffer[index]!=ip_str[tindex]){
                         break;
		       }

                       tindex++;
		       index++;
                     }
	           }

                   //printf("%c",buffer[index]);



                 }
                 index++;
               }
             }            
             //printf("--------------end--------\n");
  
             file.close();           

             if(pass){
               printf("<Reply>:Accept\n");

               //printf("---------------pass-------------\n");
              // printf("-------------make a Bind session----------\n");
               std::make_shared<BindSession>(move(in_socket))->start();
	     }else{
               in_buf[0]=0;
	       in_buf[1]=91;

               in_socket.async_write_some(
	         boost::asio::buffer(in_buf,8),
		 [this,self](boost::system::error_code ec , std::size_t length){
                   if(!ec){
		    // printf("after async write!\n");
		   }else{
		   }
	         }		       
	       );
	       
               printf("<Reply>:Reject\n");
               //printf("------------don't pass----------\n");
	     }

             
              
           }else{ 
             //check socks.conf (Connect mode)
             fstream file;
             file.open("socks.conf",ios::in); 
             char buffer[200];
             //printf("--------------buffer--------\n");

             int pass = 0;

             memset(buffer,0,200);
             while(file.getline(buffer,sizeof(buffer))){
               int index=0;
               // test whether tail is \x00
               while(1){
                 if(buffer[index]==0){
                   //printf("\n");
                   memset(buffer,0,200);
                   break;    
                 }else{
			 
	           //只需要看c，所以等到c出現再開始做事
                   if(buffer[index]=='c'){
                     index+=2;
                     int tindex=0;
                     
 		     while(1){
                       if((buffer[index]==0 && ip_str[tindex]!=0) || (buffer[index]!=0 && ip_str[tindex]==0)){
                         break;
		       }else if(buffer[index]=='*'){
                         pass=1;
			 break;
		       }else if(buffer[index]==ip_str[tindex] && buffer[index+1]==0){
                         pass=1;
			 break;
		       }else if(buffer[index]!=ip_str[tindex]){
                         break;
		       }

                       tindex++;
		       index++;
                     }
	           }

                   //printf("%c",buffer[index]);



                 }
                 index++;
               }
             }            
             //printf("--------------end--------\n");
  
             file.close();           

             if(pass){
               printf("<Reply>:Accept\n");
               //printf("---------------pass-------------\n");
               remote_host = boost::asio::ip::address_v4(ntohl(*((uint32_t*)&in_buf[4]))).to_string();
               remote_port = std::to_string(ntohs(*((uint16_t*)&in_buf[2])));
               //std::cout<<remote_host<<endl; 
               //std::cout<<remote_port<<endl;
               do_resolve();

	     }else{
               printf("<Reply>:Reject\n");
               in_buf[0]=0;
	       in_buf[1]=91;

               in_socket.async_write_some(
	         boost::asio::buffer(in_buf,8),
		 [this,self](boost::system::error_code ec , std::size_t length){
                   if(!ec){
		     //printf("after async write!\n");
		   }else{

		   }
	         }		       
	       );

               //printf("------------don't pass----------\n");
	     }
             //if(!pass) write socks reject reply

       
           }


         }else{
           //printf("read socks head error!\n");

         }    
 
       }
     );

   }   


   void write_socks_head(){
     auto self(shared_from_this());

     in_buf[0]=0;
     in_buf[1]=90;

     //printf("start write socks head\n");
 
     in_socket.async_write_some(boost::asio::buffer(in_buf,8),
       [this,self](boost::system::error_code ec, std::size_t length){
         if(!ec){
           for(int i=0;i<8;i++){
            // printf("%hhu\n",in_buf[i]);
           }
          // printf("size : %lu\n",in_buf.size());

           //printf("after async_write\n");   

           do_read_from_client();
           do_read_from_server();
           //do_resolve();
         }else{
          // printf("write socks head error!\n");
         } 
       }
     );

   }

   void do_resolve(){
     auto self(shared_from_this());

    // printf("start resolve\n");

     resolver.async_resolve(ip::tcp::resolver::query(remote_host,remote_port),
       [this,self](const boost::system::error_code& ec , ip::tcp::resolver::iterator it){
         if(!ec){
           do_connect(it);
           //printf("resolve suc!\n");
         }else{
          // printf("resolve error!\n");
         }
       }
     );

   }

   void do_connect(ip::tcp::resolver::iterator& it){
     auto self(shared_from_this());

     //printf("start connect!\n");

     out_socket.async_connect(*it,
       [this,self](const boost::system::error_code& ec){
         if(!ec){
           write_socks_head();
          // printf("connect suc!\n"); 

           //new bind session
            

         }else{
          // printf("do connect error!\n");
         }
       }
     );
   }

   void do_read_from_client(){
     auto self(shared_from_this());   
     
    // printf("start do read from client\n"); 
   
     in_socket.async_read_some(boost::asio::buffer(in_buf,max_length),
       [this,self](boost::system::error_code ec , std::size_t length){
         if(!ec){       
          // printf("after read from client %lu!\n",length);

          // printf("read from client:");
          
          /* for(int i=0;i<length;i++){
             printf("%c ",(char)in_buf[i]);
           }
           cout<<endl; */
 
           do_write_to_server(length); 
         }else{
          // printf("do read from client error!\n");
         }
       }
     ); 
   }

   void do_read_from_server(){
     auto self(shared_from_this());

     //printf("start read  from server\n");

     out_socket.async_read_some(boost::asio::buffer(out_buf,max_length),
       [this,self](boost::system::error_code ec , std::size_t length){
         if(!ec){
          // printf("after read from server %lu\n",length);
           /*for(int i=0;i<length;i++){
             printf("%c ",(char)out_buf[i]);
           }
           cout<<endl; */

           do_write_to_client(length);
         }else{
          // printf("do read from server error!\n");
         }
       }
     );

   }

   void do_write_to_client(std::size_t length){
     auto self(shared_from_this());
 
     //printf("start to write to client\n");
 
     in_socket.async_write_some(boost::asio::buffer(out_buf,length),
       [this,self](boost::system::error_code ec , std::size_t length){
         if(!ec){
          // printf("after write to client %lu\n",length);

           do_read_from_server();
         }else{
          // printf("do write to client error!\n");
         }
       }
     );
   }

   void do_write_to_server(std::size_t length){
     auto self(shared_from_this());
    
     //printf("start  to write to server\n");
 
     out_socket.async_write_some(boost::asio::buffer(in_buf,length),
       [this,self](boost::system::error_code ec , std::size_t length){
         if(!ec){         
           do_read_from_client();
           //printf("after write to server %lu\n",length);
         }else{
          // printf("do write to server error!\n");
         }
       }
     );


   }



};

class EchoServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;

 public:
  EchoServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service)
	{

    do_accept();
  }

 private:
  void do_accept() {

    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {

      if (!ec) make_shared<EchoSession>(move(_socket))->start();

      do_accept();

    });
  }
};


int main(int argc, char* const argv[]) {
  if (argc != 2) {
    std::cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }
  

  try {
    short port = atoi(argv[1]);
    EchoServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
