#include "basic_io.h"
#include "test.h"
#include "LCD.h"
#include "DM9000A.C"

#include "VGA.C"
#include "HAL4D13.C"
#include "isa290.h"




#define LEDR_BASE_ADDRESS 0x006781070 //from SOPC builder
#define SW_BASE_ADDRESS   0x0067810A0 //from SOPC builder
#define LEDG_BASE_ADDRESS = 0x00681080

unsigned int aaa,rx_len,i,packet_num;
unsigned char RXT[68];


unsigned char get_sw();

char incomingPacket[1319];
char outgoingPacket[1319];
unsigned int receivedPackets = 0;
int ackCounter = 0;
unsigned int encodedNum=0;

char imageArray[38400];

int ackCount = 0;


typedef enum {IDLE, RECEIVING} status;
status currentState = IDLE;









unsigned char rleArray[38400];
unsigned int rleByteIndex = 0;
unsigned int outputByteIndex = 0;
unsigned int outputPixelIndex = 0;
unsigned int repeatCount;
unsigned int currentPixel;
unsigned int r;
unsigned int outputArray[38400];



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




void set_leds(short led_data) {
//    printf("LED DATA: %x\n", led_data);
    outport((int*)0x00681070, (led_data & 0x000000FF));
}

unsigned char get_sw() {
    unsigned char sw_data;
    sw_data = (unsigned char)(inport((int*)0x006810A0) & 0x000000FF);
//    printf("SW DATA: %x\n", sw_data);
    return sw_data;
}

void flashGreenLEDs() {
    outport((int*)0x00681080, 0x000000FF);
	msleep(10);
	outport((int*)0x00681080, 0x00000000);
}

void flashRedLEDs() {
    outport((int*)0x00681070, 0x000000FF);
	msleep(10);
	outport((int*)0x00681070, 0x00000000);
}

