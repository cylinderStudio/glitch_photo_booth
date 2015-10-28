#pragma once

#include "ofMain.h"
#include "ofxVideoRecorder.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxTimer.h"

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    void exit();
    
    void keyPressed  (int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void clearFbo(ofFbo& fbo);
    void startRecording(const unsigned long long duration);
    void stopRecording();
    void sendHeartbeat(ofEventArgs &args);
    string generateTimeStamp(unsigned long long time);
    void drawButton(const ofVec2f& center);
    const int buttonSize = 50;
    const int DURATION = 6000;
    const int RECEIVE_PORT = 12345;
    const int SEND_PORT = 12346;
    ofVec2f buttonCenter;

    ofMesh quad;
    
    bool hideGui;
    ofxPanel gui;
    ofParameterGroup parameters;
    ofParameterGroup BadTV;
    ofParameter<float> thickDistort;
    ofParameter<float> fineDistort;
    ofParameter<float> distortSpeed;
    ofParameter<float> rollSpeed;
    float time;
    
    ofParameterGroup RGBShift;
    ofParameter<float> rgbAmount;
    ofParameter<float> angle;
    
    ofParameterGroup Static;
    ofParameter<float> staticAmount;
    ofParameter<float> size;
    
    ofParameterGroup Scanlines;
    ofParameter<int> count;
    ofParameter<float> sIntensity;
    ofParameter<float> nIntensity;
    
    ofVideoGrabber      vidGrabber;
    ofxVideoRecorder    vidRecorderMP4;
    ofxVideoRecorder    vidRecorderMP4Distort;

    string fileName;
    string fileExt;
    string lastFile;
    string code;
    
    ofFbo badTVFbo;
    ofFbo rgbShiftFbo;
    ofFbo staticFbo;
    ofFbo filmFbo;
    
    ofShader badTvShader, rgbShiftShader, filmShader, staticShader;
    ofTrueTypeFont openSansLarge, openSansRegular;
    
    ofxOscReceiver receiver;
    ofxOscSender sender;
    
    ofArduino	arduino;
    bool		isArduinoSetup;			// flag variable for setting up arduino once
    void setupArduino(const int & version);
    void analogPinChanged(const int & pinNum);
    void updateArduino();
    
    void timerFinished(ofEventArgs& arg);
    
private:
    int programState;
    string serverPID; //process ID for node server
    ofxTimer timer, heartbeatTimer;
    
    enum States{
        READY,
        RECORDING,
        PROCESSING,
        FINISHED,
        ERROR
    };
};
