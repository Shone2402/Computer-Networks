#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>


#define PACKET_SIZE     4096
#define PING_SLEEP_RATE 1000000 
#define MAX_NO_PACKETS  10
#define RECV_TIMEOUT 1
#define MAX_WAIT_TIME 5
#define PING_PKT_S 64

int pingloop=1;
char sendpacket[PACKET_SIZE],recvpacket[PACKET_SIZE];
int sockfd, datalen = 56;

int nsend = 0, nreceived = 0;
struct sockaddr_in dest_addr;
pid_t pid;
struct sockaddr_in from;
struct timeval tvrecv;

void intHandler(int dummy)
{pingloop=0;}

unsigned short cal_chksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum +=  *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

int unpack(char *buf, int len,long double rtt_msec)
{
    int i, iphdrlen;
    struct ip *ip;
    struct icmp *icmp;

    ip = (struct ip*)buf;
    iphdrlen = ip->ip_hl << 2; 
    icmp = (struct icmp*)(buf + iphdrlen);
    len -= iphdrlen; 
    if (len < 8) return -1;                                  //check for valid length

    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid))
        printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3Lf ms\n", len,inet_ntoa(from.sin_addr), icmp->icmp_seq, ip->ip_ttl, rtt_msec);
    else
      return -1;
    return 0;
}

int send_packet()
{
    int packetsize;
    struct timespec time_start, time_end, tfs, tfe;   //to store start and end times
    long double rtt_msec=0, total_msec=0;
    struct timeval tv_out;
    int n, fromlen;
    fromlen = sizeof(from);
    tv_out.tv_sec = RECV_TIMEOUT;
    tv_out.tv_usec = 0;
    clock_gettime(CLOCK_MONOTONIC, &tfs);           //start the timer
    
    while (pingloop && nsend<MAX_NO_PACKETS)           //pingloop is used to detect an interrupt
    {
        nsend++;
        packetsize = datalen+8;                       //packet size is 64
        struct icmp *icmp;
        struct timeval *tval;
        icmp = (struct icmp*)sendpacket;
        icmp->icmp_type = ICMP_ECHO;                    //ICMP_ECHO is used as the type
        icmp->icmp_seq = nsend;                          //packet number
        icmp->icmp_id = pid;
        icmp->icmp_code = 0;icmp->icmp_cksum = 0;
        tval = (struct timeval*)icmp->icmp_data;gettimeofday(tval, NULL); 
        icmp->icmp_cksum = cal_chksum((unsigned short*)icmp, packetsize);  //calculate the checksum  
        usleep(PING_SLEEP_RATE);
        
	clock_gettime(CLOCK_MONOTONIC, &time_start);     //timer for individual packet starts
        
        if (sendto(sockfd, sendpacket, packetsize, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0)
        {
            perror("Request timed out or host unreacheable");
            continue;
        } 
    
        alarm(MAX_WAIT_TIME);                         //wait for some time to get response from receiver

        if ((n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr*) &from, &fromlen)) < 0)
        {
            perror("Request timed out or host unreacheable");
            continue;
        } 
        gettimeofday(&tvrecv, NULL); 
        
        clock_gettime(CLOCK_MONOTONIC, &time_end);         //stop the timer of packet
        
        double timeElapsed = ((double)(time_end.tv_nsec -time_start.tv_nsec))/1000000.0;
	rtt_msec = (time_end.tv_sec- time_start.tv_sec) * 1000.0 + timeElapsed;        //calculate the rtt
        
        if (unpack(recvpacket,n ,rtt_msec) ==  - 1)        //check if current number of bytes received
            continue;
       
       
        nreceived++;
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec))/1000000.0;      //calculate the total time
	
    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ timeElapsed;
    return total_msec;
}






int main(int argc, char *argv[])
{
    struct hostent *host;
    struct protoent *protocol;
    unsigned long inaddr = 0l;
    int waittime = MAX_WAIT_TIME;
    int size = 50 * 1024;

    if (argc < 2)                                                         //checking for valid args
    {
        printf("Please enter destination hostname or ip address\n", argv[0]);
        exit(1);
    } 

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)           // Create RAW socket
    {
        printf("Socket not created\n");return 0;
    }
   
    setuid(getuid());         
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));  // set socket options at ip to TTL and value to 64,
    bzero(&dest_addr, sizeof(dest_addr));                           //set dest addr to zero
    dest_addr.sin_family = AF_INET;                                 //set address family to AF_INET
    if (inaddr = inet_addr(argv[1]) == INADDR_NONE)                 // Get destination IP address and check for validity
    {
        if ((host = gethostbyname(argv[1])) == NULL)                  //if the host deosnt exist
        {
            perror("Bad hostname");
            return 0;
        }
        memcpy((char*) &dest_addr.sin_addr, host->h_addr, host->h_length);
    }
    else
        dest_addr.sin_addr.s_addr = inet_addr(argv[1]);               


    pid = getpid();
    printf("PING %s(%s): %d bytes of data.\n", argv[1], inet_ntoa(dest_addr.sin_addr), datalen);
    signal(SIGINT, intHandler);                 //use Ctrl+C to create interrupts
    double long time=send_packet();                 

    printf("\n--------------------PING statistics-------------------\n");

    printf("%d packets transmitted, %d received , %%%d lost.. Total time: %Lf ms \n", nsend,nreceived, (nsend - nreceived) / nsend *100,time);

    close(sockfd);

    return 0;
}