void decodeThis() {
    
for (rleByteIndex = 0 ; rleByteIndex < sizeof(rleArray) ; rleByteIndex+=3) {
    currentPixel = ((rleArray[rleByteIndex] & (1 << 7)) != 0); //is this a white or black pixel
    repeatCount = ((rleArray[rleByteIndex] & (0 << 7)) << 16) + (rleArray[rleByteIndex+1] << 8) + rleArray[rleByteIndex+2]; //slice up the rest of the bytez
    for (r = 0; r < repeatCount; r++) {
        //which output byte are we on
        outputByteIndex = (outputPixelIndex-(outputPixelIndex%8))/8;
        //throw that pixel into the array
        outputArray[outputByteIndex] |=  (1 << (outputPixelIndex%8));
        outputPixelIndex++;
    }
}
}
void sendAck() {
    //load headers, clean container
    int i;
    for (i = 0; i <= 1319; i++) {
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
    int i;
    for (i = 0; i <= 1319; i++) {
        if (i < 38) {
            outgoingPacket[i] = packetHeaders[i];
        } else {
            outgoingPacket[i] = 0;
        }
    }
    outgoingPacket[38] = 0xAA;
    TransmitPacket(outgoingPacket, sizeof(outgoingPacket));
    msleep(100);
    printf("Sent app ACK\n");
}

void displayImage() {
    unsigned char currentPixel;
	printf("Reading binary pixel from flash memory\n");
	int offset = 0;
    int i = 0;
    int j = 0;
    
	for(i = 0 ; i < 480 ; i++) {
		for(j = 0 ; j < 640 ; j++) {
			currentPixel = (imageArray[offset] >> (7-(j%8))) & 0x01;
			if(currentPixel) {
				Vga_Set_Pixel(VGA_0_BASE,j,i);
			} else {
				Vga_Clr_Pixel(VGA_0_BASE,j,i);
			}
            
	
			if (j % 8 == 7) {
				offset++;
			}
		}
	}
}

void initializePacket() {
    //load headers, clean container
    int i;
    for (i = 0; i <= 1319; i++) {
        if (i < 38) {
            outgoingPacket[i] = packetHeaders[i];
        } else {
            outgoingPacket[i] = 0;
        }
    }



    outgoingPacket[38]= get_sw();
    
    //cafe babes
    for (i = 39; i < 1039; i += 4) {
        outgoingPacket[i]   =  0xCA;
        outgoingPacket[i+1] =  0xFE;
        outgoingPacket[i+2] =  0xBA;
        outgoingPacket[i+3] =  0xBE;
    }
}

void ethernet_interrupts()
{
    packet_num++;
    
    
    aaa=ReceivePacket (incomingPacket,&rx_len);
    if(!aaa) {
        unsigned char switchData = incomingPacket[38];
        printf("Decoded Data: %x\n",switchData);
        printf("Babes?: 0x%x \n", (incomingPacket[1035]+1 << 24) + (incomingPacket[1036]+1 << 16) + (incomingPacket[1037]+1 << 8) + incomingPacket[1038]);

        switch (currentState) {
            case IDLE:
                printf("got a packet while idling... wtf\n");
                //if (notAck) sendAck
                break;
            
            case RECEIVING:
                printf("got a packet while receiving\n");
                if (switchData == (unsigned char)(0xF0)) {
                    //dec ack counter
                    ackCounter--;
                    printf("Got ACK\n\n");
					flashGreenLEDs();
					
                } else if (switchData == (unsigned char)(0xAA)) {
                    //dec ack counter
                    ackCounter--;
                    printf("Got App ACK\n\n");
                    sendAck();
					flashGreenLEDs();					
                    //image is done
                    //send layer 5 ack
                    //display image
					printf("FINISHED RECEIVING IMAGE\n\n");

                    receivedPackets = 0;
                    currentState = IDLE;
					
					displayImage();

					

				} else if (switchData == (unsigned char)(0xBE)) {
                    sendAck();
                    
                    if(receivedPackets < encodedNum) {
                        //store pixel data
                        
                        printf("Packet Count: %u\n",receivedPackets);
						int i = 0;
						for (i = 0; i < 1280; i++) {
							decodeThis();
							imageArray[(receivedPackets*1280)+i] = incomingPacket[39+i];
						}
                        receivedPackets++;
						
                    }
                } else if (switchData == (unsigned char)(0xFF)) {
                	printf ("Received NAK packet");
					flashRedLEDs();
                    receivedPackets = 0;
                    currentState = IDLE;
				} else {
					printf ("Received weird packet, incomingPacket[39] = %u\n", switchData);
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

    LCD_Test();
    DM9000_init();
    alt_irq_register( DM9000A_IRQ, NULL, (void*)ethernet_interrupts ); 
    packet_num=0;
    char swData;
	
	//setup VGA output
	VGA_Ctrl_Reg vga_ctrl_set;
	vga_ctrl_set.VGA_Ctrl_Flags.RED_ON	= 1;
	vga_ctrl_set.VGA_Ctrl_Flags.GREEN_ON  = 1;
	vga_ctrl_set.VGA_Ctrl_Flags.BLUE_ON   = 1;
	vga_ctrl_set.VGA_Ctrl_Flags.CURSOR_ON = 1;
  
	Vga_Write_Ctrl(VGA_0_BASE, vga_ctrl_set.Value);
	Set_Pixel_On_Color(1023,1023,1023);
	Set_Pixel_Off_Color(0,0,0);
	Set_Cursor_Color(0,1023,0);

	unsigned int i,j;
   
	for(i = 0 ; i < 480 ; i++) {
		for(j = 0 ; j < 640 ; j++) {
			if((j % 8) == 0)Vga_Set_Pixel(VGA_0_BASE,j,i);
			else Vga_Clr_Pixel(VGA_0_BASE,j,i);
		}
	}
	


    //main loop
    while (1)
    {
        printf("State: %u\n",currentState);
        switch (currentState) {
            case IDLE:
                printf("Press enter to send...\n");
                scanf("%d",&swData);

                initializePacket();
                printf("Packet (almost) sent.\n\n");

                currentState = RECEIVING;
                TransmitPacket(outgoingPacket, sizeof(outgoingPacket));
                ackCounter++;
                msleep(100);

                break;
                
            case RECEIVING:
				if (ackCount < 100) {
					ackCount++;
				} else {
					ackCount = 0;
					receivedPackets = 0;
					currentState = IDLE;
				}
                msleep(100);
                break;
                
                
            default:
                initializePacket();
                TransmitPacket(outgoingPacket, sizeof(outgoingPacket));
                msleep(100);
                printf("\nSent packet in broken state machine\n\n");
                break;
        }
    }

    return 0;
}
//-------------------------------------------------------------------------
