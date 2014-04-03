#include "basic_io.h"
#include "test.h"
#include "LCD.h"
#include "DM9000A.C"

#define LEDR_BASE_ADDRESS 0x006781070 //from SOPC builder
#define SW_BASE_ADDRESS   0x0067810A0 //from SOPC builder

unsigned int aaa,rx_len,i,packet_num;
unsigned char RXT[68];
unsigned short get_sw();

unsigned char IMG[38400];
unsigned char packetHolder[1514]; //1514 = 14 + 20 + 4 + 1476

    unsigned char packetIncoming[39]  = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


unsigned char packetHeaders[39]  = 
{//ETHERNET (14 bytes)
    0xFF, 0x48, 0x48, 0x48, 0x48, 0x48, //dst mac
    0xFF, 0x10, 0x10, 0x10, 0x10, 0x10, //src mac
    0x08, 0x00,                         //ethernet type
    //IPv4 (20 bytes)
    0x47, 0x00, 0x99, 0x99,             //version, header length, type of service, total length
    0x99, 0x99, 0x99, 0x99,             //ID, flags, evil bit, frag offset
    0x99, 0x99, 0x99, 0x99,             //TTL, protocol, header checksum
    0x11, 0x11, 0x11, 0x11,             //Source Address
    0x22, 0x22, 0x22, 0x22,             //Destination Address
    //UDP (4 bytes)
    0x50, 0x50,                         //source port, dst port
    0x99, 0x99                          //length, checksum, data (1476 bytes)
};






void set_leds(unsigned short led_data) {
    printf("LED DATA: %x\n", led_data);
    outport((int*)0x00681070, (led_data & 0xFFFCFFFF));
}

unsigned short get_sw() {
    unsigned short sw_data;
    sw_data = (short)(inport((int*)0x006810A0) & 0x0000FFFF);
    printf("SW DATA: %x\n", sw_data);
    return sw_data;
}


void initialize_packet() { 

    //load headers
    int i;
    for (i = 0; i < 38; i++) {
        packetHolder[i] = packetHeaders[i];
    }

    //load data
    unsigned int sw_data = get_sw();
    packetHolder[38] = sw_data & 0xff;
    //printf("sending packet: %u, ", sw_data);







    /*  int pixelIndex;
    int numDataBytes;

    for (int i=0, i < 1476; i++) {
        char packedPixels;
    
        for (int j=0; j < 8; j++) {
            if (pixelIndex < 640*480) {
                char shiftedPixel = BIN_PIX << (7-j);
                packedPixels += shiftedPixel;
                pixelIndex++;
            } else {
                //herp derpes last packet aint full
            
            }
        }
        if (pixelIndex < 640*480) {
            numDataBytes++;
        }
        packetHolder[42+i] = packedPix; 
    }*/

/*  //set packet lengths & checksums
    packetHolder[20] = //IPv4 header length
    packetHolder[29] = //IPv4 header checksum
    packetHolder[30] = //IPv4 header checksum
    
    packetHolder[41] = //UDP data length
    packetHolder[41] = //UDP checksum*/
    
    
}



void ethernet_interrupts()
{
    //unsigned char* packetIncoming[40] = (unsigned char*)malloc(40*sizeof(unsigned char));
    //unsigned char packetIncoming[39];
    //packetIncoming = (unsigned char*)malloc(39*sizeof(char));
    packet_num++;
    printf("Packet received %u\n",packet_num);
    msleep(500);

    
    
    
    aaa=ReceivePacket (packetIncoming,&rx_len);//store data in inc_packet,
    
    if(!aaa) {
       
        unsigned short tester;
        
        tester = packetIncoming[38];
        
        printf("Payload: %x",tester);
       
        //set_leds(0xAAAA);
        msleep(50);
        
        set_leds(tester);
        
    }
    
    
    outport(SEG7_DISPLAY_BASE,packet_num);
    DM9000_init();
    
}

int main(void)
{

    LCD_Test();
    DM9000_init();
    alt_irq_register( DM9000A_IRQ, NULL, (void*)ethernet_interrupts ); 
    packet_num=0;
    short sw_data;


    int tempCount = 0;
    //main loop
    while (1)
    {
        //iow(ISR, 0x01); 
        //iow(IMR, INTR_set);
        
        initialize_packet();

        
        TransmitPacket(packetHolder,39*sizeof(unsigned char));

        //set_leds(get_sw());  //pretty
        
        msleep(2000);
        tempCount++;
        printf("Transmit Count: %u\n",tempCount);
        
    }

    return 0;
}
//-------------------------------------------------------------------------
