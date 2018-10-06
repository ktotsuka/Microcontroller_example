
// Timer2: 1ms interrupt
// Timer3: Used for ADC sampling
// Timer4: Frequency generator
// Timer5: This is a 32bit timer.  Used for time stamp
// USART2: Used for USB communication
// USART3: Used for low level debug output
// ADC1: Primary sampling source
// ADC2: Secondary sampling source
// ADC3: Tertiary sampling source
// A2: USART2 TX pin
// A3: USART2 RX pin
// A4: DAC1
// A5: DAC2
// C0: ADC Ch10
// C1: ADC Ch11
// C2: ADC Ch12
// D1: MUX select ch1
// D2: MUX select ch2
// D3: MUX select ch3
// D8: USART3 TX pin
// D9: USART3 RX pin
// D11: Frequency generation
// D12 ~ D15: Digital output for LED (only on the eval board)

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_dac.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_tim.h"
#include "stdio.h"
#include "stdlib.h"
#include "misc.h"
#include "usb.h"
#include "main.h"
#include "data.h"

// Function prototypes
void ConfigPins(void);
void ConfigUSART2(void);
void ConfigUSART3(void);
void ConfigTimer2(void);
void ConfigTimer3(void);
void ConfigTimer4(void);
void ConfigTimer5(void);
void ConfigDAC1(void);
void ConfigDAC2(void);
void ConfigADC(void);
void UpdateSettings(void);
void FinishSampling(void);
void ResetSampling(void);
float ConvertADC_ValueToVoltage(uint16_t adc_value, uint8_t channel);

// Data
uint16_t ADCConvertedValues[ADC_BUFFER_SIZE];
extern int32_t RunApplication;

int main(void)
{
	// Set up the system clocks
	SystemInit();

	// Initialize NVIC setting
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	// Configure stuff
	ConfigPins();
	ConfigUSART2();
	ConfigUSART3();
	ConfigDAC1();
	ConfigDAC2();

	// Write DAC
	DAC_SetChannel1Data(DAC_Align_12b_R, (uint16_t)DAC1_value);
	DAC_SetChannel2Data(DAC_Align_12b_R, (uint16_t)DAC2_value);

	SimpleDebug('\n'); SimpleDebug('\r');
	SimpleDebug('R'); SimpleDebug('e'); SimpleDebug('a'); SimpleDebug('d'); SimpleDebug('y'); SimpleDebug('\n'); SimpleDebug('\r');

	// Wait for go signal from GUI
	while (RunApplication == 0)
	{
	}

	SimpleDebug('s'); SimpleDebug('t'); SimpleDebug('a'); SimpleDebug('r'); SimpleDebug('t'); SimpleDebug('\n'); SimpleDebug('\r');

	USBWriteBufferMessage("Application started", 0);

	// Configure Timers
	ConfigTimer2();
	ConfigTimer5();

	while(1)
	{
		USBSend();
	}
}

void ConfigPins(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	// Enable clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	// Configure pins for output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void ConfigUSART2(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	// A2 = USART2 TX pin, A3 = USART2 RX pin
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;  //enable Port A2 and A3 pins
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //set pin speed...ok
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  		//Set up for alternative function (Can be input, output, analog or alternative function)
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  	//output is push-pull, can be push-pull or open-drain
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    	//Can be pull-up, pull-dpwn or no-pull
	GPIO_Init(GPIOA, &GPIO_InitStructure);  			//Initialize the pin with these settings
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  //configures pins for alternative function, in this case USART2 TX pin to A2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);  //configures pins for alternative function, in this case USART2 RX pin to A3

	// Configure USART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	USART_InitStruct.USART_BaudRate = 1250000;				//
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	USART_InitStruct.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	USART_InitStruct.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART2, &USART_InitStruct);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting

	// Set up USART2 RX interrupt
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); // Enable RX interrupt for USART2
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  // Set up interrupt priority
	USART_Cmd(USART2, ENABLE); // Enable USART2
}

void ConfigUSART3(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStruct;

	// D8 = USART3 TX pin, D9 = USART3 RX pin
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //set pin speed...ok
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  		//Set up for alternative function (Can be input, output, analog or alternative function)
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  	//output is push-pull, can be push-pull or open-drain
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    	//Can be pull-up, pull-dpwn or no-pull
	GPIO_Init(GPIOD, &GPIO_InitStructure);  			//Initialize the pin with these settings
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);  //configures pins for alternative function
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);  //configures pins for alternative function

	// Configure USART3
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	USART_InitStruct.USART_BaudRate = 921600;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	USART_InitStruct.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	USART_InitStruct.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	USART_InitStruct.USART_Mode = USART_Mode_Tx; // we want to enable the transmitter
	USART_Init(USART3, &USART_InitStruct);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting
	// Enable USART3
	USART_Cmd(USART3, ENABLE);
}

void ConfigTimer2(void)
{
   NVIC_InitTypeDef NVIC_InitStructure;
   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

   // Enable the TIM2 global Interrupt
   NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   // TIM clock enable
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

   // Time base configuration
   TIM_TimeBaseStructure.TIM_Period = 1000- 1; // 1 MHz down to 1 KHz (1 ms)
   TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1; // Down to 1 MHz (adjust per your clock)
   TIM_TimeBaseStructure.TIM_ClockDivision = 0;
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
   TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

   // TIM enable counter
   TIM_Cmd(TIM2, ENABLE);

   // Enable update interrupt
   TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void ConfigTimer3(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	// Enable the TIM global Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// TIM clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	// Time base configuration
	TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
	TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)TimerPreScaler; // Down to 10 KHz (adjust per your clock)
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	if ((SampleMode == SAMPLE_MODE_TIMER) || (SampleMode == SAMPLE_MODE_TIMER_WITH_TRIGGER))
	{
		// TIM enable counter
		TIM_Cmd(TIM3, ENABLE);
	}

	// Enable update interrupt
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}

