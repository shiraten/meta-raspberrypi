import processing.serial.*;
import peasy.*;
import processing.dxf.*;
boolean saveDXF = false; // cannot draw point

PeasyCam cam;

Serial port;
String x = "", y = "", z = "", up = "";

int sample = 2048;
float ix = 1.;
float iy = 1.;
float iz = 1.;
float mysize = 20;
int i = 0;
int nb_data = 0;
float vecx[];
float vecy[];
float vecz[];
color c;
color d;

void setup() {
    size(800, 800, P3D);
    background(255);
    
    cam = new PeasyCam(this, 100);
    cam.setMinimumDistance(50);
    cam.setMaximumDistance(500);
    
    port = new Serial(this, "/dev/ttyUSB0", 115200);
    port.bufferUntil('\n');
    
    vecx = new float[2050];
    vecy = new float[2050];
    vecz = new float[2050];
     
    c =  color(130,130,130,20);
    d =  color(255,0,0,255);
}

void draw() {
    noFill();
    if(nb_data > sample){
      background(255);
       if ( saveDXF == true ) {
          beginRaw( DXF, "mymag.dxf" );
       }
    }
    ortho(-width/mysize, width/mysize, -height/mysize, height/mysize);
    //translate(width/2, height/2, 0);
    rotateX(-PI/6);
    rotateY(PI/3);
    pushMatrix();
    stroke(c); sphere(10);
  if(nb_data > sample){
    stroke(d);
    line(200, 200, 0, (200 + 50*ix), (200 + 50*iy), (0 + 50*iz));
    for(i=0; i<sample; i++)
      point(vecx[i]/50,vecy[i]/50,vecz[i]/50);
    if ( saveDXF == true ) {
      endRaw();
      saveDXF = false;
    }
  }
  else {
    point(vecx[sample]/5,vecy[sample]/5,vecz[sample]/500);
  }
  popMatrix();
}

void serialEvent (Serial p){
  if(nb_data<=sample){
    try{
      String data = p.readStringUntil('\n');
      if(data != null){
        data = data.substring(0, data.length()-1);
        String[] dataList = split(data, ',');
        
        x = dataList[0];
        y = dataList[1];
        z = dataList[2];
        
        vecx[nb_data] = Float.parseFloat(x);
        vecy[nb_data] = Float.parseFloat(y);
        vecz[nb_data] = Float.parseFloat(z);
        
        nb_data++;  // incremente le nombre de data
      }
    }
    catch(RuntimeException e) {}
  }
}