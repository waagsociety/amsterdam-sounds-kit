//to be used with  AdamSoundsSPEC_SLM
import processing.serial.*;

Serial serial;

void setup()
{
  String portName = Serial.list()[1];
  println("reading from: " + portName);
  serial = new Serial(this, portName, 9600);
  size(512, 512);
}

double readDoubleFromData(byte[] data, int offset) {
  long l = 0;
  for (int i = offset+7 ; i >= offset ; i--) {
    l <<= 8;
    l |= (data[i] & 0x00ff);
  }
  double d = Double.longBitsToDouble(l);
  return d;
}

void draw()
{
 
  byte[] data = new byte[32];
  
  //read float values from arduino
  if(serial.available() >= (32)) 
  {  
    background(0);
    stroke(255);
    float textSize = 120;
    int read = serial.readBytes(data);
    
    double dbLEQ = readDoubleFromData(data, 0);
    double dbMin = readDoubleFromData(data, 8);
    double dbMax = readDoubleFromData(data, 16);
    double dbMean = readDoubleFromData(data, 24);
    
    String textLEQ = nf((float)dbLEQ, 2, 1);
    String textMin = nf((float)dbMin, 2, 1);
    String textMax = nf((float)dbMax, 2, 1);
    String textMean = nf((float)dbMean, 2, 1);
    textSize(textSize);
    text(textLEQ, (width - textWidth(textLEQ)) * 0.5, height * 0.55);
    textSize(textSize / 3);
    text(textMin, 0, height * 0.99);
    text(textMean, (width - textWidth(textMean)) * 0.5, height * 0.99);
    text(textMax, width - textWidth(textMax), height * 0.99);

  }
  
  
}

void keyPressed() {
  if (key == ' ') {
    serial.write(0x00);
  }
}