void ConfigTimer4(void)
{
   NVIC_InitTypeDef NVIC_InitStructure;
   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

   // Enable the TIM global Interrupt
   NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   // TIM clock enable
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

   // Time base configuration
   TIM_TimeBaseStructure.TIM_Period = FreqGenerationPeriod; // 500-1 = 1kHz, 50-1 = 10kHz, 5-1 = 100kHz (1MHz/((FreqGenerationPeriod + 1)*2))
   TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1; // Down to 1 MHz
   TIM_TimeBaseStructure.TIM_ClockDivision = 0;
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
   TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

   // TIM enable counter
   TIM_Cmd(TIM4, ENABLE);

   // Enable update interrupt
   TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
}

void ConfigTimer5(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	// TIM clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	// Time base configuration
	TIM_TimeBaseStructure.TIM_Period = 0x7FFFFFFF;
	TIM_TimeBaseStructure.TIM_Prescaler = 1 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	// TIM enable counter
	TIM_Cmd(TIM5, ENABLE);
}

void ConfigDAC1(void)
{
	DAC_InitTypeDef  DAC_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;

	// Enable clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	// Configure PA4 for DAC channel 1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    	//Can be pull-up, pull-dpwn or no-pull
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //set pin speed...ok
	GPIO_Init(GPIOA, &GPIO_InitStructure);  			//Initialize the pin with these settings

	// Initialize DAC
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	// Enable DAC channel
	DAC_Cmd(DAC_Channel_1, ENABLE);
}

void ConfigDAC2(void)
{
	DAC_InitTypeDef  DAC_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;

	// Enable clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	// Configure PA5 for DAC channel 2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    	//Can be pull-up, pull-dpwn or no-pull
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //set pin speed...ok
	GPIO_Init(GPIOA, &GPIO_InitStructure);  			//Initialize the pin with these settings

	// Initialize DAC
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);

	// Enable DAC channel
	DAC_Cmd(DAC_Channel_2, ENABLE);
}

