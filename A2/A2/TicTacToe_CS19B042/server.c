#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<time.h>

int player_count = 0;
pthread_mutex_t mutexcount;


void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
}


void write_clients_msg(int * cli_sockfd, char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

void write_clients_int(int * cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
}

int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));
    if (n < 0 || n != sizeof(int))  return -1;
    return msg;
}

void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        printf("ERROR writing msg to client socket");
}


int get_player_move(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "TRN");
    return recv_int(cli_sockfd);
}

int check_move(char board[][3], int move, int player_id)
{
    if ((move == 9) || (board[move/3][move%3] == ' ')) 
        return 1;
    return 0;
}

void update_board(char board[][3], int move, int player_id)
{   int x=move/3;
    int y=move%3;
    board[x][y] = player_id ? 'X' : 'O';
}

void draw_board(char board[][3])
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

void send_update(int * cli_sockfd, int move, int player_id)
{
    write_clients_msg(cli_sockfd, "UPD");
    write_clients_int(cli_sockfd, player_id);
    write_clients_int(cli_sockfd, move);
   
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);
    printf("[DEBUG] Player Count Sent.\n");
    
}

int check_board(char board[][3], int last_move)
{
    int row = last_move/3;
    int col = last_move%3;

    if ( board[row][0] == board[row][1] && board[row][1] == board[row][2] ) 
       return 1;
    
    
    if ( board[0][col] == board[1][col] && board[1][col] == board[2][col] ) 
        return 1;
    
    
    if (!(last_move % 2)) { 
    
    if ( (last_move == 0 || last_move == 4 || last_move == 8) && (board[1][1] == board[0][0] && board[1][1] == board[2][2]) ) 
            return 1;
        
    
    if ( (last_move == 2 || last_move == 4 || last_move == 6) && (board[1][1] == board[0][2] && board[1][1] == board[2][0]) )  
         return 1;
        
    }
    return 0;
}

void *run_game(void *thread_data) 
{
    int *cli_sockfd = (int*)thread_data; 
    char board[3][3] = { {' ', ' ', ' '},  
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

   
    write_clients_msg(cli_sockfd, "SRT");
    
    

    draw_board(board);
    
    int prev_player_turn = 1;
    int player_turn = 0,game_over = 0,turn_count = 0;
    int checker=0;
    
     time_t t;   // not a primitive datatype
    time(&t);

    printf("\nGame started at (date and time): %s", ctime(&t));

    while(!game_over) {                                     //until game is over
        
        if (prev_player_turn != player_turn)
            write_client_msg(cli_sockfd[(player_turn + 1) % 2], "WAT");

        int valid = 0;
        int move = 0;
        while(!valid) {                           //check for valid move
            move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1) break; 
            
            valid = check_move(board, move, player_turn);
            if (!valid) 
                write_client_msg(cli_sockfd[player_turn], "INV");
            
        }

	 if (move == -1) { 
            printf("Player disconnected.\n");               
            break;
        }
        else if (move == 9) {
         
            prev_player_turn = player_turn;
            send_player_count(cli_sockfd[player_turn]);
        
        }
        else {
            
            update_board(board, move, player_turn);                 //update the board
            send_update( cli_sockfd, move, player_turn );
            draw_board(board);
            game_over = check_board(board, move);              //check if game is over
            
            if (game_over == 1) {                                              
                write_client_msg(cli_sockfd[player_turn], "WIN");
                write_client_msg(cli_sockfd[(player_turn + 1) % 2], "LSE");
                int a=recv_int(cli_sockfd[player_turn]);
                int b=recv_int(cli_sockfd[(player_turn+1)%2]);             //ask for replay
                if(a==1 && b==1)
                 checker=1;
                
            }
            else if (turn_count == 8) {                printf("Draw.\n");
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
                int a=recv_int(cli_sockfd[player_turn]);
                int b=recv_int(cli_sockfd[(player_turn+1)%2]);
                if(a==1 && b==1)
                 checker=1;
                
            }
            
            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
            
            if(checker==1)
            {
              
              time(&t);

              printf("\nGame ended at (date and time): %s", ctime(&t));
              game_over=0;
              for(int i=0;i<3;i++)
               for(int j=0;j<3;j++)
                board[i][j]=' ';
              write_clients_msg(cli_sockfd, "SRT");
             
              time(&t);

              printf("\nGame started at (date and time): %s", ctime(&t));
              draw_board(board);
    
              prev_player_turn = 1;
              player_turn = 0;game_over = 0;turn_count = 0;
              checker=0;
              write_clients_msg(cli_sockfd, "REP");
            }
            
            
        }
    }

   
     
              time(&t);

              printf("\nGame ended at (date and time): %s", ctime(&t));
    printf("Game over.\n");
   
    close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    player_count--;
    printf("Number of players is now %d.", player_count);
    pthread_mutex_unlock(&mutexcount);
    
    free(cli_sockfd);

    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{   
    if (argc < 2) {
        printf("Insufficient arguments");
        exit(1);
    }
    
    printf("$SERVER$ Game server started. Waiting for players\n");

    
    int sockfd;
    
    //setup the server socket
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) printf("Error opening the socket");
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;	
    	
    
    int port_number=atoi(argv[1]);
    server_addr.sin_port = htons(port_number);		
    pthread_mutex_init(&mutexcount, NULL);
    
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)   //bind the socket to created address
        printf("ERROR binding listener socket.");

   
    int lis_sockfd = sockfd;  //listen to clients
    
    


    while (1) {
              //loop for each game
        
            int *cli_sockfd = (int*)malloc(2*sizeof(int)); 
            memset(cli_sockfd, 0, 2*sizeof(int));
            
           
            
            socklen_t clilen;
	    struct sockaddr_in serv_addr, cli_addr;
	    clilen = sizeof(cli_addr);
	    
	    int connections = 0;
	    
	    while(connections < 2)              //only 2 players allowed
	    {
		listen(lis_sockfd, 253 - player_count);
		
		memset(&cli_addr, 0, sizeof(cli_addr));

		cli_sockfd[connections] = accept(lis_sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    
		
		write(cli_sockfd[connections], &connections, sizeof(int));
	       
		pthread_mutex_lock(&mutexcount);
		player_count=player_count+1;
		printf("Number of players is now %d.\n", player_count);
		pthread_mutex_unlock(&mutexcount);

		if (connections == 0) {
		    write_client_msg(cli_sockfd[0],"HLD");
		}

		connections=connections+1;
	    }
           
            
            pthread_t thread;
            int result = pthread_create(&thread, NULL, run_game, (void *)cli_sockfd);         //call run game using thread
            if (result) printf("Thread creation failed");
              
            
            
            
            
    }

    close(lis_sockfd);

    pthread_mutex_destroy(&mutexcount);
    pthread_exit(NULL);
    return 0;
}
