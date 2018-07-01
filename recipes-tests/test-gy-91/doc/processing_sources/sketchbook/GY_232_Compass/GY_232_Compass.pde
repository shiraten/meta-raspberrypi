import processing.serial.*;
import java.awt.event.KeyEvent;
import java.io.IOException;
Serial myPort;
PImage imgCompass;
PImage imgCompassArrow;
PImage background;
String data="";
String x = "", y = "", z = "";
Float xf, yf, zf;
float heading, headingDegrees;
void setup() {
  size (1920, 1080, P3D);
  smooth();
  imgCompass = loadImage("Compass.png");
  imgCompassArrow = loadImage("CompassArrow.png");
  background = loadImage("Background.png");
  
  myPort = new Serial(this, "/dev/ttyUSB0", 115200); // starts the serial communication
  myPort.bufferUntil('\n');
}
void draw() {
  
  image(background,0, 0); // Loads the Background image
  
  heading = atan2(yf,xf);
  if(heading <0) heading += 2*PI;
  // Correcting due to the addition of the declination angle
  if(heading > 2*PI)heading -= 2*PI;
  headingDegrees = heading * 180/PI;
  
  pushMatrix();
  translate(width/2, height/2, 0); // Translates the coordinate system into the center of the screen, so that the rotation happen right in the center
  rotateZ(radians(-headingDegrees)); // Rotates the Compass around Z - Axis 
  image(imgCompass, -960, -540); // Loads the Compass image and as the coordinate system is relocated we need need to set the image at -960x, -540y (half the screen size)
  popMatrix(); // Brings coordinate system is back to the original position 0,0,0
  
  image(imgCompassArrow,0, 0); // Loads the CompassArrow image which is not affected by the rotateZ() function because of the popMatrix() function
  textSize(30);
  text("Heading: " + headingDegrees,40,40); // Prints the value of the heading on the screen
  delay(40);
  
}
// starts reading data from the Serial Port
 void serialEvent (Serial myPort) { 
   try{
      String data = myPort.readStringUntil('\n');
      if(data != null){
        data = data.substring(0, data.length()-1);
        String[] dataList = split(data, ',');
        
        x = dataList[0];
        y = dataList[1];
        z = dataList[2];
        
        xf = Float.parseFloat(x);
        yf = Float.parseFloat(y);
        zf = Float.parseFloat(z);
      }
    }
    catch(RuntimeException e) {}
}