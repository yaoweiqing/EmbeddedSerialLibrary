#include "serial_protocol.h"
#include "circular_fifo.h"

CircularFifo fifoIn, fifoOut;
uint8_t dataBufferIn[FIFO_IN_SIZE];
uint8_t dataBufferOut[FIFO_OUT_SIZE];

static uint64_t lastTimeDataReceived = 0;

static bool atModeEnable = false;

static SerialMode serialMode = {0};
static PacketDefinition packetDefinition = {0};

void (*processRawDataCallBack)() = NULL;
void (*processCommandCallBack)() = NULL;
void (*processBinaryPacketCallBack)() = NULL;

void (*sedDataCallBack)(uint8_t *data, uint32_t size) = NULL;

void initSerialProtocol() {
	initSerialUsr();
	initCircularFifo(&fifoIn, dataBufferIn, FIFO_IN_SIZE);
	initCircularFifo(&fifoOut, dataBufferOut, FIFO_OUT_SIZE);
	serialMode = SERIAL_MODE_TRANSPARENT;
	packetDefinition.isConfigured = false;
}

uint32_t bytesAvailable() {
	return getSize(&fifoIn);
}

void setSerialMode(SerialMode mode) {
	serialMode = mode;
}

SerialMode getSerialMode() {
	return serialMode;
}

void setPacketDefinition(PacketDefinition definition) {
	packetDefinition = definition;
}

PacketDefinition getPacketDefinition() {
	return packetDefinition;
}

inline void dataReceived(uint8_t *data, uint32_t size) {
	bool sizeEnough = ((size + getNumElements(&fifoIn)) < getSize(&fifoIn));
	if(sizeEnough) {
		switch(serialMode) {
			case SERIAL_MODE_COMMAND:

			break;
			case SERIAL_MODE_TRANSPARENT:
				pushMultiple(&fifoIn, data, size);
				if(processRawDataCallBack) {
					processRawDataCallBack();
				}
			break;
			case SERIAL_MODE_BINARY_PACKET:
				pushMultiple(&fifoIn, data, size);
				if(processBinaryPacketCallBack) {
					processBinaryPacketCallBack();
				}
			break;
		}
	}
	else {
		/* Jump to Hardfault */
		printf("[dataReceived] FIFO IN overflow.\n");
	}
}

/* 
   Read number of bytes passed on size variable and
   return number of bytes read. 
*/
uint32_t read(uint8_t *data, uint32_t size) {
	if(bytesAvailable() >= size) {
		popMultiple(&fifoIn, data, size);
	}
	return 0;
}

/* 
  Read next AT comamnd. Command String must
  be able to accommodate the longest command
  string possible. Format: AT[COMMAND]\n
  Return if it was possible to read a AT command from the buffer 
*/
bool readNextCommand(char *commandString) {
	return false;
}

/* 
    Try to Read the next binary packet. Packet size must be specified
    in the variable packetSize. This function returns the number of bytes 
    read.
*/
uint32_t readNextPacket(void *packet) {
	if(packetDefinition.isConfigured) {
		if(packetDefinition.isPayloadSizeFixed) {
			uint16_t packetSize = packetDefinition.headerSize + packetDefinition.payloadSize;
			if(getSize(&fifoIn) >= packetSize) {
				popMultiple(&fifoIn, (uint8_t*)packet, packetSize);
				return packetSize;
			}
			else {
				return 0;
			}
		}
		else {
			if(getSize(&fifoIn) >= packetDefinition.headerSize) {
				peakMultiple(&fifoIn, (uint8_t*)packet, packetDefinition.headerSize);
				uint16_t payloadSize = *((((uint8_t*)packet) + packetDefinition.payloadSizeOffset));
				uint16_t packetSize = payloadSize + packetDefinition.headerSize;
				if(getSize(&fifoIn) >= packetSize) {
					peakMultiple(&fifoIn, ((uint8_t*)packet+packetDefinition.headerSize), payloadSize);
					discardMultiple(&fifoIn, packetSize);
					return packetSize;
				}
				else {
					return 0;
				}
			}
		}
	}
	return 0;
}

/* Get current serial mode */
SerialMode getCurrentSerialMode() {
	return serialMode;
}

bool isAtCommandEnable() {
	return atModeEnable;
}

void setAtCommandEnable(bool enable) {
	atModeEnable = enable;
}