void ConfigADC(void)
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	DMA_InitTypeDef       DMA_InitStructure;
	GPIO_InitTypeDef      GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable ADC1, ADC2, DMA2, GPIOC clocks ****************************************/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2 | RCC_APB2Periph_ADC3, ENABLE);

	/* Configure ADC1 = PC0 = Ch10, and ADC2 = PC1 = Ch11, and ADC3 = PC2 = Ch12 ******************************/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	if (SampleMode == SAMPLE_MODE_INTERLEAVE)
	{
		/* DMA configuration **************************************/
		DMA_DeInit(DMA2_Stream0);
		DMA_InitStructure.DMA_Channel = DMA_Channel_0; // Channel 0 is for ADC1
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC_CDR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCConvertedValues;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
		DMA_InitStructure.DMA_BufferSize = NumberOfSamples + EXTRA_SAMPLE_SIZE;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word; // DMA_PeripheralDataSize_HalfWord
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word; // DMA_MemoryDataSize_HalfWord
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
		DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		DMA_Init(DMA2_Stream0, &DMA_InitStructure);
		DMA_Cmd(DMA2_Stream0, ENABLE);

		/* ADC Common Init **********************************************************/
		ADC_CommonInitStructure.ADC_Mode = ADC_TripleMode_Interl;
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
		ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_2;
		ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
		ADC_CommonInit(&ADC_CommonInitStructure);

		/* ADC1 Init ****************************************************************/
		ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
		ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
		ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1; // Not used
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
		ADC_InitStructure.ADC_NbrOfConversion = 1;
		ADC_Init(ADC1, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_3Cycles);

		/* ADC2 Init ****************************************************************/
		ADC_Init(ADC2, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC2, ADC_Channel_10, 1, ADC_SampleTime_3Cycles);

		/* ADC3 Init ****************************************************************/
		ADC_Init(ADC3, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC3, ADC_Channel_10, 1, ADC_SampleTime_3Cycles);

		/* Enable DMA request after last transfer (Single-ADC mode) */
		ADC_MultiModeDMARequestAfterLastTransferCmd(DISABLE);

		/* Enable ADC */
		ADC_Cmd(ADC1, ENABLE);
		ADC_Cmd(ADC2, ENABLE);
		ADC_Cmd(ADC3, ENABLE);

		// Set up ADC interrupt
		ADC_ITConfig(ADC1, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC2, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC3, ADC_IT_OVR, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		// Set up DMA interrupt
		DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_TE, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else if (SampleMode == SAMPLE_MODE_DMA)
	{
		/* DMA configuration **************************************/
		DMA_DeInit(DMA2_Stream0); //Set DMA registers to default values
		DMA_InitStructure.DMA_Channel = DMA_Channel_0; // Channel 0 is for ADC1
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC_CDR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCConvertedValues;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
		DMA_InitStructure.DMA_BufferSize = NumberOfSamples*3;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // DMA_Mode_Normal
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
		DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		DMA_Init(DMA2_Stream0, &DMA_InitStructure);
		DMA_Cmd(DMA2_Stream0, ENABLE);

		/* ADC Common Init **********************************************************/
		ADC_CommonInitStructure.ADC_Mode = ADC_TripleMode_RegSimult;
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
		ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
		ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; // Not used
		ADC_CommonInit(&ADC_CommonInitStructure);

		/* ADC1 Init ****************************************************************/
		ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
		ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
		ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1; // Not used
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
		ADC_InitStructure.ADC_NbrOfConversion = 1;
		ADC_Init(ADC1, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime);

		/* ADC2 Init ****************************************************************/
		ADC_Init(ADC2, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, ADC_SampleTime);

		/* ADC3 Init ****************************************************************/
		ADC_Init(ADC3, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime);

		/* Enable DMA request after last transfer (Single-ADC mode) */
		ADC_MultiModeDMARequestAfterLastTransferCmd(DISABLE);

		/* Enable ADC */
		ADC_Cmd(ADC1, ENABLE);
		ADC_Cmd(ADC2, ENABLE);
		ADC_Cmd(ADC3, ENABLE);

		// Set up ADC interrupt
		ADC_ITConfig(ADC1, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC2, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC3, ADC_IT_OVR, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		// Set up DMA interrupt
		DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_TE, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else if ((SampleMode == SAMPLE_MODE_TIMER) || (SampleMode == SAMPLE_MODE_TIMER_WITH_TRIGGER))
	{
		/* ADC Common Init **********************************************************/
		ADC_CommonInitStructure.ADC_Mode = ADC_TripleMode_RegSimult;
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
		ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
		ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; // I think this is used only for interleaved mode
		ADC_CommonInit(&ADC_CommonInitStructure);

		/* ADC1 Init ****************************************************************/
		ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
		ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
		ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1; // Not used
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
		ADC_InitStructure.ADC_NbrOfConversion = 1;
		ADC_Init(ADC1, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime);

		/* ADC2 Init ****************************************************************/
		ADC_Init(ADC2, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, ADC_SampleTime);

		/* ADC3 Init ****************************************************************/
		ADC_Init(ADC3, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime);

		/* Enable ADC */
		ADC_Cmd(ADC1, ENABLE);
		ADC_Cmd(ADC2, ENABLE);
		ADC_Cmd(ADC3, ENABLE);

		// Set up ADC interrupt
		ADC_ITConfig(ADC1, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC2, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC3, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC3, ADC_IT_EOC, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
	else if (SampleMode == SAMPLE_MODE_DMA_WITH_TRIGGER)
	{
		/* DMA configuration **************************************/
		DMA_DeInit(DMA2_Stream0); //Set DMA registers to default values
		DMA_InitStructure.DMA_Channel = DMA_Channel_0; // Channel 0 is for ADC1
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC_CDR;
		DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCConvertedValues;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
		DMA_InitStructure.DMA_BufferSize = (NumberOfSamples + EXTRA_SAMPLE_SIZE)*3;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // DMA_Mode_Normal
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
		DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		DMA_Init(DMA2_Stream0, &DMA_InitStructure);
		DMA_Cmd(DMA2_Stream0, ENABLE);

		/* ADC Common Init **********************************************************/
		ADC_CommonInitStructure.ADC_Mode = ADC_TripleMode_RegSimult;
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
		ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
		ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; // Not used
		ADC_CommonInit(&ADC_CommonInitStructure);

		/* ADC1 Init ****************************************************************/
		ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
		ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
		ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1; // Not used
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
		ADC_InitStructure.ADC_NbrOfConversion = 1;
		ADC_Init(ADC1, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime);

		/* ADC2 Init ****************************************************************/
		ADC_Init(ADC2, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, ADC_SampleTime);

		/* ADC3 Init ****************************************************************/
		ADC_Init(ADC3, &ADC_InitStructure);
		ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime);

		/* Enable DMA request after last transfer (Single-ADC mode) */
		ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);

		/* Enable ADC */
		ADC_Cmd(ADC1, ENABLE);
		ADC_Cmd(ADC2, ENABLE);
		ADC_Cmd(ADC3, ENABLE);

		// Set up ADC interrupt
		ADC_ITConfig(ADC1, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC2, ADC_IT_OVR, ENABLE);
		ADC_ITConfig(ADC3, ADC_IT_OVR, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		// Set up DMA interrupt
		DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_TE, ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}
}

void SimpleDebug(uint8_t data)
{
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	USART_SendData(USART3, data);
}

void UpdateSettings(void)
{
	int32_t changed = FALSE;

	// Check if any setting has changed
	if (NumberOfSamplesOld != NumberOfSamples)
	{
		changed = TRUE;
	}
	if (SamplingFrequencyHzOld != SamplingFrequencyHz)
	{
		changed = TRUE;
	}
	if (EnableChannel1Old != EnableChannel1)
	{
		changed = TRUE;
	}
	if (EnableChannel2Old != EnableChannel2)
	{
		changed = TRUE;
	}
	if (EnableChannel3Old != EnableChannel3)
	{
		changed = TRUE;
	}
	if (RisingEdgeTriggerEnableOld != RisingEdgeTriggerEnable)
	{
		changed = TRUE;
	}
	if (FallingEdgeTriggerEnableOld != FallingEdgeTriggerEnable)
	{
		changed = TRUE;
	}
	if (NumberOfSamplesBeforeTriggerOld != NumberOfSamplesBeforeTrigger)
	{
		changed = TRUE;
	}
	if (TriggerLevelOld != TriggerVoltageLevel)
	{
		changed = TRUE;
	}
	if ((Ch1Probe10x != Ch1Probe10xOld) || (Ch2Probe10x != Ch2Probe10xOld) || (Ch3Probe10x != Ch3Probe10xOld))
	{
		changed = TRUE;
	}
	// Limit selections
	if (NumberOfSamples < MIN_NUMBER_OF_SAMPLES)
	{
		NumberOfSamples = MIN_NUMBER_OF_SAMPLES;
	}
	if (NumberOfSamples*3 > ADC_BUFFER_SIZE)
	{
		NumberOfSamples = ADC_BUFFER_SIZE / 3;
	}
	if (NumberOfSamplesBeforeTrigger >= NumberOfSamples)
	{
		NumberOfSamplesBeforeTrigger = NumberOfSamples - 1;
	}
	else if (NumberOfSamplesBeforeTrigger < MIN_NUMBER_OF_SAMPLES_BEFORE_TRIGGER)
	{
		NumberOfSamplesBeforeTrigger = MIN_NUMBER_OF_SAMPLES_BEFORE_TRIGGER;
	}
	if (RisingEdgeTriggerEnable == TRUE)
	{
		FallingEdgeTriggerEnable = FALSE;
	}

	// Update setting
	if (changed == TRUE)
	{
		// Update sampling frequency
		if (SamplingFrequencyHz < 1.5)
		{
			SamplingFrequencyHz = 1;
			TimerPeriod = 10000;
			TimerPreScaler = 8400 - 1;
			ADC_SampleTime = ADC_SampleTime_480Cycles;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_TIMER_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_TIMER;
			}
		}
		else if (SamplingFrequencyHz < 15)
		{
			SamplingFrequencyHz = 10;
			TimerPeriod = 1000;
			TimerPreScaler = 8400 - 1;
			ADC_SampleTime = ADC_SampleTime_480Cycles;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_TIMER_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_TIMER;
			}
		}
		else if (SamplingFrequencyHz < 150)
		{
			SamplingFrequencyHz = 100;
			TimerPeriod = 100;
			TimerPreScaler = 8400 - 1;
			ADC_SampleTime = ADC_SampleTime_480Cycles;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_TIMER_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_TIMER;
			}
		}
		else if (SamplingFrequencyHz < 1500)
		{
			SamplingFrequencyHz = 1000;
			TimerPeriod = 10;
			TimerPreScaler = 8400 - 1;
			ADC_SampleTime = ADC_SampleTime_480Cycles;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_TIMER_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_TIMER;
			}
		}
		else if (SamplingFrequencyHz < (ADC_CLK_480*1.5))
		{
			SamplingFrequencyHz = ADC_CLK_480;
			ADC_SampleTime = ADC_SampleTime_480Cycles;
			TimerPeriod = NumberOfSamples - NumberOfSamplesBeforeTrigger;
			TimerPreScaler = (int32_t)((float)PCLK2/SamplingFrequencyHz) - 1;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_DMA_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_DMA;
			}
		}
		else if (SamplingFrequencyHz < (ADC_CLK_144*1.5))
		{
			SamplingFrequencyHz = ADC_CLK_144;
			ADC_SampleTime = ADC_SampleTime_144Cycles;
			TimerPeriod = NumberOfSamples - NumberOfSamplesBeforeTrigger;
			TimerPreScaler = (int32_t)((float)PCLK2/SamplingFrequencyHz) - 1;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_DMA_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_DMA;
			}
		}
		else if (SamplingFrequencyHz < (ADC_CLK_3*1.5))
		{
			SamplingFrequencyHz = ADC_CLK_3;
			ADC_SampleTime = ADC_SampleTime_3Cycles;
			TimerPeriod = NumberOfSamples - NumberOfSamplesBeforeTrigger;
			TimerPreScaler = (int32_t)((float)PCLK2/SamplingFrequencyHz) - 1;
			if ((RisingEdgeTriggerEnable == TRUE) || (FallingEdgeTriggerEnable == TRUE))
			{
				SampleMode = SAMPLE_MODE_DMA_WITH_TRIGGER;
			}
			else
			{
				SampleMode = SAMPLE_MODE_DMA;
			}
		}
		else
		{
			SamplingFrequencyHz = ADC_CLK_INTERLEAVE;
			ADC_SampleTime = ADC_SampleTime_3Cycles;
			SampleMode = SAMPLE_MODE_INTERLEAVE;
			EnableChannel1 = TRUE;
			EnableChannel2 = FALSE;
			EnableChannel3 = FALSE;
			RisingEdgeTriggerEnable = FALSE;
			FallingEdgeTriggerEnable = FALSE;
		}
		// Update stuff
		DurationSec = (float)NumberOfSamples / SamplingFrequencyHz;
		TimeInterval = (float)1/SamplingFrequencyHz;
		if (Ch1Probe10x == FALSE)
		{
			GPIO_ResetBits(GPIOD, GPIO_Pin_1);
			ADC1_Threshold = (int32_t)(TriggerVoltageLevel/GainCh1_1x) + OffsetCh1_1x;
		}
		else
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_1);
			ADC1_Threshold = (int32_t)(TriggerVoltageLevel/GainCh1_10x) + OffsetCh1_10x;
		}
		if (Ch2Probe10x == FALSE)
		{
			GPIO_ResetBits(GPIOD, GPIO_Pin_2);
		}
		else
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_2);
		}
		if (Ch3Probe10x == FALSE)
		{
			GPIO_ResetBits(GPIOD, GPIO_Pin_3);
		}
		else
		{
			GPIO_SetBits(GPIOD, GPIO_Pin_3);
		}

		// Reset old values
		NumberOfSamplesOld = NumberOfSamples;
		SamplingFrequencyHzOld = SamplingFrequencyHz;
		EnableChannel1Old = EnableChannel1;
		EnableChannel2Old = EnableChannel2;
		EnableChannel3Old = EnableChannel3;
		RisingEdgeTriggerEnableOld = RisingEdgeTriggerEnable;
		FallingEdgeTriggerEnableOld = FallingEdgeTriggerEnable;
		NumberOfSamplesBeforeTriggerOld = NumberOfSamplesBeforeTrigger;
		TriggerLevelOld = TriggerVoltageLevel;
		Ch1Probe10xOld = Ch1Probe10x;
		Ch2Probe10xOld = Ch2Probe10x;
		Ch3Probe10xOld = Ch3Probe10x;

		USBWriteBufferMessage("Updated setting", 0);
	}
}

