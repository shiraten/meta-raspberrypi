/*
imu10dof01 sample sketch

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

import processing.opengl.*;
import processing.serial.*;

String q0s = "";
String q1s = "";
String q2s = "";
String q3s = "";
String rolls = "";
String pitchs = "";
String yaws = "";
String press = "";

float[] q = {0,0,0,0};
float roll = 0;
float pitch = 0;
float yaw = 0;
float pressure = 0;

Serial myPort;

void setup() {
  size(400, 450, P3D);
  //println(Serial.list());
  myPort = new Serial(this, "/dev/ttyUSB0",  115200);
  //myPort = new Serial(this, "/dev/ttyACM0",  115200);
  noLoop();
  PFont font = loadFont("ArialUnicodeMS-48.vlw"); 
  textFont(font, 14);
}

void draw() {
  background(#000000);
  fill(#ffffff);
  
  int transX = 0;
  int transY = 0;
  
  //print data  
  text("Quaternion:\n  qw: " + q[0] + "\n  qx: " + q[1] + "\n  qy: " + q[2] + "\n  qz: " + q[3], 10, 20);
  text("Pressure:\n  P: " + pressure + "\n", 140, 20);
  
  //draw rotating cube
  transX = 200;
  transY = 200;
  translate(transX, transY);
  pushMatrix();
  draw_rotocube();
  popMatrix();
  translate(-transX, -transY);
  
  //draw pitch, roll and yaw
  transX = 55;
  transY = 380;
  translate(transX, transY);
  pushMatrix();
  draw_pitchrollyaw();
  popMatrix();
  translate(-transX, -transY);
}

/*
draw a rotating cube
*/
void draw_rotocube() {
  rotateZ(yaw);
  rotateX(pitch);
  rotateY(roll);
  
  //axis
  strokeWeight(2);
  stroke(255, 0, 0);
  line(-100, 0, 0, 100, 0, 0);
  translate(+100, 0, 0);
  box(10,10,10);
  translate(-100, 0, 0);
  stroke(0, 255, 0);
  line(0, -100, 0, 0, 100, 0);
  translate(0, -100, 0);
  box(10,10,10);
  translate(0, +100, 0);
  stroke(0, 0, 255);
  line(0, 0, -100, 0, 0, 100);
  translate(0, 0, +100);
  box(10,10,10);
  translate(0, 0, -100);
  
  //box
  strokeWeight(2);
  stroke(#009999);
  fill(#009999, 220);
  box(100,100,100);
}

/*
draw pitch, roll and yaw indicators
*/
void draw_pitchrollyaw() {
  //roll circle
  pushMatrix();
  strokeWeight(2);
  stroke(#009999);
  fill(#009999, 220);
  ellipse(25, -25, 50, 50);
  
  fill(255);
  text("roll: " + nf(roll,1,5), 0, 20);
   
  translate(25, -25);
  strokeWeight(2);
  rotateZ(roll);
  stroke(255, 0, 0);
  line(-25, 0, 0, 25, 0, 0);
  popMatrix();
  
  translate(120, 0);
  //pitch circle
  pushMatrix();
  strokeWeight(2);
  stroke(#009999);
  fill(#009999, 220);
  ellipse(25, -25, 50, 50);

  fill(255);
  text("pitch: " + nf(pitch,1,5), 0, 20);
  
  translate(25, -25);
  strokeWeight(2);
  rotateZ(pitch);
  stroke(0, 255, 0);
  line(-25, 0, 0, 25, 0, 0);
  popMatrix();
  
  translate(120, 0);
  //yaw circle
  pushMatrix();
  strokeWeight(2);
  stroke(#009999);
  fill(#009999, 220);
  ellipse(25, -25, 50, 50);
  
  fill(255);
  text("yaw: " + nf(yaw,1,5), 0, 20);
  
  translate(25, -25);
  strokeWeight(2);
  rotateZ(yaw);
  stroke(0, 0, 255);
  line(-25, 0, 0, 25, 0, 0);
  popMatrix();
}
  
void serialEvent(Serial myPort) {  
  try{
    String bufread = myPort.readStringUntil('\n');
    if(bufread != null){
        bufread = bufread.substring(0, bufread.length()-1);
        String[] dataList = split(bufread, ',');
        
        q0s = dataList[0];
        q1s = dataList[1];
        q2s = dataList[2];
        q3s = dataList[3];
        rolls = dataList[4];
        pitchs = dataList[5];
        yaws = dataList[6];
        press = dataList[7];
        int i = 0;
        q[0] = Float.parseFloat(q0s);
        q[1] = Float.parseFloat(q1s);
        q[2] = Float.parseFloat(q2s);
        q[3] = Float.parseFloat(q3s);
        roll = Float.parseFloat(rolls);
        pitch = Float.parseFloat(pitchs);
        yaw = Float.parseFloat(yaws);
        pressure = Float.parseFloat(press);
    }
  }
  catch(RuntimeException e) {}
  redraw();
}