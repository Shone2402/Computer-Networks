#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <time.h>



void receive_msg(int sockfd, char * msg)
{
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3)
     {printf("ERROR reading message from server socket.");
      printf("Either the server shut down or the other player disconnected.\nGame over.\n");
      exit(0);
     }
}

int receive_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
     {printf("ERROR reading int from server socket");
      printf("Either the server shut down or the other player disconnected.\nGame over.\n");
      exit(0);
     }
    
    return msg;
}

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        printf("ERROR writing int to server socket");
}


void print_board(char board[][3])
{   
    printf("-----------\n");
    for(int i=0;i<3;i++)
    {
      for(int j=0;j<3;j++)
      {
        printf(" %c |",board[i][j]);
       
      }
      printf("\n");
      printf("-----------\n");
    }
    
}

int take_turn(int sockfd)
{
    char buffer[10];
    
    while (1) { 
        printf("Enter 0-8 to make a move( 0-> (1,1) and 8->(3,3) ), or 9 for number of active players: ");
        unsigned int x_minutes=0;
        unsigned int x_hours=0;
	unsigned int x_seconds=0;
	unsigned int x_milliseconds=0;
        unsigned int totaltime=0,count_down_time_in_secs=0,time_left=0;
        clock_t x_startTime,x_countTime;
        count_down_time_in_secs=15;
        x_startTime=clock();  // start clock
        int count=1;
        time_left=count_down_time_in_secs-x_seconds;
        while (time_left>0) 
	{
		x_countTime=clock(); // update timer difference
		
		
		x_milliseconds=x_countTime-x_startTime;
		x_seconds=(x_milliseconds/(CLOCKS_PER_SEC))-(x_minutes*60);
		x_minutes=(x_milliseconds/(CLOCKS_PER_SEC))/60;
		x_hours=x_minutes/60;
		


	 

		time_left=count_down_time_in_secs-x_seconds; // subtract to get difference 
               fgets(buffer, 10, stdin);
		int move = buffer[0] - '0';
		printf("\n");
		if (move <= 9 && move >= 0){
		    write_server_int(sockfd, move);   
		    return 1;
		    
		} 
		else
		   { printf("Invalid input. Try again.\n");
		    break;
		    }

		
	}
	if(time_left<0)
	 return -1;
	return 1;
	
    }
}
 
void get_update(int sockfd, char board[][3])
{
    int player_id = receive_int(sockfd);
    int move = receive_int(sockfd);
    board[move/3][move%3] = player_id ? 'X' : 'O';    
}

void solve(char * msg,char * board[][3],int sockfd)
{

       int a=0,b=0,c=0;
       while(1) {
        
        receive_msg(sockfd, msg);
        if (!strcmp(msg, "TRN")) { 
	        printf("Your move...\n");
	        a=take_turn(sockfd);
	        if(a==-1)
	         {break;
	         close(sockfd);
	         }
        }
        else if (!strcmp(msg, "CNT")) { 
            int num_players = receive_int(sockfd);
            printf("There are currently %d active players.\n", num_players); 
        }
        else if (!strcmp(msg, "UPD")) { 
            get_update(sockfd, board);
            print_board(board);
        }
        else if(!strcmp(msg, "REP")){
          for(int i=0;i<3;i++)
               for(int j=0;j<3;j++)
                board[i][j]=' ';
           printf("Starting the game!\n");
           print_board(board);
        
        }
        else if (!strcmp(msg, "WAT")) { 
            printf("Waiting for other players move...\n");
        }
        else if (!strcmp(msg, "INV")) { 
            printf("That position has already been played. Try again.\n"); 
        }
        else if (!strcmp(msg, "WIN")) { 
            printf("You win!\n");
            printf("Press 1 to play again:");
            char buffer[10];
            fgets(buffer, 10, stdin);
	     a = buffer[0] - '0';
	    write_server_int(sockfd, a);  
            break;
        }
        else if (!strcmp(msg, "LSE")) { 
            printf("You lost.\n");
            printf("Press 1 to play again:");
            char buffer[10];
            fgets(buffer, 10, stdin);
	    b = buffer[0] - '0';
	    write_server_int(sockfd, b);
            break;
        }
        else if (!strcmp(msg, "DRW")) { 
            printf("Draw.\n");
            printf("Press 1 to play again:");
            char buffer[10];
            fgets(buffer, 10, stdin);
	    c = buffer[0] - '0';
	    write_server_int(sockfd, c);
            break;
        }
        else 
            printf("Unknown message.");

        }
        if(a==1 || b==1 || c==1)
         solve(msg,board,sockfd);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
       printf("Insufficient arguments\n");
       exit(0);
    }

    char *hostname=argv[1];
    int portno=atoi(argv[2]);
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    server = gethostbyname(hostname);
	
    if (server == NULL) {
        printf("No such host\n");
        exit(0);
    }
	
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    memmove(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        printf("ERROR connecting to server");

    printf("Connected to server.\n");
    int id = receive_int(sockfd);

    printf("Your player ID is %d. Your symbol is %c",id,id ? 'X' : 'O');
    

    char msg[4];
    char board[3][3] = { {' ', ' ', ' '}, 
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    printf("Tic-Tac-Toe\n------------\n");

    do {
        receive_msg(sockfd, msg);
        if (!strcmp(msg, "HLD"))
            printf("Waiting for a partner to join . . .\n");
    } while ( strcmp(msg, "SRT") );

    /* The game has begun. */
    printf("Starting the game!\n");
    

    print_board(board);

    
    solve(msg,board,sockfd);
        
    
    
    printf("Game over.\n");
    close(sockfd);
    return 0;
}