void FinishSampling(void)
{
	DMA_Cmd(DMA2_Stream0, DISABLE);
	ADC_Cmd(ADC1, DISABLE);
	ADC_Cmd(ADC2, DISABLE);
	ADC_Cmd(ADC3, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
  ADC_DeInit();
  DMA_DeInit(DMA2_Stream0);
  TIM_DeInit(TIM3);

  // Disable interrupts
	ADC_ITConfig(ADC1, ADC_IT_OVR, DISABLE);
	ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
	ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
	ADC_ITConfig(ADC2, ADC_IT_OVR, DISABLE);
	ADC_ITConfig(ADC2, ADC_IT_EOC, DISABLE);
	ADC_ITConfig(ADC3, ADC_IT_OVR, DISABLE);
	ADC_ITConfig(ADC3, ADC_IT_EOC, DISABLE);
  TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_TE, DISABLE);
}

void ResetSampling(void)
{
	DataTimeSec = 0;
	DataIndex = 0;
	SampleIndex = 0;
	TimerWithTriggerState = TIMER_WITH_TRIGGER_STATE_COLLECT_MINIMUM_SAMPLES;
	DMA_WithTriggerState = DMA_WITH_TRIGGER_STATE_COLLECT_MINIMUM_SAMPLES;
	LastSampleIndex = 0;
}

