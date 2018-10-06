#define CAPTURE_NUMBER_FOR_SCOPE                        0
#define MIN_NUMBER_OF_SAMPLES                           5
#define MIN_NUMBER_OF_SAMPLES_BEFORE_TRIGGER            2
#define EXTRA_SAMPLE_SIZE                               1000
#define PGA_GAIN_SELECT                                 0x40
#define ADC_VALUE_0V                                    2095
#define ADC_BUFFER_SIZE 30000 + (EXTRA_SAMPLE_SIZE*3) // Up to 10,000 samples, each sample has 3 data (3 channels).  Extra sample is used to capture enough data needed
#define ADC_CLK                                         41000000
#define PCLK2                                           84000000
#define ADC_CLK_480                                     (float)((float)ADC_CLK/((float)12+(float)480))
#define ADC_CLK_144                                     (float)((float)ADC_CLK/((float)12+(float)144))
#define ADC_CLK_3                                       (float)((float)ADC_CLK/((float)12+(float)3))
#define ADC_CLK_INTERLEAVE                              (float)((float)ADC_CLK*(float)3/((float)12+(float)3))
#define ADC1_DR                                         ((uint32_t)0x4001204C)
#define ADC2_DR                                         ((uint32_t)0x4001214C)
#define ADC3_DR                                         ((uint32_t)0x4001224C)
#define ADC_CDR                                         ((uint32_t)0x40012308) // Address of the regular data register for dual or triple modes

void SimpleDebug(uint8_t data);

enum
{
	SAMPLE_MODE_INTERLEAVE = 0,
	SAMPLE_MODE_DMA,
	SAMPLE_MODE_TIMER,
	SAMPLE_MODE_DMA_WITH_TRIGGER,
	SAMPLE_MODE_TIMER_WITH_TRIGGER
} SampleMode_t;

enum
{
	TIMER_WITH_TRIGGER_STATE_COLLECT_MINIMUM_SAMPLES = 0,
	TIMER_WITH_TRIGGER_STATE_PRE_TRIGGER,
	TIMER_WITH_TRIGGER_STATE_POST_TRIGGER,
	TIMER_WITH_TRIGGER_STATE_FINISHED,
} TimerWithTriggerState_t;

enum
{
	DMA_WITH_TRIGGER_STATE_COLLECT_MINIMUM_SAMPLES = 0,
	DMA_WITH_TRIGGER_STATE_WF_PRE_THRESHOLD,
	DMA_WITH_TRIGGER_STATE_WF_POST_THRESHOLD,
	DMA_WITH_TRIGGER_STATE_POST_TRIGGER,
	DMA_WITH_TRIGGER_STATE_FINISHED,
} DMA_WithTriggerState_t;
