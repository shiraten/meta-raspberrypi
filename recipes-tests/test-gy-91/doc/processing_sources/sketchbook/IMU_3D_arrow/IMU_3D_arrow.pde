import processing.serial.*;

Serial port;
String x = "", y = "", z = "", up = "";
Float roll = 0.0, pitch = 0.0, yaw = 0.0, slide = 0.0;
PImage arrows, reference;
PFont font;

void setup(){
  size(800,600, P3D);
  
  arrows = loadImage("arrows.png");
  reference = loadImage("reference.png");
  
  font = loadFont("Helvetica-Light-48.vlw");
  textFont(font,48);
  
  port = new Serial(this, "/dev/ttyACM0", 115200);
  port.bufferUntil('\n');
}

void draw(){
  background(255);
  textFont(font, 40);
  
  //REFERENCE TEXT
  fill(254, 210, 2);
  text("X: "+x,10,260);
  fill(26, 188, 156);
  text("Y: "+y,10,320);
  fill(252, 148, 3);
  text("Z: "+z,10,380);
  
  //REFERENCE ARROWS
  pushMatrix();
  imageMode(CORNERS);
  image(reference,10,10);
  popMatrix();
  
  //ARROWS
  pushMatrix();
  imageMode(CENTER);
  translate(width/2, height/2);
  rotateX(radians(pitch+90.0));
  rotateY(radians(roll));
  rotateZ(radians(yaw));
  image(arrows,0,0);
  popMatrix();
  
  //SLIDER 
  line(width-50, 50, width-50, height-50);
  line(width-65, 50, width-35, 50);
  line(width-65, height-50, width-35, height-50);
  line(width-60, height/2, width-40, height/2);
  fill(0, 0, 0);
  rectMode(CENTER);
  rect(width-50, (128-slide)*2+45, 50, 10);
}

void serialEvent (Serial p){
  try{
    String data = p.readStringUntil('\n');
    if(data != null){
      data = data.substring(0, data.length()-1);
      String[] dataList = split(data, ',');
      
      y = dataList[0];
      x = dataList[1];
      z = dataList[2];
      up = dataList[3];
      
      pitch = Float.parseFloat(y);
      roll = Float.parseFloat(x);
      yaw = Float.parseFloat(z);
      slide = Float.parseFloat(up)*64;
    }
  }
  catch(RuntimeException e) {}
}