void SendSampleData(void)
{
	if (StartSendingData == TRUE)
	{
		if (SampleMode == SAMPLE_MODE_INTERLEAVE)
		{
			SampleIndex++;
			DataTimeSec += TimeInterval;
			DataIndex = SampleIndex - 1;
			Volt1 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex], 1);
			Volt2 = 0;
			Volt3 = 0;

			USBWriteBufferPlot(CAPTURE_NUMBER_FOR_SCOPE);
			if (SampleIndex == LastSampleIndex)
			{
				StartSendingData = FALSE;
				ResetSampling();
			}
		}
		else if (SampleMode == SAMPLE_MODE_DMA)
		{
			SampleIndex++;
			DataTimeSec += TimeInterval;

			Volt1 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 1);
			if (EnableChannel1 == FALSE)
			{
				Volt1 = 0;
			}
			Volt2 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 2);
			if (EnableChannel2 == FALSE)
			{
				Volt2 = 0;
			}
			Volt3 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 3);
			if (EnableChannel3 == FALSE)
			{
				Volt3 = 0;
			}

			USBWriteBufferPlot(CAPTURE_NUMBER_FOR_SCOPE);

			if (SampleIndex >= NumberOfSamples)
			{
				StartSendingData = FALSE;
				ResetSampling();
			}
		}
		else if (SampleMode == SAMPLE_MODE_DMA_WITH_TRIGGER)
		{
			// Update sampling index
			SampleIndex++;
			if (SampleIndex > (NumberOfSamples + EXTRA_SAMPLE_SIZE))
			{
				SampleIndex = 1;
			}
			DataIndex = (SampleIndex-1)*3;
			// Update data time
			DataTimeSec += TimeInterval;
			// Get voltages
			Volt1 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 1);
			if (EnableChannel1 == FALSE)
			{
				Volt1 = 0;
			}
			Volt2 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 2);
			if (EnableChannel2 == FALSE)
			{
				Volt2 = 0;
			}
			Volt3 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 3);
			if (EnableChannel3 == FALSE)
			{
				Volt3 = 0;
			}
			// Write data to USB buffer
			USBWriteBufferPlot(CAPTURE_NUMBER_FOR_SCOPE);
			// Check if end of data
			if (SampleIndex == LastSampleIndex)
			{
				StartSendingData = FALSE;
				ResetSampling();
			}
		}
		else if (SampleMode == SAMPLE_MODE_TIMER_WITH_TRIGGER)
		{
			// Update sampling index
			SampleIndex++;
			if (SampleIndex > NumberOfSamples)
			{
				SampleIndex = 1;
			}
			DataIndex = (SampleIndex-1)*3;
			// Update data time
			DataTimeSec += TimeInterval;
			// Get voltages
			Volt1 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 1);
			if (EnableChannel1 == FALSE)
			{
				Volt1 = 0;
			}
			Volt2 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 2);
			if (EnableChannel2 == FALSE)
			{
				Volt2 = 0;
			}
			Volt3 = ConvertADC_ValueToVoltage(ADCConvertedValues[DataIndex++], 3);
			if (EnableChannel3 == FALSE)
			{
				Volt3 = 0;
			}
            // Write data to USB buffer
			USBWriteBufferPlot(CAPTURE_NUMBER_FOR_SCOPE);
			// Check if end of data
			if (SampleIndex == LastSampleIndex)
			{
				StartSendingData = FALSE;
				ResetSampling();
			}
		}
	}
}

float ConvertADC_ValueToVoltage(uint16_t adc_value, uint8_t channel)
{
	float voltage;
	float gain = 0;
	float offset = 0;

	if ((channel == 1) && (Ch1Probe10x == FALSE))
	{
		gain = (float)GainCh1_1x;
		offset = (float)OffsetCh1_1x;
	}
	else if ((channel == 1) && (Ch1Probe10x == TRUE))
	{
		gain = (float)GainCh1_10x;
		offset = (float)OffsetCh1_10x;
	}
	else if ((channel == 2) && (Ch2Probe10x == FALSE))
	{
		gain = (float)GainCh2_1x;
		offset = (float)OffsetCh2_1x;
	}
	else if ((channel == 2) && (Ch2Probe10x == TRUE))
	{
		gain = (float)GainCh2_10x;
		offset = (float)OffsetCh2_10x;
	}
	else if ((channel == 3) && (Ch3Probe10x == FALSE))
	{
		gain = (float)GainCh3_1x;
		offset = (float)OffsetCh3_1x;
	}
	else if ((channel == 3) && (Ch3Probe10x == TRUE))
	{
		gain = (float)GainCh3_10x;
		offset = (float)OffsetCh3_10x;
	}

	if (UseRawADC == TRUE)
	{
		voltage = (float)adc_value;
	}
	else
	{
		voltage = ((float)adc_value - offset) * gain;
	}
	return voltage;
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
	SimpleDebug('f'); SimpleDebug('a'); SimpleDebug('i'); SimpleDebug('l'); SimpleDebug('\n'); SimpleDebug('\r');
	printf("Wrong parameters value: file %s on line %u\r\n", file, (unsigned int)line);
	__disable_irq();
	/* Infinite loop */
	while (1)
	{
	}
}
#endif

