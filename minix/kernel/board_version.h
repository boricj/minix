#include "mailbox.h"

/*currently arbitarirtyliy chosen*/
#define BUFFER_ADDRESS 0x1000

#define RR_REQUEST 0x00000000
#define RR_RESPONSE_OK 0x80000000
#define RR_RESPONSE_ERROR 0x80000001

#define SLOT_OVERALL_LENGTH 0
#define SLOT_RR 1
#define SLOT_TAGSTART 2

#define SLOT_TAG_ID 0
#define SLOT_TAG_BUFLEN 1
#define SLOT_TAG_DATALEN 2
#define SLOT_TAG_DATA 3

#define MBOX_HEADER_LENGTH 2
#define TAG_HEADER_LENGTH 3

#define MBX_DEVICE_SDCARD 0x00000000
#define MBX_DEVICE_UART0 0x00000001
#define MBX_DEVICE_UART1 0x00000002
#define MBX_DEVICE_USBHCD 0x00000003
#define MBX_DEVICE_I2C0 0x00000004
#define MBX_DEVICE_I2C1 0x00000005
#define MBX_DEVICE_I2C2 0x00000006
#define MBX_DEVICE_SPI 0x00000007
#define MBX_DEVICE_CCP2TX 0x00000008

#define MBX_TAG_GET_BOARD_REVISION 0x00010002

//volatile uint32_t *mailbuffer = (uint32_t *) BUFFER_ADDRESS;
volatile uint32_t mailbuffer[50] __attribute((aligned(16))); 

uint32_t RPI2_REVISIONS[] = {0xa01040, 0x01041, 0x21041, 0x22042};
#define NO_RPI2_REVISIONS 4

uint32_t RPI3_REVISIONS[] = { 0xa02082, 0xa020a0, 0xa22082, 0xa32082};
#define NO_RPI3_REVISIONS 4


void add_mailbox_tag(volatile uint32_t* buffer, uint32_t tag, uint32_t buflen, uint32_t len, uint32_t* data) {
  volatile uint32_t* start = buffer + SLOT_TAGSTART;
  start[SLOT_TAG_ID] = tag;
  start[SLOT_TAG_BUFLEN] = buflen;
  start[SLOT_TAG_DATALEN] = len & 0x7FFFFFFF;

  uint32_t bufwords = buflen >> 2;

  if (0 == data) {
    for (int i = 0; i < bufwords; ++i) {
      start[SLOT_TAG_DATA + i] = 0;
    }
  } else {
    for (int i = 0; i < bufwords; ++i) {
      start[SLOT_TAG_DATA + i] = data[i];
    }
  }

  start[SLOT_TAG_DATA+bufwords] = 0; // end of tags, unless overwritten later
}


void build_mailbox_request(volatile uint32_t* buffer) {
  uint32_t tag_length = buffer[MBOX_HEADER_LENGTH + SLOT_TAG_BUFLEN];
  uint32_t end = (MBOX_HEADER_LENGTH*4) + (TAG_HEADER_LENGTH*4) + tag_length;
  uint32_t overall_length = end + 4;
  buffer[SLOT_OVERALL_LENGTH] = overall_length;
  buffer[SLOT_RR] = RR_REQUEST;
}



int get_parameter(const char* name, uint32_t tag, int nwords) {
  add_mailbox_tag(mailbuffer, tag, nwords * 4, 0, 0);
  build_mailbox_request(mailbuffer);


  writemailbox(8, (uint32_t)mailbuffer);
  readmailbox(8);
   if(mailbuffer[1] != 0x80000000) {
  	return 1;  
  } else {
  
	return 0;
	}   
  

}

int get_board_revision(){

	if(get_parameter("board rev", MBX_TAG_GET_BOARD_REVISION, 1)){
		/*error*/
	}
	else{
		uint32_t value = mailbuffer[MBOX_HEADER_LENGTH + TAG_HEADER_LENGTH ];
		for(int _i = 0; _i<NO_RPI2_REVISIONS; _i++){
			if(value == RPI2_REVISIONS[_i])
				return 2;
		}
		for(int _i = 0; _i<NO_RPI3_REVISIONS; _i++){
			if(value == RPI3_REVISIONS[_i])
				return 3;
		}
	}
	return -1;
}

