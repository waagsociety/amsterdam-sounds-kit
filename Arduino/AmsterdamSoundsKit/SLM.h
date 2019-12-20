/*

 This application reads audio data from an Knowles I2S microphone (SPH0645LM4H-B on AdaFruit breakout board).

 SPH0645LM4H-B Datasheet:
 https://cdn-shop.adafruit.com/product-files/3421/i2S+Datasheet.PDF

 DSP is done using the CMSIS-DSP library for ARM:
 https://arm-software.github.io/CMSIS_5/DSP/html/group__RealFFT.html

 Theory on calculating DB(a) using FFT can be found here (Sound Level by Meter Douglas R. Lanman):
 http://alumni.media.mit.edu/~dlanman/courses/decibel_meter.pdf

*/

#include <I2S.h>
#include "arm_math.h"
#include "arm_const_structs.h"
#include "SLMSettings.h"
#include "Stats.h"
#include "RunningAvg.h"
#include "EQ.h"

// Audio buffer indexes and counters
static uint32_t buffer_write_index = 0;
static uint32_t buffers_written = 0;
static int32_t buffer[BUFFER_SIZE];
static uint64_t samples_read = 0;
static uint32_t frames_done = 0;

/// Buffer for reading from I2S
static int32_t chunk[I2S_BUFFER_SIZE];

// Buffers for FFT are in q31 format. Fixed point values range from [-1,1] mapped to [-2^31,2^32].
// The DSP library uses fixed point (q31) as input. But the output of the FFT can be in different format (values bigger than 1).
// For the FFT the output depends on the FFT size (bigger FFT, bigger values, see CMSIS-DSP documention).
// N.B. FFT size is defined as 2 x number of bins.
// For example (documentation) the output size for q31 FFT with size 2048 (1024 real input samples) is q11.21.
// That means we have to divide output values by 2^21.
static q31_t hann[DATA_SIZE];

// Test tone generation, in order to test the dBA algorithm test tones can be generated instead of using audio input from the microphone
uint32_t test_tone_index = 0;
uint32_t test_tone_step = INT32_MAX / (FS / TEST_TONE_FREQ);

//
static double normal_hann_scale_factor = (1.63 * sqrt(2.0))/((double)DATA_SIZE);                                                                                                        //normalize and double because we use half of the bins, multiply to correct hann window
static uint8_t fracbits = (FRACBITS + FRACBITS + SCALING_TABLE_FRACBITS);



class SLM
{
  Stream& serial;
  bool paused = true;
  int bands;
  RunningAvg avg = RunningAvg();
public:
  Stats stats = Stats();
  Stats band_stats[16] = { Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats(), Stats() }; //1 per band 16 is more than enough


  // Interrupt must take as little time as possible. Copy samples from the I2S buffer to a circular buffer.
  // SPH0645LM4H-B delivers 18 bit (datasheet) in 2x32 bit (2 channels) frames.
  // By default with one microphone attached the second channel contains the data
  // N.B. test tone amplitude is to high for the fixed point version (saturates), devide by 4 to lower with 12dB
  static void onI2SReceive()
  {
    // Number of bytes
    size_t l = I2S.available();
    int r = I2S.read(chunk, l);
    samples_read += (l/8);
    // Copy data from I2S or fill with sine test tone
    uint8_t channel = 1; //0 or 1
    int32_t val;
    for (unsigned int i = channel; i < l / 4; i += 2)
    {
      val = (chunk[i] << 0) & 0xFFFFC000;  // mask with 14 zero's
#if (TEST == 1)
      q31_t r = test_tone_index;
      buffer[buffer_write_index] = arm_sin_q31(test_tone_index); // expects [0,1>
      test_tone_index += test_tone_step;
      if (test_tone_index > INT32_MAX) // INT32_MAX is Q31_MAX
      {
        test_tone_index -= INT32_MAX;
      }
#else
      buffer[buffer_write_index] = val;
#endif
      buffer_write_index++;
      if (buffer_write_index == BUFFER_SIZE)
      {
        buffer_write_index = 0;
      }
    }
  }