//////////////////////   Interrupt handlers     ////////////////////////////////////////////////////////////////

// Timer 2 interrupt
void TIM2_IRQHandler(void)
{
	// Update interrupt
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		// Clear flag
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		// Increment 1ms counter
		Counter_1ms++;

		// 10 ms task
		if (Counter_1ms % 10 == 0)
		{
			SendSampleData();
		}

		// 500 ms task
		if (Counter_1ms % 500 == 0)
		{
			// Update capture setting
			UpdateSettings();

			// Write non plot data
			USBWriteBufferNonPlot();

			// Start sampling
			if (StartSample == TRUE)
			{
				StartSample = FALSE;
				USBWriteBufferMessage("StartSample", 0);

				if (SampleMode == SAMPLE_MODE_INTERLEAVE)
				{
					ConfigADC();
					// Start ADC conversion (ADC1 is the master)
					ADC_SoftwareStartConv(ADC1);
				}
				else if (SampleMode == SAMPLE_MODE_DMA)
				{
					ConfigADC();
					// Start ADC conversion (ADC1 is the master)
					ADC_SoftwareStartConv(ADC1);
				}
				else if (SampleMode == SAMPLE_MODE_TIMER)
				{
					ConfigADC();
					ConfigTimer3();
				}
				else if (SampleMode == SAMPLE_MODE_DMA_WITH_TRIGGER)
				{
					ConfigADC();
					ConfigTimer3();
					// Start ADC conversion (ADC1 is the master)
					ADC_SoftwareStartConv(ADC1);
				}
				else if (SampleMode == SAMPLE_MODE_TIMER_WITH_TRIGGER)
				{
					ConfigADC();
					ConfigTimer3();
				}
			}
			// Frequency generation
			if (StartFrequencyGeneration)
			{
				StartFrequencyGeneration = FALSE;
				ConfigTimer4();
			}
		}
		// 1s task
		if (Counter_1ms % 1000 == 0)
		{
			GPIO_ToggleBits(GPIOD, GPIO_Pin_14);

			// Write DAC
			DAC_SetChannel1Data(DAC_Align_12b_R, (uint16_t)DAC1_value);
			DAC_SetChannel2Data(DAC_Align_12b_R, (uint16_t)DAC2_value);
		}
   }
}

// Timer 3 interrupt
void TIM3_IRQHandler(void)
{
	// Update interrupt
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		if (SampleMode == SAMPLE_MODE_DMA_WITH_TRIGGER)
		{
			int32_t remaining_data_count, adc1_value;
			int32_t found = FALSE;

			// Finish sampling
			FinishSampling();

			// Skip the area that is possibly overwritten
			remaining_data_count = (int32_t)DMA_GetCurrDataCounter(DMA2_Stream0);
			SampleIndex = NumberOfSamples + EXTRA_SAMPLE_SIZE - (remaining_data_count/3); // Points to the last sample taken
			SampleIndex = SampleIndex + (EXTRA_SAMPLE_SIZE/2);
			if (SampleIndex > (NumberOfSamples + EXTRA_SAMPLE_SIZE))
			{
				SampleIndex -= (NumberOfSamples + EXTRA_SAMPLE_SIZE);
			}
			// Get old value
			ADC1DataOld = (int32_t)ADCConvertedValues[(SampleIndex-1)*3];
			// Keep searching for the threshold
			while (found == FALSE)
			{
				SampleIndex++;
				if (SampleIndex > (NumberOfSamples + EXTRA_SAMPLE_SIZE))
				{
					SampleIndex = 1;
				}
				adc1_value = ADCConvertedValues[(SampleIndex-1)*3];
				if (((RisingEdgeTriggerEnable == TRUE) && (ADC1DataOld < ADC1_Threshold) && (adc1_value >= (uint16_t)ADC1_Threshold))
					||
					((FallingEdgeTriggerEnable == TRUE) && (adc1_value < (uint16_t)ADC1_Threshold) && (ADC1DataOld >= ADC1_Threshold)))
				{
					found = TRUE;
					LastSampleIndex = SampleIndex + (NumberOfSamples - NumberOfSamplesBeforeTrigger) - 1;
					if (LastSampleIndex > (NumberOfSamples + EXTRA_SAMPLE_SIZE))
					{
						LastSampleIndex -= (NumberOfSamples + EXTRA_SAMPLE_SIZE);
					}
					SampleIndex = SampleIndex - NumberOfSamplesBeforeTrigger - 1;
					if (SampleIndex < 1)
					{
						SampleIndex += (NumberOfSamples + EXTRA_SAMPLE_SIZE);
					}
				}
				// Update old value
				ADC1DataOld = adc1_value;
			}
			StartSendingData = TRUE;
			DMA_WithTriggerState = DMA_WITH_TRIGGER_STATE_FINISHED;
		}
		else if ((SampleMode == SAMPLE_MODE_TIMER) || (SampleMode == SAMPLE_MODE_TIMER_WITH_TRIGGER))
		{
			// Start conversion
			ADC_SoftwareStartConv(ADC1);
		}
		// Clear flag
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}

