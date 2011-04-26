/********************************************************************
*  
*  Copyright (c) 2010, Institute of Embedded Systems at 
*                      Zurich University of Applied Sciences 
*                      (http://ines.zhaw.ch)
*  
*  All rights reserved.
* 
* 
*  Redistribution and use in source and binary forms, with or  
*  without modification, are permitted provided that the 
*  following conditions are met:
*  
*  - Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer. 
*
*  - Redistributions in binary form must reproduce the above 
*    copyright notice, this list of conditions and the following 
*    disclaimer in the documentation and/or other materials provided
*    with the distribution. 
*
*  - Neither the name of the Zurich University of Applied Sciences
*    nor the names of its contributors may be used to endorse or 
*    promote products derived from this software without specific 
*    prior written permission. 
*
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
*  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
*  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
*  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
*  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
*  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
*  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*  
*********************************************************************/

/********************************************************************
*   _____       ______  _____                                       *
*  |_   _|     |  ____|/ ____|                                      *
*    | |  _ __ | |__  | (___    Institute of Embedded Systems       *
*    | | | '_ \|  __|  \___ \   Zurich University of Applied        *
*   _| |_| | | | |____ ____) |  Sciences                            *
*  |_____|_| |_|______|_____/   8401 Winterthur, Switzerland        *
*                                                                   *
*********************************************************************
* 
*  Project     : Parallel Redundancy Protocol
* 
*  Version     : 1.0
*  Author      : Sven Meier/Patrick Staehlin/David Gunzinger
* 
*********************************************************************
*  Change History
*
*  Date     | Name     | Modification
************|**********|*********************************************
*  01.01.08 | mesv     | file created
************|**********|*********************************************
*  05.02.08 | stpa	   | error handling / fixes
*********************************************************************
*  18.03.10 | gunz	   | rewrite for single thread, pcap and tun/tap
*********************************************************************/


/********************************************************************

Getting started:

1)		./prp_linux_users_pace ethLanA ethLanB
2)		ifconfig prp0 <your ip>

Known problems:
* timer accuracy 
* linux answers on interfaces even if there is no ip assigned, there is
  no solution for disable TCP completly for an interface !
* no restore of interface MACs and Flags once the program quits

*********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>    
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/if_ether.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>  

#include <pcap.h>
#include <linux/if_tun.h>


#include "../prp/PRP_T.h"
#include "../prp/PRP_LogItf_T.h"
#include "../prp/PRP_NetItf_T.h"



/********************************************************************/
/*			local vars and functions					    		*/
/********************************************************************/
static volatile boolean exit_prp = FALSE;
pcap_t *port_a;
pcap_t *port_b;
int tap;
char devname[IFNAMSIZ];
char port_a_name[IFNAMSIZ];
char port_b_name[IFNAMSIZ];
    
    
static void sig_term(int sig)
{
    exit_prp = TRUE;
}



#ifdef PRP_DEBUG_LOG
	extern int debug_level;
	extern int net_itf_debug_level;
	extern int prp_debug_level;
#endif

/********************************************************************/
/*			function prototypes			    						*/
/********************************************************************/
int set_mac(char *dev, unsigned char *buffer);


/************************************************************/
/*       PRP_NetItf_T_set_active_A                          */
/************************************************************/
integer32 PRP_NetItf_T_set_active_A(boolean value)
{
    /* Don't care*/
	return(0);
}

/************************************************************/
/*       PRP_NetItf_T_set_active_B                          */
/************************************************************/
integer32 PRP_NetItf_T_set_active_B(boolean value)
{
    /* Don't care*/
	return(0);
}

/************************************************************/
/*       PRP_NetItf_T_set_mac_address_A                     */
/************************************************************/
integer32 PRP_NetItf_T_set_mac_address_A(octet* mac)
{
 
    /* Don't care*/
    return(0);
}

/************************************************************/
/*       PRP_NetItf_T_set_mac_address_B                     */
/************************************************************/
integer32 PRP_NetItf_T_set_mac_address_B(octet* mac)
{

    /* Don't care*/
    return(0);
}

/************************************************************/
/*       PRP_NetItf_T_set_supervision_address               */
/************************************************************/
integer32 PRP_NetItf_T_set_supervision_address(octet* mac)
{
    /* Don't care*/
      return(0);
}


