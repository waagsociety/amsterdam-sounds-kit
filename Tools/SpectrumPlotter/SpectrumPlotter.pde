//to be used with  AdamSoundsSPEC_SLM
import processing.serial.*;

float logX(float v, float base) {
  return log(v)/log(base);
}


float db_full_scale(float rms) {
  return (20 * logX(rms * sqrt(2), 10));
}



Serial serial;
int DATA_SIZE = 1024;
int FFT_SIZE = DATA_SIZE * 2;

double Q_SCALAR = (1 << (32 - int(logX(FFT_SIZE, 2))));

void setup()
{
  String portName = Serial.list()[1];
  println("reading from: " + portName);
  serial = new Serial(this, portName, 9600);
  println("scalar: " + Q_SCALAR);
  size(512, 512);
}

int c=0;
void draw()
{
  
  int FFT_SIZE = 2048;
  
  
  byte[] inBuffer = new byte[FFT_SIZE*4];
  
  if(serial.available() >= (FFT_SIZE*4)) 
  {  
    background(0);
    stroke(255);
    int read = serial.readBytes(inBuffer);
    int bins = FFT_SIZE / 2;
    double total = 0;
    println("read: " + read);
    for (int i = 2; i < (bins / 2 + 1); i++)
    {
      int re_i = i * 2 * 4;
      int im_i = (i * 2 + 1) * 4;
      int re_q = 0;
      int im_q = 0;
      for(int j = 0; j < 4; j++) {
        byte re_b = inBuffer[re_i + j];
        byte im_b = inBuffer[im_i + j];
        re_q |= (int(re_b) << (j * 8));
        im_q |= (int(im_b) << (j * 8));
      }
      double re_f = re_q / Q_SCALAR;
      double im_f = im_q / Q_SCALAR;
      double mag_squared = (re_f * re_f + im_f * im_f);
      
      float RMS_bin = sqrt((float)(mag_squared * 2.0/(DATA_SIZE*DATA_SIZE)));
      
      float db = db_full_scale(RMS_bin*1.63);
      
      float range = 90;
      float y = -db * height/range;
      
      total += mag_squared;
      line(i, height, i, y);
    }
    
    total *= 2.0 / DATA_SIZE;

    // Scaling the individual amplitudes of the bins is proportional with scaling the RMS as such we scale at the end
    float RMS = 1.63 * sqrt((float)(total / DATA_SIZE));
    float dbA = 120 + db_full_scale(RMS);

    //
    println(c + ": dbA: " + dbA);
    c++;
  } 

}
