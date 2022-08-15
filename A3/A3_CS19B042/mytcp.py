import matplotlib.pyplot as plt
import numpy as np
import argparse
import math
import random
import sys


def congestion_window_simulation(K_i , K_m , K_n , K_f , P_s , T , path):
    MSS=1
    RWS=1024 # 1MB size on receiver size.
    y_axis=[]
    total_segments,threshold=0,512.0#initial value of threshiold 512 KB
    loop=True
    CW_old=K_i
    CW_new=CW_old
    y_axis.append(CW_new)
    stdout=sys.stdout
    with open(path+'.txt','w') as fd:
        sys.stdout=fd
        print(CW_new)
        while loop:
            print("Current segment is : ",total_segments)
            for i in range(min( math.ceil(CW_old/MSS) , T-total_segments )):
                if(random.random()>P_s):
                    CW_new = max( 1,K_f*CW_old)              #timeout
                    threshold=CW_new/2              #reduce threshold after timeout
                else:
                    if(CW_old>=threshold):
                        CW_new = min(CW_old+(K_n/CW_old)*MSS*MSS, RWS)#linear growth phase
                    else :
                        CW_new = min(CW_old+K_m*MSS, RWS) #exponential growth phase
                CW_old=CW_new
                total_segments=total_segments+1
                y_axis.append(CW_new)
                print(CW_new)
            if(total_segments==T):
                loop=False
        sys.stdout=stdout
    x_axis=[]
    for i in range(len(y_axis)):
     x_axis.append(i)
    plt.plot(x_axis, y_axis)
    plt.title("TCP Congestion Control")
    plt.xlabel("update number", fontsize=10)
    plt.ylabel("Congestion Window Size", fontsize=10)
    plt.savefig(path+".png")    
    plt.close()
    


def main():
    input_parser.add_argument("-i",type=float,required=False,default=1.0)#Initial Congestion Window Size
    input_parser.add_argument("-m",type=float,required=False,default=1.0)#Multiplier, during the exponential growth phase
    input_parser.add_argument("-n",type=float,required=False,default=1.0)#Multiplier, during the linear growth phase (0.5 or 1.0)
    input_parser.add_argument("-f",type=float,required=True)#Multiplier, during timeout (0.1 or 0.3) 
    input_parser.add_argument("-s",type=float, required=True)#Probability of successful ACK packet received
    input_parser.add_argument("-T",type=int,required=True)#Total segments to be transferred
    input_parser.add_argument("-o",type=str,required=True)#output file destination
    args=input_parser.parse_args()
    
    if(args.i<1.0 or args.i>4.0 or args.m<0.5 or args.m>2 or args.n<0.5 or args.n>2 or args.f<0.1 or args.f>0.5 or args.s<0.0 or args.s>1.0  or args.T<1):
        print("Error Please verfiy:-i argument should be in the range [1.0,4.0] , -m argument should be in the range [0.5,2.0] , -n argument should be in the range [0.5,2.0] , -f argument should be in the range [0.1,0.5] , -s argument should be in the range [0.0,1.0] ,  -T argument should be a positive integer \n")
   
   
    congestion_window_simulation(args.i , args.m , args.n , args.f , args.s , args.T , args.o)
    
    #for i in [1.0,4.0]:
    #    for m in [1.0,1.5]:
    #        for n in [0.5,1.0]:
     #           for f in [0.1,0.3]:
      #              for s in [0.99,0.9999]:
       #                congestion_window_simulation(i , m , n , f , s , 2000 , str(i)+"_"+str(m)+"_"+"_"+str(n)+"_"+"_"+str(f)+"_"+"_"+str(s))
                   

main()