/************************************************************/
/*       PRP_NetItf_T_transmit                              */
/************************************************************/
integer32 PRP_NetItf_T_transmit(octet* data, uinteger32* length, octet lan_id)
{
	if(lan_id == PRP_ID_LAN_A)
	{
        pcap_sendpacket(port_a,data,*length);
	}
	else if(lan_id == PRP_ID_LAN_B)
	{
        pcap_sendpacket(port_b,data,*length); /* LAN B */
	}
	else
	{
    	fprintf(stderr, "Invalid lan ID in %s\n", __FUNCTION__);
	}
	return(0);
}




/*******************************************************************/
/*** PcapReceive_callback                                        ***/
/*** Args = PRP_ID_LAN_[A|B]                                     ***/
/*******************************************************************/


void PcapReceive_callback(unsigned char *Args,
                      const struct pcap_pkthdr* Pkthdr,
                      unsigned char *Packet)
{
        octet lanID = *Args;
        uinteger32  len;
    	int res = -1;

        len = Pkthdr->caplen;

        res = PRP_T_receive(Packet, &len, lanID);  

        if(res == PRP_KEEP)
        {
            write(tap,Packet,len);
        }
}




/*******************************************************************/
/*** Open new virtual TAP PRP device                             ***/
/*******************************************************************/


int tap_open(char *dev)
{
    struct ifreq ifr;
    int fd;
    int flags;
    if ((fd = open("/dev/net/tun", O_RDWR)) > 0){
        fprintf(stderr,"/dev/net/tun open okay\n");
    }else if ((fd = open("/dev/net", O_RDWR)) > 0){
        fprintf(stderr,"/dev/net open okay\n");
    } else {
        goto failed;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (dev)
       strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
          fprintf(stderr,"ioctl failed\n");
          goto failed;
    } 
    strcpy(dev, ifr.ifr_name);
    
    /* set nonblocking */
    if(-1==(flags=fcntl(fd,F_GETFL,0))){
        flags = 0;
    }
    if(fcntl(fd,F_SETFL,flags|O_NONBLOCK) == -1){
        fprintf(stderr,"set nonblock failed\n");
    }


    return fd;
failed:
    perror("tap open failed");
    close(fd);
    return -1;
}
/*******************************************************************/
/*** open pcap live for sniffing                                 ***/
/*******************************************************************/
pcap_t*  raw_open(char *dev){
    pcap_t  *pcap_handle;
    char    errbuf[PCAP_ERRBUF_SIZE];
    
    pcap_handle = pcap_open_live(dev,ETHER_MAX_LEN,1,1,errbuf);
   
   if(pcap_handle == NULL)
   {
    fprintf(stderr, "Could not open device \"%s\": %s\n", dev, errbuf);
    return NULL;
   }

    if(pcap_setnonblock(pcap_handle, 1, errbuf) == 1)
   {
   fprintf(stderr, "Could not set device \"%s\" to non-blocking: %s\n", dev, errbuf);
    return NULL;
   }
   
   return pcap_handle;
    
}

