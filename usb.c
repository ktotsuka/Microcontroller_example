#include "stm32f4xx.h"
#include "stm32f4xx_usart.h"
#include "stdio.h"
#include "stdlib.h"
#include "usb.h"
#include "string.h"
#include "main.h"

////Variables
uint8_t USBBuffer[USBBUFSIZE];
uint8_t USBReadBuf[USB_RBUF_SIZE];
int32_t USBBufInIndex;
int32_t USBBufOutIndex;
int32_t USBBufDiffIndex;
int32_t RunApplication;
int32_t NumVarPlot[NUM_CAPTURE_PERIOD] = {0, 0, 0, 0};
int32_t NumVarNonPlot = 0;
uint32_t* VarAddressNonPlot[30];
uint32_t* VarAddress[NUM_CAPTURE_PERIOD][30];

////Functions
void USBWriteByteBuffer(uint8_t data) {
	if (data == PACKET_START)
	{
	   // Protect buffer
		__disable_irq();
	}

	USBBuffer[USBBufInIndex] = data;
	USBBufInIndex++;
	if (USBBufInIndex >= USBBUFSIZE) {
		USBBufInIndex = 0;
	}

	if (data == PACKET_END)
	{
	   // Protection finished
		__enable_irq();
    }
}

void USBSend(void) {
	int32_t i;

	for (i = 0;i < 10;i++)
	{
		USBBufDiffIndex = USBBufInIndex - USBBufOutIndex;
		if (USBBufDiffIndex < 0)
		{
			USBBufDiffIndex = USBBufDiffIndex + USBBUFSIZE;
		}
		if (USBBufDiffIndex > (USBBUFSIZE*8/10))
		{
			SimpleDebug('O'); SimpleDebug('v'); SimpleDebug('e'); SimpleDebug('r'); SimpleDebug('f'); SimpleDebug('l'); SimpleDebug('o'); SimpleDebug('w'); SimpleDebug('\n'); SimpleDebug('\r');
		}
		if (USBBufDiffIndex > 0)
		{
			while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
			USART_SendData(USART2, USBBuffer[USBBufOutIndex]);
			USBBufOutIndex++;
			if (USBBufOutIndex >= USBBUFSIZE)
			{
				USBBufOutIndex = 0;
			}
		}
	}
}

void HandleReceivedData() {
	uint32_t temp[4];
	uint8_t header;
	uint32_t* p_data;
	int32_t i;
	int32_t capture_period_num;

	// Get header
	header = USBReadBuf[0];
	// Check command type
	if (header == data_changed) {
		SimpleDebug('c'); SimpleDebug('h'); SimpleDebug('a'); SimpleDebug('n'); SimpleDebug('g'); SimpleDebug('e'); SimpleDebug('\n'); SimpleDebug('\r');
		//USBWriteBufferMessage("Data changed", 0);

		// Get address
		p_data = (uint32_t*)((((uint32_t)USBReadBuf[4] << 24) & 0xFF000000) | (((uint32_t)USBReadBuf[3] << 16) & 0x00FF0000) | (((uint32_t)USBReadBuf[2] << 8) & 0x0000FF00) | ((unsigned int)USBReadBuf[1] & 0x000000FF));

		// Get the data
		temp[0] = ((uint32_t)USBReadBuf[5])&0x000000FF;
		temp[1] = (((uint32_t)USBReadBuf[6])<<8)&0x0000FF00;
		temp[2] = (((uint32_t)USBReadBuf[7])<<16)&0x00FF0000;
		temp[3] = (((uint32_t)USBReadBuf[8])<<24)&0xFF000000;
		// Write data
		*p_data = temp[0] + temp[1] + temp[2] + temp[3];
		// Write acknowledge to buffer
		USBWriteBufferAcknowledge((uint32_t)p_data);
	}
	else if (header == bootloader) {
		// Go to Bootloader (For now just reset)
		SimpleDebug('r'); SimpleDebug('e'); SimpleDebug('s'); SimpleDebug('e'); SimpleDebug('t');
		USBWriteBufferMessage("Reseting", 0);
		// Wait for debug output to go through
		for (i = 0; i < 10000; i++)
		{

		}
		NVIC_SystemReset();
	}
	else if (header == assign_non_plot) {
		//SimpleDebug('A'); SimpleDebug('N');SimpleDebug('\n'); SimpleDebug('\r');
		//USBWriteBufferMessage("Assigning non plot items", 0);

		// Get number of variables
		NumVarNonPlot = (int32_t)USBReadBuf[1];

		// Receive addresses of variables
		for (i = 0; i < NumVarNonPlot; i++) {
			// Get address
			VarAddressNonPlot[i] = (uint32_t*)((((uint32_t)USBReadBuf[5 + VAR_SIZE*i] << 24) & 0xFF000000) | (((uint32_t)USBReadBuf[4 + VAR_SIZE*i] << 16) & 0x00FF0000) | (((uint32_t)USBReadBuf[3 + VAR_SIZE*i] << 8) & 0x0000FF00) | ((unsigned int)USBReadBuf[2 + VAR_SIZE*i] & 0x000000FF));
		}
	}
	else if (header == assign_plot) {
		// Get capture period number
		capture_period_num = (int32_t)USBReadBuf[1];
		// Get number of variables
		NumVarPlot[capture_period_num] = (int32_t)USBReadBuf[2];
		// Receive addresses of variables
		for (i = 0; i < NumVarPlot[capture_period_num]; i++) {
			// Get address
			VarAddress[capture_period_num][i] = (uint32_t*)((((uint32_t)USBReadBuf[6 + VAR_SIZE*i] << 24) & 0xFF000000) | (((uint32_t)USBReadBuf[5 + VAR_SIZE*i] << 16) & 0x00FF0000) | (((uint32_t)USBReadBuf[4 + VAR_SIZE*i] << 8) & 0x0000FF00) | ((unsigned int)USBReadBuf[3 + VAR_SIZE*i] & 0x000000FF));
		}
	}
	else {
		USBWriteBufferMessage("Unhandled case", 0);
		SimpleDebug('n'); SimpleDebug('o'); SimpleDebug('n'); SimpleDebug('\n'); SimpleDebug('\r');
	}
}

