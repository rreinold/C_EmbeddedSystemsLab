#include "basic_io.h"
#include "test.h"
#include "LCD.h"
#include "DM9000A.C"
//Start Lab 2 Includes
#include "my_includes.h"
#include "VGA.C"
//End Lab 2 Includes

#define LEDR_BASE_ADDRESS 0x006781070 //from SOPC builder
#define SW_BASE_ADDRESS   0x0067810A0 //from SOPC builder

unsigned int aaa,rx_len,i,packet_num;
unsigned char RXT[68];

unsigned int src_mac1   = 0xFFFFFFFF;
unsigned short src_mac2 = 0xFFFF;
unsigned int src_ip     = 0x22222222;

//void initialize_packet();
unsigned short get_sw();


typedef struct network_struct {
    //Link 
    int dst_mac1;           //48
    short dst_mac2;         //48
    int src_mac1;           //48
    short src_mac2;         //48
    short ethernet;         //16    

    //Network 
    char version_and_head_length;//4each, 8 bits total 01000111 = 0x47
    short total_length;     //16 //in words
    short id;               //16
    short header_checksum;  //16
    int src_ip;             //32
    int dst_ip;             //32

    //Data
    //Transport [
    short src_port;         //16
    short dst_port;         //16
    short length;           //16
    short checksum;         //16

    //Application
    short led_status;       //16
    
    // ] end Transport
    // ] end Network
    int link_checksum;      //32
    // ] Link
} network_struct;

void initialize_packet(network_struct* toSend, short swtemp) { 
    //Get button info
    toSend->led_status = swtemp;
    //Start Transport Initialization
    toSend->src_port = 80;
    toSend->dst_port = 80;
    toSend->length = 10; //length in bytes of our message+header 
    
    unsigned int checkSumTemp = toSend->src_port+toSend->dst_port; if(checkSumTemp > 65535){checkSumTemp++;}
    checkSumTemp += toSend->length; if(checkSumTemp > 65535){checkSumTemp++;}
    checkSumTemp += toSend->led_status; if(checkSumTemp > 65535){checkSumTemp++;}
    toSend->checksum = (short)checkSumTemp;
    toSend->checksum = 65535-toSend->checksum; //flip bits
    //End Transport Initialization
    
    //Start Network Initialization

    toSend->version_and_head_length = 0x47;
    toSend->total_length = 2;    //16 in bytes
    toSend->id=0;              //16 for fragments
    //toSend->header_checksum calculated below
    toSend->src_ip = 0x1111;       //arb
    toSend->dst_ip = 0x2222;       //arb
    //toSend->network_data;   All previous layers
    
    checkSumTemp = toSend->version_and_head_length + toSend->total_length + toSend->id + toSend->src_ip + toSend->dst_ip;
    checkSumTemp = checkSumTemp & 0xFF + (checkSumTemp >> 16);
    checkSumTemp = 65535 - checkSumTemp;
    //End Network Initialization
    
    //Begin Link Initialization
    toSend->dst_mac1 = 0xFFFF;     //arb
    toSend->dst_mac2=0xFF;
    toSend->src_mac1=0x0000;    //arb
    toSend->src_mac2=0x00;
    toSend->ethernet=4 ;  //Guessing 
    //toSend->link_data;  // all prior layers (TBD)
    toSend->link_checksum=0;; //extra credit
    //End Link Initialization
   
    return toSend;  
}

int compare(network_struct* rec){
    return (rec->src_mac1 == src_mac1 &&
            rec->src_mac2 == src_mac2 &&
            rec->dst_ip   == src_ip   &&
            rec->dst_port == 80);
}

void set_leds(short led_data) {
    printf("LED DATA: %x\n", led_data);
    outport((int*)0x00681070, (led_data & 0xFFFCFFFF));
}

unsigned short get_sw() {
    unsigned short sw_data;
    sw_data = (short)(inport((int*)0x006810A0) & 0x0000FFFF);
    printf("SW DATA: %x\n", sw_data);
    return sw_data;
}

/*
 * void ethernet_interrupts()
{
    struct network_struct* inc_packet = (network_struct*) malloc(sizeof

(network_struct));
    packet_num++;
    aaa=ReceivePacket (inc_packet,&rx_len);//store data in inc_packet, 

store length in rx_len
    if(!aaa) {
        printf("\n\nStruct size is %i Receive Packet Length = %d",sizeof

(network_struct),rx_len);
        
        /*for(i=0;i<rx_len;i++) {
            if(i%8==0)
                printf("\n");
            printf("0x%2X,",RXT[i]);
        }
    }
    outport(SEG7_DISPLAY_BASE,packet_num);
}
*/


void ethernet_interrupts()
{
    
        struct network_struct* inc_packet = (network_struct*) malloc(sizeof(network_struct));
    
    packet_num++;
aaa=ReceivePacket (inc_packet,&rx_len);//store data in inc_packet,
    if(!aaa) {
unsigned short tester = inc_packet->led_status;    
printf("Decoded Data: %x\n",tester);

//set_leds(0xAAAA);
//msleep(500);
set_leds(tester);
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
    short sw_data;
    
    struct network_struct* button_packet = (network_struct*) malloc(sizeof(network_struct));



    //main loop
    while (1)
    {
        initialize_packet(button_packet,get_sw());
        
        TransmitPacket(button_packet,sizeof(network_struct));
        //printf(sizeof(network_struct));
        //set_leds(get_sw());  //pretty
        
        msleep(500);
    }

    return 0;
}
//-------------------------------------------------------------------------