  SLM(Stream &s) : serial(s)
  {
  }

  void start()
  {
    // Start I2S at 48 kHz with 32-bits per block (containes 24-bit sample), allowed rates for ics43432 8khz, 16khz, 24khz, 48khz
    if (!I2S.begin(I2S_PHILIPS_MODE, FS, 32))
    {
      if (SLM_MODE == SLM_MODE_DEBUG)
      {
        serial.println(F("Failed to initialize I2S!"));
      }

      while (1)       // do nothing
      {
      }
    }
    // 
    I2S.read();
    //
    paused = false;
    // Number of bands is highest band index + 1
    bands = (31 - __builtin_clz((DATA_SIZE / 2) / START_BIN)) + 1;
  }

  // End audio stream from mic
  void stop()
  {
    I2S.end();
  }

  // Bypass processing. Cannot stop and restart I2S for some reason
  void pause()
  {
    paused = true;
  }

  // Reset statistics 
  void reset()
  {
    stats.Reset();
#if CALC_BANDS == 1
    for (int i = 0; i < bands; i++)
    {
      band_stats[i].Reset();
    }
#endif
  }

  // Resume processing, reset counters so we are not behind when we restart processing
  void resume()
  {
    paused = false;
    samples_read = 0;
    frames_done = 0;
  }

  // One time stuff
  void setup()
  {
    // Make a q31 hann window for scaling FFT real input buffer
    double hann_s = 1.0 / DATA_SIZE;
    double hann_i = 0;
    for (unsigned int i = 0; i < DATA_SIZE; i++)
    {
      double v = sin(hann_i * PI);
      hann[i] = v * v * 2147483647.0;       //to q31
      hann_i += hann_s;
    }
    // Set receive funcion for audio data
    I2S.onReceive(SLM::onI2SReceive);
  }

   
  // Stream audio to serial port. Make sure to enable and write nothing else then this audio.
  // Listen: cat /dev/cu.usbmodem14111 | mplayer -rawaudio rate=48000:channels=1:samplesize=4 -demuxer rawaudio -cache 1024 -
  // Or write to raw file: cat /dev/cu.usbmodem14111 > audio.raw
  void streamAudio()
  {
    uint32_t frames_ought_to_be_done = samples_read / DATA_SIZE;
    if ((SLM_MODE == SLM_MODE_STREAM_AUDIO) && (frames_done < frames_ought_to_be_done))
    {
      q31_t windowed[DATA_SIZE];
      // Copy latest window of samples possibly wrapped over circular buffer
      uint32_t start = (BUFFER_SIZE + buffer_write_index - DATA_SIZE) % BUFFER_SIZE;
      if (start <= (BUFFER_SIZE - DATA_SIZE))
      {
        // Without wrapping
        memcpy((uint8_t *)windowed, (uint8_t *)(buffer + start), DATA_SIZE * sizeof(int32_t));
      }
      else
      {
        // With wrapping in two parts
        memcpy((uint8_t *)windowed, (uint8_t *)(buffer + start), (BUFFER_SIZE - start) * sizeof(int32_t));
        memcpy((uint8_t *)(windowed + (BUFFER_SIZE - start)), (uint8_t *)buffer, (DATA_SIZE - (BUFFER_SIZE - start)) * sizeof(int32_t));
      }
      serial.write((uint8_t *)windowed, DATA_SIZE * sizeof(int32_t));
      frames_done++;
    }
  }