// Timer 4 interrupt
void TIM4_IRQHandler(void)
{
	// Update interrupt
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		GPIO_ToggleBits(GPIOD, GPIO_Pin_11);
		// Clear flag
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}

// DMA2 Stream 0 interrupt
void DMA2_Stream0_IRQHandler()
{
	USBWriteBufferMessage("DMA2 interrupt", 0);

	SimpleDebug('D'); SimpleDebug('M'); SimpleDebug('A'); SimpleDebug('\n'); SimpleDebug('\r');

	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TEIF0) == SET)
	{
		SimpleDebug('x');
		USBWriteBufferMessage("DMA_IT_TEIF0", 0);
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TEIF0);
	}
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0) == SET)
	{
		if (SampleMode == SAMPLE_MODE_DMA_WITH_TRIGGER)
		{
			if (DMA_WithTriggerState == DMA_WITH_TRIGGER_STATE_COLLECT_MINIMUM_SAMPLES)
			{
				uint16_t high, low;

				// Go to next state
				DMA_WithTriggerState = DMA_WITH_TRIGGER_STATE_WF_PRE_THRESHOLD;
				//Disable TC interrupt
				DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, DISABLE);
				// Set up analog watchdog for pre-threshold
				if (RisingEdgeTriggerEnable)
				{
					high = 0xFFF;
					low = ADC1_Threshold;
				}
				else
				{
					high = ADC1_Threshold;
					low = 0;
				}
				ADC_AnalogWatchdogSingleChannelConfig(ADC1, ADC_Channel_10);
				ADC_AnalogWatchdogThresholdsConfig(ADC1, high, low);
				ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_SingleRegEnable);
				ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);
			}
		}
		else if (SampleMode == SAMPLE_MODE_DMA)
		{
			USBWriteBufferMessage("DMA_IT_TCIF0 - DMA", 0);

			StartSendingData = TRUE;
			FinishSampling();
		}
		else if (SampleMode == SAMPLE_MODE_INTERLEAVE)
		{
			USBWriteBufferMessage("DMA_IT_TCIF0 - Interleave", 0);

			SampleIndex = EXTRA_SAMPLE_SIZE;
			LastSampleIndex = NumberOfSamples + EXTRA_SAMPLE_SIZE;
			StartSendingData = TRUE;
			FinishSampling();
		}
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0);
	}
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_HTIF0) == SET)
	{
		SimpleDebug('v');
		USBWriteBufferMessage("DMA_IT_HTIF0", 0);
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_HTIF0);
	}
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_DMEIF0) == SET)
	{
		SimpleDebug('y');
		USBWriteBufferMessage("DMA_IT_DMEIF0", 0);
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_DMEIF0);
	}
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_FEIF0) == SET)
	{
		SimpleDebug('z');
		USBWriteBufferMessage("DMA_IT_FEIF0", 0);
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_FEIF0);
	}
}