/*********************************************************************/
/*** set_mac_filter(pcap_t* dev,unsigned char *mac)                ***/
/*********************************************************************/
int set_mac_filter(pcap_t* dev,unsigned char *mac){
    struct bpf_program fp;
    char filter_string[100];
    sprintf(filter_string,"not ether src %02x:%02x:%02x:%02x:%02x:%02x",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    if(pcap_compile(dev,&fp,filter_string,1,0) == -1){
        fprintf(stderr,"pcap compile failed %s",pcap_geterr(dev));
        return -1;
    }
    if(pcap_setfilter(dev,&fp) == -1){
        fprintf(stderr,"pcap setfilter failed %s",pcap_geterr(dev));
        return -1;
    }
    return 0;
}


/*********************************************************************/
/*** get_mac(char *dev, char *buffer)                               ***/
/*********************************************************************/
int get_mac(char *dev, unsigned char *buffer){
    int sd;
    struct ifreq ifr;
    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sd < 0) {
        perror("socket");
        return -1;
    }
    strncpy((char*)&ifr.ifr_name, dev, IFNAMSIZ);
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFHWADDR");
        close(sd);
        return -1;
    }

    memcpy(buffer,	&ifr.ifr_hwaddr.sa_data,IFHWADDRLEN);
    close(sd);
    return 0;

}
/*********************************************************************/
/*** set_mac(char *dev, char *buffer)                               ***/
/*********************************************************************/
int set_mac(char *dev, unsigned char *buffer){
    int sd;
    struct ifreq ifr;
    int flags;
    struct sockaddr *addr;
    bzero(&ifr,sizeof(ifr));
    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sd < 0) {
        perror("socket");
        return -1;
    }

     strncpy((char*)&ifr.ifr_name, dev, IFNAMSIZ);
        
    /*get interface flags */
    if (ioctl(sd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCGIFFLAGS");
        close(sd);
        return -1;
    }        
     /* save flags */
    flags = ifr.ifr_flags;
    /*set interface down*/
    ifr.ifr_flags = 0;
    if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sd);
        return -1;
    }            

    /* set the mac addres */
    addr = (struct sockaddr*)&ifr.ifr_hwaddr;
    addr->sa_family = AF_UNIX;
    memcpy(addr->sa_data,buffer,IFHWADDRLEN);
    if (ioctl(sd, SIOCSIFHWADDR, &ifr) < 0) {
        perror("ioctl SIOCSIFHWADDR");
        close(sd);
        return -1;
    }
    /* restore flags */
    ifr.ifr_flags = flags;
    if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sd);
        return -1;
    }
    close(sd);           
    return 0; 
}
/*********************************************************************/
/*** set_flags_red(char *dev)   -   device flags for red. net. cards   ***/
/*********************************************************************/
int set_flags_red(char *dev){
    int sd;
    struct ifreq ifr;
    bzero(&ifr,sizeof(ifr));
    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sd < 0) {
        perror("socket");
        return -1;
    }
     strncpy((char*)&ifr.ifr_name, dev, IFNAMSIZ);
        
                
    /*set flags, set MULTICAST */
    ifr.ifr_flags = IFF_ALLMULTI |  IFF_MULTICAST | IFF_UP | IFF_NOARP | IFF_PROMISC;
    if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sd);
        return -1;
    }
         
    /* set mtu to 1500 */
     ifr.ifr_mtu = 1500;
    if (ioctl(sd, SIOCSIFMTU, &ifr) < 0) {
        perror("ioctl SIOCSIFMTU");
        close(sd);
        return -1;
    }       
    close(sd);              
    return 0; 
}

/*********************************************************************/
/*** set_flags_prp(char *dev)   -   device flags for virtual prp   ***/
/*********************************************************************/
int set_flags_prp(char *dev){
    int sd;
    struct ifreq ifr;
    bzero(&ifr,sizeof(ifr));
    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sd < 0) {
        perror("socket");
        return -1;
    }
    /*fprintf(stderr,"dev %s\n",dev);*/
     strncpy((char*)&ifr.ifr_name, dev, IFNAMSIZ);

    /* set flags */
    ifr.ifr_flags = IFF_ALLMULTI |  IFF_MULTICAST | IFF_UP  ;
    if (ioctl(sd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sd);
        return -1;
    }
    
    
    /* set mtu to 1496 */
     ifr.ifr_mtu = 1496;
    if (ioctl(sd, SIOCSIFMTU, &ifr) < 0) {
        perror("ioctl SIOCSIFMTU");
        close(sd);
        return -1;
    }
    close(sd);    
    return 0; 
}

/*********************************************************************/
/*** main                                                          ***/
/*********************************************************************/