  // Main function call as often as possible. Sticks to the frame rate configured
  void update()
  {
    if (paused)
    {
      return;
    }

    //
    if (SLM_MODE == SLM_MODE_STREAM_AUDIO)
    {
      streamAudio();
      return;
    }

    //
    if (SLM_MODE == SLM_MODE_OUTPUT)
    {
      // Reset stats on user input
      if (serial.available() > 0)
      {
        byte b = serial.read();
        if (b == 0x00)
        {
          stats.Reset();
        }
      }
    }

    // All modes except for SLM_MODE_STREAM_AUDIO
    uint32_t frames_ought_to_be_done = samples_read / (FS / FPS);
    if (frames_done < frames_ought_to_be_done)
    {
      // Input and output buffer
      q31_t windowed[DATA_SIZE];
      q31_t out_q[DATA_SIZE * 2];

      // Measure, all calcs take about 27ms with Q nums, 50ms with floats
      int m = millis();

      // Skip the first buffers, seem to contain noise
      if (frames_ought_to_be_done < 10)
      {
        frames_done++;
        return;
      }

      // Multipy latest window of samples possibly wrapped over buffer
      uint32_t start = (BUFFER_SIZE + buffer_write_index - DATA_SIZE) % BUFFER_SIZE;

      // Apply Hann window to newest samples in buffer, correct later with scaling. 
      // Take samples from circular buffer with or without wrap depending on current write index. 
      // Following operation must be done within (BUFFER_HEAD / FS) seconds, it should take about max 2ms
      if (start <= (BUFFER_SIZE - DATA_SIZE))
      {
        // Without wrapping
        arm_mult_q31(buffer + start, hann, windowed, DATA_SIZE);
      }
      else
      {
        // With wrapping in two parts
        arm_mult_q31(buffer + start, hann, windowed, BUFFER_SIZE - start);
        arm_mult_q31(buffer, hann + (BUFFER_SIZE - start), windowed + (BUFFER_SIZE - start), DATA_SIZE - (BUFFER_SIZE - start));
      }

      // Init FFT
      arm_rfft_instance_q31 real;
      if (arm_rfft_init_q31(&real, DATA_SIZE, 0, 1) != ARM_MATH_SUCCESS)         //0 = forward, 1 = bitreverse
      {
        serial.print(F("err"));
      }

      // Perform FFT respect upscaling in other Q format
      arm_rfft_q31(&real, windowed, out_q);

      // Stream FFT data out in binary format
      if (SLM_MODE == SLM_MODE_STREAM_FFT)
      {
        serial.write((uint8_t *)out_q, DATA_SIZE * 2 * sizeof(int32_t));
        frames_done++;
        return;
      }

      // When we remove lower bins, things get much better... this seems to have to do with DC offset.
      // Seems to be a lot of noise below 50hz while the mic cannot even measure that, as such we ignore te first bins.
      // At 1024 real input size (FFT size 2048) first bins 0 & 1 are respectively 0Hz and 43Hz.
      // Floating point version: normalizing and scaling the q (fixed point) number is done after the loop to save floating point calulcations.
#if USE_FLOATS == 1
      double total = 0;       // RMS total
      for (unsigned int i = START_BIN; i < (DATA_SIZE / 2); i++)
      {
        q31_t re = out_q[i * 2];
        q31_t im = out_q[i * 2 + 1];
        double re_f = ((double)re);
        double im_f = ((double)im);
        // Because magnitude is squared here we need to multiply with A-weighting amplitude scaling squared (which is power scaling)
        float scale = CORRECTED_DBS_FLOAT[i];
        double mag_squared = (re_f * re_f + im_f * im_f) * scale;
        total += mag_squared;
      }

      double RMSf = 1.63 * q2double(sqrt(2.0 * total / (DATA_SIZE * DATA_SIZE)), FRACBITS);
      double dbA = 120 + db_full_scale(RMSf);
#else
      // Fixed point version, take in account that numbers keep fitting in the fixed point numbers. Adapted the qmath lib to give 64 bit room!
      // When normalizing in the loop a lot of resolution (data) is lost, so we want to postpone scaling back as long as possible.
      // The fp version was found to be 5 times quicker, however with really high levels (0db) we might saturate the fp number.
      // For example the RMS total (before normalizing) with an unsigned q14.50, the maximum value is 2^14. As such the maximum level is only -7.8dB with 1024 real input size.
      // Ruby code : 20 * Math::log10(1.63 * Math::sqrt(2.0 * (2**14) / (1024 ** 2)) * Math::sqrt(2))
      // The testtone does not reach 120 for this reason!
      // To partly solve this problem we calculate a total per band and add all the bands as floating point after the heavy calculation.
      // When using a test tone of 0db fixed point numbers can still saturate because all energy is in one band.
      // Comments for code assumes using 2048 FFT size
      uint64_t band_totals[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };       //totals per band, amount of bands varies with FFT_SIZE and START_BIN, however 16 is more than enough
      int64_t re;
      int64_t im;
      uint64_t q_squared;
      uint64_t q_squared_scaled;
      uint8_t band_index;
      unsigned int o;
      for (unsigned int i = START_BIN; i < (DATA_SIZE / 2); i++)
      {
        band_index = (31 - __builtin_clz(i / START_BIN));                               //from start bin, band increases with 1 on every doubling
        o = i * 2;
        re = out_q[o];                                                                  //get the q number, fractional bit depends on FFT SIZE, q11.21 for FFT size 2048
        im = out_q[o + 1];                                                              //get the q number, fractional bit depends on FFT SIZE, q11.21 for FFT size 2048
        q_squared = re * re + im * im;                                                  //output q22.42, from here we know the value is positive we can save one bit for more resolution
        // Because magnitude is squared here we need to multiply with A-weighting amplitude scaling squared (which is power scaling)
        q_squared_scaled = q_squared * (uint64_t)CORRECTED_DBS_FIXED_POINT[i];          //output q14.50
        band_totals[band_index] += q_squared_scaled;
      }

      // After heavy calculation is done we go back to floating point numbers
      double bands_total = 0;
      double band_total;
      double RMS_band;
      double dbA_band;
      for (unsigned int i = 0; i < bands; i++)
      {
        band_total = q2double(band_totals[i], fracbits);
        bands_total += band_total;
#if CALC_BANDS == 1
        RMS_band = normal_hann_scale_factor * sqrt(band_total);
        dbA_band = 120 + db_full_scale(RMS_band);
        band_stats[i].UpdateMean(dbA_band);
#endif
      }

      double RMS = normal_hann_scale_factor * sqrt(bands_total);       //normalize by dividing by DATA_SIZE squared, double for mirrored freqs
      double dbA = 120 + db_full_scale(RMS);
#endif
      double dbLEQ = avg.Update(dbA);
      stats.Update(dbLEQ);

      frames_done++;

      if (SLM_MODE == SLM_MODE_OUTPUT)
      {
        serial.write((uint8_t *)&dbLEQ, sizeof(double));
        double dbMin = stats.Min();
        double dbMax = stats.Max();
        double dbMean = stats.Mean();
        serial.write((uint8_t *)&dbMin, sizeof(double));
        serial.write((uint8_t *)&dbMax, sizeof(double));
        serial.write((uint8_t *)&dbMean, sizeof(double));
      }

      if (SLM_MODE == SLM_MODE_DEBUG)
      {
        serial.print(dbA);
        serial.print(F(","));
        serial.print(dbLEQ);
        serial.print(F(","));
        serial.print(stats.Min());
        serial.print(F(","));
        serial.print(stats.Max());
        serial.print(F(","));
        serial.print(stats.Mean());
        serial.print(F(","));
        serial.println(stats.StdDev(true));
#if CALC_BANDS == 1
        for (unsigned int i = 0; i < bands; i++)
        {
          serial.print(i);
          serial.print(F(","));
          serial.println(band_stats[i].Mean());
        }
#endif
      }

      if (SLM_MODE == SLM_MODE_PROFILE)
      {
        serial.printf("%ms %i\r\n", millis() - m);
        serial.printf("missed frames: %i\n", frames_ought_to_be_done - frames_done);
      }
    }
  }
};
