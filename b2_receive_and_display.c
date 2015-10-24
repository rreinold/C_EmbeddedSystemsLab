#include "basic_io.h"
#include "test.h"
#include "LCD.h"
#include "DM9000A.C"
#include "sys/alt_irq.h"
//#include "HAL4D13.C"
#include "my_includes.h"

#define LEDR_BASE_ADDRESS 0x006781070 //from SOPC builder
#define SW_BASE_ADDRESS   0x0067810A0 //from SOPC builder
#define LEDG_BASE_ADDRESS  0x00681080


unsigned int aaa,rx_len,i,packet_num;
unsigned char RXT[68];

int timeout = 0;

unsigned short get_sw();

char incomingPacket[1318];
char outgoingPacket[1318];
char encodedArray[76800];

char imageArray[38400];
char alteredArray[38400];

typedef enum {IDLE, TRANNY} status;
status currentState = IDLE;
char imageCommand = 0;
char imageCount = 0;

unsigned int packetCount = 0;
char packetHeaders[38] =
{   //ARP (14 bytes)
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


unsigned char numeralFont [10][7] = {
    {//0
    0x00,
    0x78,
    0x48,
    0x48,
    0x48,
    0x78,
    0x00},


    { //1
    0x00,
    0x10,
    0x30,
    0x10,
    0x10,
    0x78,
    0x00},


    { //2
    0x00,
    0x78,
    0x08,
    0x78,
    0x40,
    0x78,
    0x00},


    { //3
    0x00,
    0x78,
    0x08,
    0x78,
    0x08,
    0x78,
    0x00},



    { //4
    0x00,
    0x48,
    0x48,
    0x78,
    0x08,
    0x08,
    0x00},


    { //5
    0x00,
    0x78,
    0x40,
    0x78,
    0x08,
    0x78,
    0x00},


    { //6
    0x00,
    0x78,
    0x40,
    0x78,
    0x48,
    0x78,
    0x00},


    { //7
    0x00,
    0x78,
    0x08,
    0x08,
    0x08,
    0x08,
    0x00},

    { //8
    0x00,
    0x78,
    0x48,
    0x78,
    0x48,
    0x78,
    0x00},


    { //9
    0x00,
    0x78,
    0x48,
    0x78,
    0x08,
    0x78,
    0x00}
};


unsigned char reverseChar(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void readImage(){
    alt_u32 time_start, ticks_per_sec;
    alt_u8 *szPacket; 
    ticks_per_sec = alt_ticks_per_second();
    alt_u8 * pData;
    pData = (volatile alt_u8 *) CFI_FLASH_0_BASE;
    unsigned int j;
    unsigned char shiftedPixel, packedPixel = 0;
    unsigned int byteIndex=0;
    alt_flash_fd* fd;
    unsigned int offset = 0x10;
    char bin_pix=0;;
    fd = alt_flash_open_dev(CFI_FLASH_0_NAME);
    if (fd==NULL) {
        printf("Flash memory open failure\n");
        return;
    }
 
    printf("Reading binary pixel from flash memory\n");
    for(i = 0 ; i < 480 ; i++) {
        printf("Currently at %u/480\n",i);
        for(j = 0 ; j < 640 ; j++) {    
            alt_read_flash(fd,offset,&bin_pix,1);

            shiftedPixel = bin_pix << (7 - j%8);//mask
            packedPixel += shiftedPixel; 
            offset++;
            if(j % 8 == 7) {
                imageArray[byteIndex] = packedPixel;
                byteIndex++;
                packedPixel=0;
            }
        } 
    }//end for
    
    //alt_flash_close_dev(fd);
}

void sendAck() {
    //load headers, clean container
    int i;
    for (i = 0; i <= 1318; i++) {
        if (i < 38) {
            outgoingPacket[i] = packetHeaders[i];
        } else {
            outgoingPacket[i] = 0;
        }
    }
    outgoingPacket[38] = 0xF0;
    TransmitPacket(outgoingPacket, sizeof(outgoingPacket));
    msleep(100);
    printf("Sent ACK\n\n");
}

void sendAppAck() {
    //load headers, clean container
    int i;
    for (i = 0; i <= 1318; i++) {
        if (i < 38) {
            outgoingPacket[i] = packetHeaders[i];
        } else {
            outgoingPacket[i] = 0;
        }
    }
    outgoingPacket[38] = 0xaa;
    TransmitPacket(outgoingPacket, sizeof(outgoingPacket));
    msleep(100);
    printf("Sent AppACK!\n\n");
}

void flashGreenLEDs() {
    outport((int*)0x00681080, 0x000000FF);
    msleep(10);
    outport((int*)0x00681080, 0x00000000);
}

void encode(){

    unsigned int e = 0, encodeOutput=0, ready = 0, full=0;
    unsigned int data0=0,data1=0,data2=0,fullEncoded= 0,encodedIndex=0;
    while(1){
        ready = inport(RESULT_READY_PIO); 
        full = inport(FIFO_IN_FULL_PIO);
        if(e == 38400){
            break; //break while look when all bits have been encoded
            }
    
        else if (!inport(FIFO_IN_FULL_PIO_BASE))  {     //Insert Byte
            outport(ODATA_PIO,  0xF3   ); //set 8 bits
            outport(FIFO_IN_WRITE_REQ_PIO , 1); //assert write request
            //usleep(10);
            outport(FIFO_IN_WRITE_REQ_PIO , 0); //deassert write request
            e++; //increment bytes encoded
        }
        
        if(inport(RESULT_READY_PIO)){
            outport(FIFO_OUT_READ_REQ_PIO,1); //assert read request
outport(FIFO_OUT_READ_REQ_PIO,0); //deassert read request
            fullEncoded = inport(IDATA_PIO); //read data
            
			
	data2 = fullEncoded & 0xFF; //mask MSB
	data1 = (fullEncoded >> 8) & 0xFF;
	data0 = (fullEncoded >> 12) & 0xFF; //mask LSB

	 //store in new array

	encodedArray[encodedIndex]=data0;	encodedArray[encodedIndex+1] = data1; 
	encodedArray[encodedIndex+2] = data2;
	encodedIndex+=3; //increment index 
	
        }
    } //end while
	packetHeader = encodedIndex; //added to header when finished
}


void initializePacket() {
    //load headers
    int i;
    for (i = 0; i < 38; i++) {
        outgoingPacket[i] = packetHeaders[i];
    }


    outgoingPacket[38]= 0xbe;


    unsigned int imageOffset = packetCount*1280;
    for (i = 0; i < 1280; i++) {
        outgoingPacket[i+39]   =  alteredArray[imageOffset+i];
    }
    printf("Packed packet #: %u\n\n",packetCount);



}

addEffect(unsigned short effect){
        int u=0;
        unsigned int x,y,byteIndex = 0;
        switch (effect) {
            case 0: 
                //take snapshot
                printf("Taking Snapshot.\n");
                break;
                
            case 1://raw image
                for (y = 0 ; y < 38400 ; y++) {
                    alteredArray[y] = imageArray[y];
                }
                
                break;

                
            default:

                break;
        }
}


void transmit(){
    if(packetCount < 30){
        initializePacket(0xbe);
        TransmitPacket(outgoingPacket, sizeof(outgoingPacket));
        msleep(100);
        packetCount++; //increment packetCount
        //currentState = WAIT; //wait for next packet
    } else if(packetCount == 30){
        sendAppAck();
        printf("finished image transition; current state: %u\n\n",currentState);
    }
}

void ethernet_interrupts()
{
    packet_num++;
    
    
    aaa=ReceivePacket (incomingPacket,&rx_len);
    if(!aaa) {
        unsigned char switchData = incomingPacket[38];
        printf("incomingPacket[38]: %x\n",switchData);

        
        switch (currentState) {
            case IDLE:
                printf("Packet received during: %u\n",currentState);
                if( switchData < 8 && switchData > 0){ //if it is a command
                    sendAck();
                    //imageCommand = switchData; //referenced in main to determine image processing
                    addEffect(switchData);
                    currentState = TRANNY; //set state to transmitting
                    imageCount++;
                    printf("Image Command: %u. Setting State to: %u\n",imageCommand, currentState);
                    //readImage();
                    //send first packet
                    transmit();
                } else {
                    printf("Got command greater than 8 during IDLE\n");
                }
                break;
            
            case TRANNY:
                printf("Packet received during: %u\n",currentState);
                if(switchData == 0xf0 && packetCount == 30){
                    flashGreenLEDs();
                    currentState = IDLE;
                    packetCount = 0;
                    switchData = 0;
                    printf("Resetting.\n");
                    sendAppAck();
                } else if(switchData == 0xf0){ //if ACK
                    flashGreenLEDs();
                    transmit();
                    currentState = TRANNY;
                    printf("Received ACK. In State: TRANNY\n");
                } 
                break;
            default: 
                
                printf("\nReceived packet that broke state machine\n\n");
                break;
        }
        
        
        
    }
    outport(SEG7_DISPLAY_BASE,packet_num);
}

int main(void)
{
    encode();
    printf("Boot up.\n");
    LCD_Test();
    DM9000_init();
    alt_irq_register( DM9000A_IRQ, NULL, (void*)ethernet_interrupts ); 
    packet_num=0;
    
    outport((int*)0x00681080, 0x000000FF);
    //main loop
    packetCount = 0;
    
    readImage();
    printf("Ready to receive.");
    while (1)
    {
        printf("State: %u\n",currentState);
        switch (currentState) {
            case IDLE:

                msleep(100);
                break;
                
            case TRANNY:            
                if (timeout < 100) {
                    timeout++;
                } else {
                    currentState = IDLE;
                    timeout = 0;
                    packetCount = 0;
                }
                msleep(100);                
                break;

            default:

                msleep(100);
                printf("\nbroken state machine\n\n");
                break;
        }
    }

    return 0;
}
//-------------------------------------------------------------------------
