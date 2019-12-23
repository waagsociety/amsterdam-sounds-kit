/**
 
 Settings for computing the dBA values.

*/

// Available modes
#define SLM_MODE_NORMAL 1 // When using as TTN device, no output 
#define SLM_MODE_STREAM_AUDIO 2 // Stream only audio data in binary format to serial port
#define SLM_MODE_STREAM_FFT 3 // Strean FFT ouput as binary data
#define SLM_MODE_OUTPUT 4  // Stream output numbers to serial port in order to display in Processing application
#define SLM_MODE_DEBUG 5 // Print output numbers to serial, human readable
#define SLM_MODE_PROFILE 6 // Print processing time of analysis function in ms

// Current mode
#define SLM_MODE SLM_MODE_NORMAL

// DATA_SIZE is the amount of real values for FFT input. This also determines the number of FFT bins and BUFFER_SIZE. The audio buffer had some extra room to avoid audio to be overwritten while processed.
#define DATA_SIZE 1024 //
#define BUFFER_HEAD 128 //
#define BUFFER_SIZE (DATA_SIZE + BUFFER_HEAD)

// Floats are more precise but slower, however being able to process more windows makes qnums more precise again.
#define USE_FLOATS 0

// Calculate dbA and stats per band (decrease frame rate or this will increase the amount of missed frames)
#define CALC_BANDS 0

// FFT size is defined as 2 times the number of bins.
#define FFT_SIZE DATA_SIZE * 2

// Use a generated full scale test sine instead of audio input. The dBA result should be 0 for 1000Hz. 
// When using qnums, the amount of energy all being in one bin will saturate the qnum, resulting in a lower dBA ouput.
#define TEST false
#define TEST_TONE_FREQ 1000


// Audio samplerate equals I2S clockrate/framesize. 48KHz means clockrate of 3.072 Mhz.
// Frequency response depends on sampling rate (datasheet).
#define FS 48000

// Frames per seconds, amount of analysises to be done within one sec, set lower if to many buffers skipped (debug to see)
#define FPS 32

// This microphone in particular has a large DC offset and generates some low frequency noise. Ignoring the low frequency bins improves the dbA result.
// The first bin is DC offset which can be thrown away totally. Highpass cutoff specified in Hz will remove the first x bins.
#define START_BIN 2

// Set the lenght of the running averge for calculating LEQ in fraction of a second (8 means 1/8). Preferably a number by which FPS is devisable.    
#define RUNNING_AVG_LENGTH 8
#define RUNNING_AVG_WINDOW_SIZE (FPS/RUNNING_AVG_LENGTH)

// The output scalar for FFT with q31 input
// As taken from the documentation (https://arm-software.github.io/CMSIS_5/DSP/html/group__RealFFT.html):
// 32   :  5.27
// 64   :  6.26
// ...
// 8192 : 13.19
// Calcaulating the leading zero's + 1 saves us having to make a table
#define FRACBITS (__builtin_clz(FFT_SIZE) + 1)

// The scaling table is defined in EQ.h 
#define SCALING_TABLE_FRACBITS 8
