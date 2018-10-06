// Constants
#define USBBUFSIZE          2000
#define USB_RBUF_SIZE       200
#define FALSE               0
#define TRUE                1
#define PACKET_START        0xAB
#define PACKET_END          0xCD
#define NUM_CAPTURE_PERIOD  4
#define VAR_SIZE            4

// Special
enum to_micro_header {
	data_changed = 1,
	bootloader = 2,
	assign_plot = 4,
	assign_non_plot = 8,
};

enum from_micro_header {
	message = 1,
	data_plot = 2,
	data_non_plot = 4,
	acknowledge = 8,
};

// Function definitions
void USBSend(void);
void HandleReceivedData(void);
void USBWriteByteBuffer(uint8_t data);
void USBWriteBufferPlot(int32_t capture_period_num);
void USBWriteBufferNonPlot();
void USBWriteBufferAcknowledge(uint32_t address);
void USBWriteBufferMessage(char* str, int32_t value);