void USBWriteBufferPlot(int32_t capture_period_num)
{
	int32_t i;
	uint8_t header;
	uint32_t data;

	// Make sure that there is data to send
	if (NumVarPlot[capture_period_num] > 0) {
		// Write start byte
		USBWriteByteBuffer(PACKET_START);
		// Put remaining number of bytes
		USBWriteByteBuffer((uint8_t)(3 + NumVarPlot[capture_period_num]*VAR_SIZE)); // 1: header, 1: capture period number, NumVarPlot[num]*VAR_SIZE: data, 1: end mark
		// Create header
		header = data_plot;
		// Write header
		USBWriteByteBuffer(header);
		// Put capture period num
		USBWriteByteBuffer((uint8_t)capture_period_num);
		// Write data for each variable
		for (i = 0; i < NumVarPlot[capture_period_num]; i++) {
			// Get value
			data = *VarAddress[capture_period_num][i];
			// Write value
			USBWriteByteBuffer((uint8_t)(data));
			USBWriteByteBuffer((uint8_t)(data>>8));
			USBWriteByteBuffer((uint8_t)(data>>16));
			USBWriteByteBuffer((uint8_t)(data>>24));
		}
		// Write end bytes
		USBWriteByteBuffer(PACKET_END);
	}
}

void USBWriteBufferMessage(char* str, int32_t value) {
	int32_t str_len;
	int32_t i;
	uint8_t header;

	// Get string length
	str_len = strlen(str);

	// Write start byte
	USBWriteByteBuffer(PACKET_START);
	// Put remaining number of bytes
	USBWriteByteBuffer((uint8_t)(6 + str_len)); // 1: header, 1: end mark, 4: value, num_char: message
	// Create header
	header = message;
	// Write header
	USBWriteByteBuffer(header);
	// Write message
	for (i = 0; i < str_len; i++)
	{
		USBWriteByteBuffer(str[i]);
	}
	// Write value
	USBWriteByteBuffer((uint8_t)(((uint32_t)value) >> 0));
	USBWriteByteBuffer((uint8_t)(((uint32_t)value) >> 8));
	USBWriteByteBuffer((uint8_t)(((uint32_t)value) >> 16));
	USBWriteByteBuffer((uint8_t)(((uint32_t)value) >> 24));

	// Write end bytes
	USBWriteByteBuffer(PACKET_END);
}

