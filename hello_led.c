
#include "basic_io.h"
#include "test.h"
#include "LCD.h"
#include "DM9000A.C"

unsigned int aaa,rx_len,i,packet_num;
unsigned char RXT[68];

typedef struct network_struct {





//Link [
int dst_mac1;       //48
short dst_mac2;
int src_mac1;       //48
short src_mac2
short ethernet; ;    //16    

//Network [

char version_&_head_length;//4each, 8 bits total 01000111 = 0x47
short total_length;    //16 //in words
short id;              //16
short header_checksum; //16
int src_ip;          //32
int dst_ip;          //32
//Data
//Transport [
short src_port;  //16
short dst_port;  //16
short length;    //16
short checksum;  //16
//Application
short led_status; //16
// ] end Transport
// ] end Network
int link_checksum; //32
 // ] Link
} network_struct;

function init(struct_toSend){
    //Start Transport Initialization
    toSend.src_port = 80;
    toSend.dst_port = 80;
    toSend.length = 10 //length in bytes of our message+header 
    
    if(1 << 15 != 65535){printf("Error in calculation\n");}
    
    int checkSumTemp = toSend.src_port+toSend.dst_port; if(checkSumTemp > 65535){checkSumTemp++;}
    checkSumTemp += toSend.length; if(checkSumTemp > 65535){checkSumTemp++;}
    checkSumTemp += toSend.led_status; if(checkSumTemp > 65535){checkSumTemp++;}
    toSend.checksum = (short)checkSumTemp;
    toSend.checksum = 65535-toSend.checksum; //flip bits
    //End Transport Initialization
    
    //Start Network Initialization

    toSend.version_&_head_length = 0x47;
    toSend.total_length = 2;    //16 in bytes
    toSend.id=0;              //16 for fragments
    //toSend.header_checksum calculated below
    toSend.src_ip = 0x90AB;          //arb
    toSend.dst_ip = 0xFEDC;          //arb
    //toSend.network_data;     All previous layers
    
    checkSumTemp = toSend.version_&_head_length + toSend.total_length + toSend.id + toSend.src_ip + toSend.dst_ip;
    checkSumTemp = checkSumTemp & 0xFF + (checkSumTemp >> 16);
    checkSumTemp = 65535 - checkSumTemp;
    //End Network Initialization
    
    //Begin Link Initialization
    toSend.dst_mac1 = 0xFFFF;      //arb
    toSend.dst_mac2=0xFF;
    toSend.src_mac1=0x0000;       /arb
    toSend.src_mac2=0x00;
    toSend.ethernet=4 ;     //Guessing 
    //toSend.link_data;     // all prior layers (TBD)
    toSend.link_checksum=0;; //extra credit
    //End Link Initialization
   
        
    
}

boolean compare(network_struct rec_struct){
  return (src_mac1 == rec.src_mac1 &&
            src_mac2 == rec.src_mac2 &&
            src_ip == rec.dst_ip &&
            80 == rec.dst_port);
}
/*
typedef struct network_struct {
//Application
short led_status; //16

//Transport
short src_port;  //16
short dst_port;  //16
short length;    //16
short checksum;  //16

//Network
char version_&_head_length;//4each, 8 bits total
short total_length;    //16
short id;              //16
short header_checksum; //16
int src_ip;          //32
int dst_ip;          //32
int network_data;     //TBD

//Link
int dst_mac1;       //48
short dst_mac2;
int src_mac1;       //48
short src_mac2
short ethernet; ;    //16    
int link_data;     // all prior layers (TBD)
int link_checksum; //32
 
} network_struct;
*/


void ethernet_interrupts()
{
    packet_num++;
    aaa=ReceivePacket (RXT,&rx_len);
    if(!aaa)
    {
      printf("\n\nReceive Packet Length = %d",rx_len);
      for(i=0;i<rx_len;i++)
      {
        if(i%8==0)
        printf("\n");
        printf("0x%2X,",RXT[i]);
      }
    }
    outport(SEG7_DISPLAY_BASE,packet_num);
}

int main(void)
{
  unsigned char TXT[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                          0x01,0x60,0x6E,0x11,0x02,0x0F,
                          0x08,0x00,0x11,0x22,0x33,0x44,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x55,0x66,0x77,0x88,0x99,0xAA,
                          0x00,0x00,0x00,0x20 };
  LCD_Test();
  DM9000_init();
  alt_irq_register( DM9000A_IRQ, NULL, (void*)ethernet_interrupts ); 
  packet_num=0;
  while (1)
  {
    TransmitPacket(TXT,0x40);
    msleep(500);
  }

  return 0;
}

//-------------------------------------------------------------------------