void ADC_IRQHandler()
{
	// For ADC 1
	if (ADC_GetITStatus(ADC1, ADC_IT_EOC) == SET)
	{
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	}

	if (ADC_GetITStatus(ADC1, ADC_IT_AWD) == SET)
	{
		if (SampleMode == SAMPLE_MODE_DMA_WITH_TRIGGER)
		{
			if (DMA_WithTriggerState == DMA_WITH_TRIGGER_STATE_WF_PRE_THRESHOLD)
			{
				uint16_t high, low;

				// Move to next state
				DMA_WithTriggerState = DMA_WITH_TRIGGER_STATE_WF_POST_THRESHOLD;
				// Setup watchdog for post threshold
				ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
				if (RisingEdgeTriggerEnable)
				{
					high = ADC1_Threshold;
					low = 0;
				}
				else
				{
					high = 0xFFF;
					low = ADC1_Threshold;
				}
				ADC_AnalogWatchdogSingleChannelConfig(ADC1, ADC_Channel_10);
				ADC_AnalogWatchdogThresholdsConfig(ADC1, high, low);
				ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_SingleRegEnable);
				ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);
			}
			else if (DMA_WithTriggerState == DMA_WITH_TRIGGER_STATE_WF_POST_THRESHOLD)
			{
				ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_None);
				ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
				// Start timer to finish up the remaining samples
				TIM_Cmd(TIM3, ENABLE);
			}
		}
		ADC_ClearFlag(ADC1, ADC_FLAG_AWD);
	}

	if (ADC_GetITStatus(ADC1, ADC_IT_JEOC) == SET)
	{
		USBWriteBufferMessage("ADC1, ADC_IT_JEOC", 0);
		ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
	}

	if (ADC_GetITStatus(ADC1, ADC_IT_OVR) == SET)
	{
		USBWriteBufferMessage("ADC1, ADC_IT_OVR", 0);
		ADC_ClearFlag(ADC1, ADC_FLAG_OVR);
	}

	// For ADC 2
	if (ADC_GetITStatus(ADC2, ADC_IT_EOC) == SET)
	{
		ADC_ClearFlag(ADC2, ADC_FLAG_EOC);
	}

	if (ADC_GetITStatus(ADC2, ADC_IT_AWD) == SET)
	{
		USBWriteBufferMessage("ADC2, ADC_IT_AWD", 0);
		ADC_ClearFlag(ADC2, ADC_FLAG_AWD);
	}

	if (ADC_GetITStatus(ADC2, ADC_IT_JEOC) == SET)
	{
		USBWriteBufferMessage("ADC2, ADC_IT_JEOC", 0);
		ADC_ClearFlag(ADC2, ADC_FLAG_JEOC);
	}

	if (ADC_GetITStatus(ADC2, ADC_IT_OVR) == SET)
	{
		USBWriteBufferMessage("ADC2, ADC_IT_OVR", 0);
		ADC_ClearFlag(ADC2, ADC_FLAG_OVR);
	}

	// For ADC 3
	if (ADC_GetITStatus(ADC3, ADC_IT_EOC) == SET)
	{
		if (SampleMode == SAMPLE_MODE_TIMER)
		{
			// Read data
			Volt1 = ConvertADC_ValueToVoltage(*((uint16_t*)ADC1_DR), 1);
			if (EnableChannel1 == FALSE)
			{
				Volt1 = 0;
			}
			Volt2 = ConvertADC_ValueToVoltage(*((uint16_t*)ADC2_DR), 2);
			if (EnableChannel2 == FALSE)
			{
				Volt2 = 0;
			}
			Volt3 = ConvertADC_ValueToVoltage(*((uint16_t*)ADC3_DR), 3);

			//Volt3 = (float)(*((uint32_t*)ADC3_DR) & 0x0000FFFF);
			if (EnableChannel3 == FALSE)
			{
				Volt3 = 0;
			}

			// Send data
			SampleIndex++;
			DataTimeSec += TimeInterval;
			USBWriteBufferPlot(CAPTURE_NUMBER_FOR_SCOPE);
			if (SampleIndex >= NumberOfSamples)
			{
				FinishSampling();
				ResetSampling();
			}
		}
		else if (SampleMode == SAMPLE_MODE_TIMER_WITH_TRIGGER)
		{
			switch(TimerWithTriggerState)
			{
				case TIMER_WITH_TRIGGER_STATE_COLLECT_MINIMUM_SAMPLES:
				{
					// Update sample index
					SampleIndex++;

					// Get ADC values
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC1_DR) & 0x0000FFFF);
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC2_DR) & 0x0000FFFF);
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC3_DR) & 0x0000FFFF);
					// Check if minimum number of samples are taken before the trigger
					if (SampleIndex == NumberOfSamplesBeforeTrigger)
					{
						ADC1DataOld = (int32_t)ADCConvertedValues[DataIndex-3];
						TimerWithTriggerState = TIMER_WITH_TRIGGER_STATE_PRE_TRIGGER;
					}
				}
				break;
				case TIMER_WITH_TRIGGER_STATE_PRE_TRIGGER:
				{
					uint16_t adc1;

					// Update sample index
					SampleIndex++;

					if (SampleIndex > NumberOfSamples)
					{
						SampleIndex = 1;
						DataIndex = 0;
					}
					// Get ADC values
					ADCConvertedValues[DataIndex] = (uint16_t)(*((uint32_t*)ADC1_DR) & 0x0000FFFF);
					adc1 = ADCConvertedValues[DataIndex++];
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC2_DR) & 0x0000FFFF);
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC3_DR) & 0x0000FFFF);
					// Check trigger condition
					if (((RisingEdgeTriggerEnable == TRUE) && (ADC1DataOld < ADC1_Threshold) && (adc1 >= (uint16_t)ADC1_Threshold))
						||
						((FallingEdgeTriggerEnable == TRUE) && (adc1 < (uint16_t)ADC1_Threshold) && (ADC1DataOld >= ADC1_Threshold)))
					{
						// Calculate last sample index
						LastSampleIndex = SampleIndex - NumberOfSamplesBeforeTrigger - 1;
						if (LastSampleIndex < 1)
						{
							LastSampleIndex += NumberOfSamples;
						}
						TimerWithTriggerState = TIMER_WITH_TRIGGER_STATE_POST_TRIGGER;
					}
					// Keep the last value
					ADC1DataOld = (int32_t)adc1;
				}
				break;
				case TIMER_WITH_TRIGGER_STATE_POST_TRIGGER:
				{
					// Update sample index
					SampleIndex++;

					if (SampleIndex > NumberOfSamples)
					{
						SampleIndex = 1;
						DataIndex = 0;
					}
					// Get ADC values
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC1_DR) & 0x0000FFFF);
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC2_DR) & 0x0000FFFF);
					ADCConvertedValues[DataIndex++] = (uint16_t)(*((uint32_t*)ADC3_DR) & 0x0000FFFF);
					// Check if all the data needed is taken
					if (SampleIndex == LastSampleIndex)
					{
						StartSendingData = TRUE;
						FinishSampling();
						TimerWithTriggerState = TIMER_WITH_TRIGGER_STATE_FINISHED;
					}
				}
				break;
				case TIMER_WITH_TRIGGER_STATE_FINISHED:
				{
				}
				break;
				default:
				{
				}
				break;
			}
		}
		else
		{
			USBWriteBufferMessage("ADC3, ADC_IT_EOC", 0);
		}
		ADC_ClearFlag(ADC3, ADC_FLAG_EOC);
	}

	if (ADC_GetITStatus(ADC3, ADC_IT_AWD) == SET)
	{
		USBWriteBufferMessage("ADC3, ADC_IT_AWD", 0);
		ADC_ClearFlag(ADC3, ADC_FLAG_AWD);
	}

	if (ADC_GetITStatus(ADC3, ADC_IT_JEOC) == SET)
	{
		USBWriteBufferMessage("ADC3, ADC_IT_JEOC", 0);
		ADC_ClearFlag(ADC3, ADC_FLAG_JEOC);
	}

	if (ADC_GetITStatus(ADC3, ADC_IT_OVR) == SET)
	{
		USBWriteBufferMessage("ADC3, ADC_IT_OVR", 0);
		ADC_ClearFlag(ADC3, ADC_FLAG_OVR);
	}
}

