#include <stdint.h>
#include "barrier.h"
static volatile unsigned int *MAILBOX0READ = (unsigned int *) 0x3F00b880;
static volatile unsigned int *MAILBOX0STATUS = (unsigned int *) 0x3F00b898;
static volatile unsigned int *MAILBOX0WRITE = (unsigned int *) 0x3F00b8a0;





#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000



uint32_t readmailbox(uint32_t channel);
void writemailbox(uint32_t channel, uint32_t data);

uint32_t readmailbox(uint32_t channel) {
  uint32_t count = 0;
  volatile uint32_t data;

	/* Loop until something is received from channel
	 * If nothing recieved, it eventually give up and returns 0xffffffff
	 */
 		
	while(1) {
		while (*MAILBOX0STATUS & MAILBOX_EMPTY) {
			/* Need to check if this is the right thing to do */
			_flushcache();

			/* This is an arbritarily large number */
			if(count++ >(1<<10)) {
				return 0xffffffff;
			}
		}

		/* Read the data
		 * Data memory barriers as we've switched peripheral
		 */
		_dmb();
		data = *MAILBOX0READ;
		_dmb();
		if ((data & 15) == channel)
			return data;
	}

	return 0;
}

void writemailbox(uint32_t channel, uint32_t data) {
	/* Wait for mailbox to be not full */
	while (*MAILBOX0STATUS & MAILBOX_FULL) 	{
		/* Need to check if this is the right thing to do */
		_flushcache();
	}

	_dmb();
	*MAILBOX0WRITE = (data | channel);
}