int main(int argc, char* argv[]){
    int port_a_fd,port_b_fd;
    unsigned char addr_A[IFHWADDRLEN];
    unsigned char new_lamo[IFHWADDRLEN] ;
    PRP_MergeLayerInfo_T merge_layer_info;
	char node_name[] = "ZHAW PRP Node";
	char manufacturer_name[] = "ZHAW InES";
	char version[] = "pcap + tap";
	struct timeval next;	

    exit_prp = FALSE;
    
	
#ifdef PRP_DEBUG_LOG
	debug_level = PRP_DEBUG_LEVEL;
	net_itf_debug_level = PRP_DEBUG_LEVEL;
	prp_debug_level = PRP_DEBUG_LEVEL;
#endif

    

	/* parse IP interface parameter */
	if(argc == (3))
	{
    	
        strncpy(port_a_name, argv[1],IFNAMSIZ);
        strncpy(port_b_name, argv[2],IFNAMSIZ);
	}
	else
	{
		fprintf(stderr, "Usage: %s eth1 eth2 \n",argv[0]);
		exit(-1);
	}
    
    /* renice */
    errno = 0;
    nice(-20);
    if(errno == 0){
        fprintf(stderr,"renice: done\n");
    } else {
        fprintf(stderr,"renice: failed");
        perror("");
        return -1;
    }

    /* sysctl */
    system("echo 2 > /proc/sys/net/ipv4/conf/all/arp_announce");
    system("echo 1 > /proc/sys/net/ipv4/conf/all/arp_ignore");





    
    PRP_T_init();
    PRP_T_get_merge_layer_info(&merge_layer_info); 
 	memset(merge_layer_info.node_, '\0' , PRP_NODE_NAME_LENGTH);	
 	memcpy(merge_layer_info.node_, node_name, sizeof(node_name));
 	memset(merge_layer_info.manufacturer_, '\0' , PRP_MANUFACTURER_NAME_LENGTH);	
 	memcpy(merge_layer_info.manufacturer_, manufacturer_name, sizeof(manufacturer_name));
 	memset(merge_layer_info.version_, '\0' , PRP_VERSION_LENGTH);	
 	memcpy(merge_layer_info.version_, version, sizeof(version));
	merge_layer_info.adapter_active_A_ = FALSE;
	merge_layer_info.adapter_active_B_ = FALSE;
    PRP_T_set_merge_layer_info(&merge_layer_info);    

    
    /* open tap device */
    strcpy(devname,"prp0");
    if((tap = tap_open(devname)) > 0){
        fprintf(stderr,"tap open %s: done\n",devname);
    }else {
        fprintf(stderr,"tap open %s: failed\n",devname);
        return -1;        
    }        

    /* copy mac address from port a to prp0 */
    if(get_mac(port_a_name,addr_A) >= 0){
        fprintf(stderr,"get mac address of %s: done %02x:%02x:%02x:%02x:%02x:%02x\n",port_a_name,
        addr_A[0],addr_A[1],addr_A[2],addr_A[3],addr_A[4],addr_A[5]);
    } else {
        fprintf(stderr,"get mac address of %s: failed\n",port_a_name);
        return -1;
    }

    /* make new addresses */
    /* linux behaves buggy if multiple interfaces receive the same packet
     * and have the same mac address ! 
     */
    memcpy(new_lamo,addr_A,IFHWADDRLEN);
    addr_A[0] = addr_A[0] & (~2);
    new_lamo[0] = new_lamo[0] | 2;
    
     if(set_mac(port_a_name,new_lamo) >= 0){
        fprintf(stderr,"set mac address of %s: done\n",port_a_name);
    } else {
        fprintf(stderr,"set mac address of %s: failed\n",port_a_name);
        return -1;
    }   
        
    if(set_mac(port_b_name,new_lamo) >= 0){
        fprintf(stderr,"set mac address of %s: done\n",port_b_name);
    } else {
        fprintf(stderr,"set mac address of %s: failed\n",port_b_name);
        return -1;
    }
    
    if(set_mac(devname,addr_A) >= 0){
        fprintf(stderr,"set mac address of %s: done\n",devname);
    } else {
        fprintf(stderr,"set mac address of %s: failed\n",devname);
        return -1;
    }
       
   if(set_flags_red(port_a_name) >= 0){
        fprintf(stderr,"set flags of %s: done\n",port_a_name);
    } else {
        fprintf(stderr,"set flags of %s: failed\n",port_a_name);
        return -1;
    }
    if(set_flags_red(port_b_name) >= 0){
        fprintf(stderr,"set flags of %s: done\n",port_b_name);
    } else {
        fprintf(stderr,"set flags of %s: failed\n",port_b_name);
        return -1;
    } 
    if(set_flags_prp(devname) >= 0){
        fprintf(stderr,"set flags of %s: done\n",devname);
    } else {
        fprintf(stderr,"set flags of %s: failed\n",devname);
        return -1;
    }       



    /* open port a */
    if((port_a = raw_open(port_a_name)) != NULL){
        fprintf(stderr,"port_a open %s: done\n",port_a_name);
    } else {
        fprintf(stderr,"port_a open %s: failed\n",port_a_name);
        return -1;
    }
    /* open port b */
    if((port_b = raw_open(port_b_name)) != NULL){
        fprintf(stderr,"port_b open %s: done\n",port_b_name);
    } else {
        fprintf(stderr,"port_b open %s: failed\n",port_b_name);
        return -1;
    }
    /* set mac filters  */
   if(set_mac_filter(port_a,addr_A) >= 0){
        fprintf(stderr,"set mac filter port a: done\n");
    } else {
        fprintf(stderr,"set mac filter port a: failed\n");
        return -1;
    }
   if(set_mac_filter(port_b,addr_A) >= 0){
        fprintf(stderr,"set mac filter port b: done\n");
    } else {
        fprintf(stderr,"set mac filter port b: failed\n");
        return -1;
    }

	/* start */
    PRP_T_get_merge_layer_info(&merge_layer_info); 
 	memcpy(merge_layer_info.mac_address_A_, addr_A, PRP_ETH_ADDR_LENGTH);
 	memcpy(merge_layer_info.mac_address_B_, addr_A, PRP_ETH_ADDR_LENGTH);  	
	merge_layer_info.adapter_active_A_ = TRUE;
	merge_layer_info.adapter_active_B_ = TRUE;
    PRP_T_set_merge_layer_info(&merge_layer_info);	   
    

    fprintf(stderr,"entering main loop, press q to exit\n");
	gettimeofday(&next,NULL);
	
    signal(SIGINT, sig_term);
    signal(SIGQUIT, sig_term);
    signal(SIGTERM, sig_term);

    while(!exit_prp){
        fd_set            descriptors;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; 
        unsigned char buffer[ETHER_MAX_LEN];
        port_a_fd = pcap_fileno(port_a);
        port_b_fd = pcap_fileno(port_b);
        unsigned char args;
        
        FD_ZERO(&descriptors);
        FD_SET(tap,&descriptors);
        FD_SET(port_a_fd,&descriptors);
        FD_SET(port_b_fd,&descriptors);
        FD_SET(STDIN_FILENO,&descriptors);
        /* select ! */
        int retval = select(FD_SETSIZE,&descriptors,NULL,NULL,&timeout);
	
		/* check for timer, replacement for timer_fd */
		struct timeval now;
		gettimeofday(&now,NULL);
		if((now.tv_sec > next.tv_sec) || ((now.tv_sec == next.tv_sec) && (now.tv_usec > next.tv_usec))) {
			PRP_T_timer();
			next.tv_usec = now.tv_usec + PRP_TIMER_TICK_INTERVAL*1000;	/* in 100 ms */
			next.tv_sec = now.tv_sec;
			if(next.tv_usec > 1000000){
				next.tv_usec -= 1000000;
				next.tv_sec += 1;
			}
		}
		
		
		if(retval < 0){
            perror("select failed");
            exit_prp = TRUE;
        } else if (retval == 0){
        /* timeout - do nothing*/
			
        } else {
            if(FD_ISSET(STDIN_FILENO,&descriptors)){
                unsigned int len;
                len = read(STDIN_FILENO,buffer,ETHER_MAX_LEN);
                if(buffer[0] == 'q'){
                    exit_prp = TRUE;
                }
            }
            
            if(FD_ISSET(tap,&descriptors)){
                unsigned int len;
                len = read(tap,buffer,ETHER_MAX_LEN);
                if(len < 60){
                    memset(&buffer[len],0x00,60-len);
                    len = 60;
                }
                PRP_T_transmit(buffer, &len);           
            }
            if(FD_ISSET(port_a_fd,&descriptors)){
                args = PRP_ID_LAN_A;
                pcap_dispatch(port_a, 1, (void *) PcapReceive_callback, &args);
            }
            if(FD_ISSET(port_b_fd,&descriptors)){
                args = PRP_ID_LAN_B;
                pcap_dispatch(port_b, 1, (void *) PcapReceive_callback, &args);
            }
            
        }
    }
    
 	PRP_T_cleanup();
		
	return 0;
}