void USBWriteBufferNonPlot() {
	int32_t i;
	uint8_t header;
	uint32_t data;

	// Make sure that variables are assigned
	if (NumVarNonPlot == 0) {
		return;
	}

	// Write start byte
	USBWriteByteBuffer(PACKET_START);
	// Put remaining number of bytes
	USBWriteByteBuffer((uint8_t)(2 + NumVarNonPlot*VAR_SIZE)); // 1: header, NomVarNonPlot*4: data, 1: end mark
	// Create header
	header = data_non_plot;
	// Write header
	USBWriteByteBuffer(header);
	// Write data for each variable
	for (i = 0; i < NumVarNonPlot; i++) {
		// Get value
		data = *VarAddressNonPlot[i];
		// Write value
		USBWriteByteBuffer((uint8_t)(data));
		USBWriteByteBuffer((uint8_t)(data>>8));
		USBWriteByteBuffer((uint8_t)(data>>16));
		USBWriteByteBuffer((uint8_t)(data>>24));
	}
	// Write end bytes
	USBWriteByteBuffer(PACKET_END);
}

void USBWriteBufferAcknowledge(uint32_t address) {
	uint8_t header;

	// Write start byte
	USBWriteByteBuffer(PACKET_START);
	// Put remaining number of bytes
	USBWriteByteBuffer((uint8_t)(6)); // 1: header, 4: address, 1: end mark
	// Create header
	header = acknowledge;
	// Write header
	USBWriteByteBuffer(header);
	// Write address
	USBWriteByteBuffer((uint8_t)(address));
	USBWriteByteBuffer((uint8_t)(address>>8));
	USBWriteByteBuffer((uint8_t)(address>>16));
	USBWriteByteBuffer((uint8_t)(address>>24));
	// Write end bytes
	USBWriteByteBuffer(PACKET_END);
}

//////////////////  Interrupts /////////////////////////////////////
// USART2 interrupts.
// Could be caused by several reasons.
void USART2_IRQHandler()
{
   // Check if data was received
   if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
   {
      uint8_t test_byte;
      int32_t num_rem_bytes;
      int32_t i;

      // Get the first byte
      test_byte = (uint8_t)USART_ReceiveData(USART2);

      // Check the first byte
      if (test_byte == PACKET_START)
      {
         // Get number of remaining bytes
         while (USART_GetITStatus(USART2, USART_IT_RXNE) == RESET)
         {

         }
         num_rem_bytes = (uint8_t)USART_ReceiveData(USART2);

         // Receive remaining bytes
         for (i = 0; i < num_rem_bytes; i++)
         {
            while (USART_GetITStatus(USART2, USART_IT_RXNE) == RESET)
            {
            }
            USBReadBuf[i] = (uint8_t)USART_ReceiveData(USART2);
         }
         // Handle data
         HandleReceivedData();
      }
      else if (test_byte == 8)
      {
          // Run application
          RunApplication = TRUE;
      }
      // Clear USART2 RX non-empty interrupt flag
      USART_ClearFlag(USART2,USART_FLAG_RXNE);
   }

   if(USART_GetITStatus(USART2, USART_IT_PE) != RESET)
   {
	   SimpleDebug('P'); SimpleDebug('E'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
   {
   	   SimpleDebug('T'); SimpleDebug('X'); SimpleDebug('E'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)
   {
       SimpleDebug('T'); SimpleDebug('C'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_ORE_RX) != RESET)
   {
       SimpleDebug('O'); SimpleDebug('R'); SimpleDebug('E'); SimpleDebug('R');  SimpleDebug('X'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
   {
       SimpleDebug('I'); SimpleDebug('D'); SimpleDebug('L'); SimpleDebug('E'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_LBD) != RESET)
   {
       SimpleDebug('L'); SimpleDebug('B'); SimpleDebug('D'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_CTS) != RESET)
   {
       SimpleDebug('C'); SimpleDebug('T'); SimpleDebug('S');  SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_ORE_ER) != RESET)
   {
       SimpleDebug('O'); SimpleDebug('R'); SimpleDebug('E'); SimpleDebug('E');  SimpleDebug('R'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_NE) != RESET)
   {
       SimpleDebug('N'); SimpleDebug('E'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_FE) != RESET)
   {
       SimpleDebug('F'); SimpleDebug('E'); SimpleDebug('\n'); SimpleDebug('\r');
   }

   if(USART_GetITStatus(USART2, USART_IT_ORE) != RESET)
   {
       SimpleDebug('O'); SimpleDebug('R'); SimpleDebug('E'); SimpleDebug('\n'); SimpleDebug('\r');
   }
}



