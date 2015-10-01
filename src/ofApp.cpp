#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex();
    
    ofSetFrameRate(60);
    ofSetLogLevel(OF_LOG_VERBOSE);
    vidGrabber.setDesiredFrameRate(30);
    vidGrabber.initGrabber(640, 480);
    
    fileName = "testMovie";
    fileExt = ".mov"; // ffmpeg uses the extension to determine the container type. run 'ffmpeg -formats' to see supported formats
    
    // override the default codecs if you like
    // run 'ffmpeg -codecs' to find out what your implementation supports (or -formats on some older versions)
    string bitRate = "800k";
    //    vidRecorder.setVideoCodec("libx264");
    vidRecorder.setVideoBitrate(bitRate);
    vidRecorderMP4.setVideoBitrate(bitRate);
    vidRecorderMP4.setVideoCodec("libx264");
    vidRecorderMP4Distort.setVideoBitrate(bitRate);
    vidRecorderMP4Distort.setVideoCodec("libx264");
    
    bRecording = false;
    ofEnableAlphaBlending();
    
    //shader stuff
    filmFbo.allocate(vidGrabber.getWidth(), vidGrabber.getHeight());
    badTVFbo.allocate(vidGrabber.getWidth(), vidGrabber.getHeight());
    rgbShiftFbo.allocate(vidGrabber.getWidth(), vidGrabber.getHeight());
    staticFbo.allocate(vidGrabber.getWidth(), vidGrabber.getHeight());
    clearFbo(filmFbo);
    clearFbo(badTVFbo);
    clearFbo(rgbShiftFbo);
    clearFbo(staticFbo);
    
    //set up basic quad
    quad.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    quad.addVertex(ofPoint(0, vidGrabber.getWidth(), 0));
    quad.addTexCoord(ofPoint(0, vidGrabber.getWidth(), 0));
    quad.addVertex(ofPoint(0, 0, 0));
    quad.addTexCoord(ofPoint(0, 0, 0));
    quad.addVertex(ofPoint(vidGrabber.getWidth(), vidGrabber.getHeight(), 0));
    quad.addTexCoord(ofPoint(vidGrabber.getWidth(), vidGrabber.getHeight(), 0));
    quad.addVertex(ofPoint(vidGrabber.getWidth(), 0, 0));
    quad.addTexCoord(ofPoint(vidGrabber.getWidth(), 0, 0));
    
    filmShader.load("shaders/passthrough_vert.c", "shaders/film_shader.c");
    badTvShader.load("shaders/passthrough_vert.c", "shaders/badtv_frag.c");
    rgbShiftShader.load("shaders/passthrough_vert.c", "shaders/rgb_shift_frag.c");
    staticShader.load("shaders/passthrough_vert.c", "shaders/static_frag.c");
    
    //gui stuff
    BadTV.setName("Bad TV");
    BadTV.add(thickDistort.set("Thick Distort", 0.1, 0.1, 20));
    BadTV.add(fineDistort.set("Fine Distort", 0.1, 0.1, 20));
    BadTV.add(distortSpeed.set("Distort Speed", 0.0, 0.0, 1.0));
    BadTV.add(rollSpeed.set("Roll Speed", 0.0, 0.0, 1.0));
    time = 0.0;
    
    RGBShift.setName("RGB Shift");
    RGBShift.add(rgbAmount.set("Amount", 0.0, 0.0, 0.1));
    RGBShift.add(angle.set("Angle", 0.0, 0.0, 2.0));
    
    Static.setName("Static");
    Static.add(staticAmount.set("Amount", 0.0, 0.0, 1.0));
    Static.add(size.set("Size", 1, 1, 100));
    
    Scanlines.setName("Scanlines");
    Scanlines.add(count.set("Count", 800, 50, 1000));
    Scanlines.add(sIntensity.set("sIntensity", 0.4, 0, 2));
    Scanlines.add(nIntensity.set("nIntensity", 0.1, 0, 2));
    
    parameters.add(BadTV);
    parameters.add(RGBShift);
    parameters.add(Static);
    parameters.add(Scanlines);
    
    gui.setup(parameters);
    gui.minimizeAll();
    
    //for testing so I don't have to keep taking videos
//    recordedVideoPlayback.load("test_video.mov");
//    recordedVideoPlayback.play();
    
    recordPixels.allocate(vidGrabber.getWidth(), vidGrabber.getHeight(), OF_IMAGE_COLOR);
    recordImage.allocate(vidGrabber.getWidth(), vidGrabber.getHeight(), OF_IMAGE_COLOR);
}

void ofApp::exit() {
    vidRecorder.close();
    vidRecorderMP4.close();
    vidRecorderMP4Distort.close();
}

//--------------------------------------------------------------
void ofApp::update(){
    vidGrabber.update();
    if(vidGrabber.isFrameNew() && bRecording){

        if (!vidRecorder.addFrame(vidGrabber.getPixels())){
            ofLogWarning("This frame was not added to the main recorder!");
        }
        
        if( !vidRecorderMP4.addFrame(vidGrabber.getPixels())){
            ofLogWarning("This frame was not added to the mp4 recorder!");
        }
        
        filmFbo.begin();
        filmShader.begin();
        filmShader.setUniformTexture("tDiffuse", vidGrabber.getTexture(), 1);
        filmShader.setUniform1f("width", vidGrabber.getWidth());
        filmShader.setUniform1f("height", vidGrabber.getHeight());
        filmShader.setUniform1f("time", time);
        filmShader.setUniform1i("grayscale", 0);
        filmShader.setUniform1f("nIntensity", nIntensity);
        filmShader.setUniform1f("sIntensity", sIntensity);
        filmShader.setUniform1f("sCount", count);
        quad.draw();
        filmShader.end();
        filmFbo.end();
        
        badTVFbo.begin();
        badTvShader.begin();
        badTvShader.setUniformTexture("tDiffuse", filmFbo.getTexture(), 1);
        badTvShader.setUniform1f("width", vidGrabber.getWidth());
        badTvShader.setUniform1f("height", vidGrabber.getHeight());
        badTvShader.setUniform1f("time", time);
        badTvShader.setUniform1f("distortion", thickDistort);
        badTvShader.setUniform1f("distortion2", fineDistort);
        badTvShader.setUniform1f("speed", distortSpeed);
        badTvShader.setUniform1f("rollSpeed", rollSpeed);
        quad.draw();
        badTvShader.end();
        badTVFbo.end();
        
        rgbShiftFbo.begin();
        rgbShiftShader.begin();
        rgbShiftShader.setUniformTexture("tDiffuse", badTVFbo.getTexture(), 1);
        rgbShiftShader.setUniform1f("width", vidGrabber.getWidth());
        rgbShiftShader.setUniform1f("height", vidGrabber.getHeight());
        rgbShiftShader.setUniform1f("amount", rgbAmount);
        rgbShiftShader.setUniform1f("angle", angle);
        quad.draw();
        rgbShiftShader.end();
        rgbShiftFbo.end();
        
        staticFbo.begin();
        staticShader.begin();
        staticShader.setUniformTexture("tDiffuse", rgbShiftFbo.getTexture(), 1);
        staticShader.setUniform1f("width", vidGrabber.getWidth());
        staticShader.setUniform1f("height", vidGrabber.getHeight());
        staticShader.setUniform1f("time", time);
        staticShader.setUniform1f("amount", staticAmount);
        staticShader.setUniform1f("size", size);
        quad.draw();
        staticShader.end();
        staticFbo.end();
        
        ofPixels px;
        staticFbo.readToPixels(px);
        ofImage img;
        img.setFromPixels(px);
        
        if (!vidRecorderMP4Distort.addFrame(img)){
            ofLogWarning("This frame was not added to the distorted mp4 recorder!");
        }
    }
    
    // Check if the video recorder encountered any error while writing video frame or audio smaples.
    if (vidRecorder.hasVideoError()) {
        ofLogWarning("The main video recorder failed to write some frames!");
    }
    if( vidRecorderMP4.hasVideoError()){
        ofLogWarning("The mp4 video recorder failed to write some frames!");
    }
    if(vidRecorderMP4Distort.hasVideoError()){
        ofLogWarning("The distorted mp4 video recorder failed to write some frames!");
    }
    
    if(recordedVideoPlayback.isLoaded()){
        recordedVideoPlayback.update();
    }
    
    time += 0.1;
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255, 255, 255);
    vidGrabber.draw(0,0);
    
    stringstream ss;
    ss << "video queue size: " << vidRecorder.getVideoQueueSize() << endl
    << "FPS: " << ofGetFrameRate() << endl
    << (bRecording?"pause":"start") << " recording: r" << endl
    << (bRecording?"close current video file: c":"") << endl;
    
    ofSetColor(0,0,0,100);
    ofRectangle(0, 0, 260, 75);
    ofSetColor(255, 255, 255);
    ofDrawBitmapString(ss.str(),15,15);
    
    if(bRecording){
        ofPushStyle();
        ofSetColor(255, 0, 0);
        ofDrawCircle(ofGetWidth() - 20, 20, 5);
        ofPopStyle();
    }
    
    //recorded video
    if(recordedVideoPlayback.isLoaded()){
        
        filmFbo.begin();
        filmShader.begin();
        filmShader.setUniformTexture("tDiffuse", recordedVideoPlayback.getTexture(), 1);
        filmShader.setUniform1f("width", vidGrabber.getWidth());
        filmShader.setUniform1f("height", vidGrabber.getHeight());
        filmShader.setUniform1f("time", time);
        filmShader.setUniform1i("grayscale", 0);
        filmShader.setUniform1f("nIntensity", nIntensity);
        filmShader.setUniform1f("sIntensity", sIntensity);
        filmShader.setUniform1f("sCount", count);
        quad.draw();
        filmShader.end();
        filmFbo.end();
        
        badTVFbo.begin();
        badTvShader.begin();
        badTvShader.setUniformTexture("tDiffuse", filmFbo.getTexture(), 1);
        badTvShader.setUniform1f("width", vidGrabber.getWidth());
        badTvShader.setUniform1f("height", vidGrabber.getHeight());
        badTvShader.setUniform1f("time", time);
        badTvShader.setUniform1f("distortion", thickDistort);
        badTvShader.setUniform1f("distortion2", fineDistort);
        badTvShader.setUniform1f("speed", distortSpeed);
        badTvShader.setUniform1f("rollSpeed", rollSpeed);
        quad.draw();
        badTvShader.end();
        badTVFbo.end();
        
        rgbShiftFbo.begin();
        rgbShiftShader.begin();
        rgbShiftShader.setUniformTexture("tDiffuse", badTVFbo.getTexture(), 1);
        rgbShiftShader.setUniform1f("width", vidGrabber.getWidth());
        rgbShiftShader.setUniform1f("height", vidGrabber.getHeight());
        rgbShiftShader.setUniform1f("amount", rgbAmount);
        rgbShiftShader.setUniform1f("angle", angle);
        quad.draw();
        rgbShiftShader.end();
        rgbShiftFbo.end();
    
        staticFbo.begin();
        staticShader.begin();
        staticShader.setUniformTexture("tDiffuse", rgbShiftFbo.getTexture(), 1);
        staticShader.setUniform1f("width", vidGrabber.getWidth());
        staticShader.setUniform1f("height", vidGrabber.getHeight());
        staticShader.setUniform1f("time", time);
        staticShader.setUniform1f("amount", staticAmount);
        staticShader.setUniform1f("size", size);
        quad.draw();
        staticShader.end();
        staticFbo.end();
        
        //final output
        staticFbo.draw(640, 0);
    }
    
    if( hideGui ){
        ofShowCursor();
        gui.draw();
    }else{
        ofHideCursor();
    }
}

//--------------------------------------------------------------
void ofApp::clearFbo(ofFbo &fbo){
    fbo.begin();
    ofClear(255,255,255, 0);
    fbo.end();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
    if(key=='r'){
        //TODO: remove stop/start functionality
        //TODO: move this and stop into methods. maintain keyboard bindings
        bRecording = !bRecording;
        if(bRecording && !vidRecorder.isInitialized()) {
            lastFile = fileName+ofGetTimestampString()+fileExt;
            vidRecorder.setupCustomOutput(vidGrabber.getWidth(), vidGrabber.getHeight(), 30, 0, 0, "-vcodec libx264 -b 800k -pix_fmt yuv420p -f mov " + ofFilePath::getAbsolutePath(lastFile), true, false); //the last booleans sync the video timing to the main thread
            vidRecorderMP4.setup(fileName+ofGetTimestampString()+".mp4", vidGrabber.getWidth(), vidGrabber.getHeight(), 30, true, false); // no audio
            vidRecorderMP4Distort.setup(fileName+ofGetTimestampString()+"_distorted.mp4", vidGrabber.getWidth(), vidGrabber.getHeight(), 30, true, false); // no audio
            
            vidRecorder.start();
            vidRecorderMP4.start();
            vidRecorderMP4Distort.start();
        }
        else if(!bRecording && vidRecorder.isInitialized()) {
            vidRecorder.setPaused(true);
        }
        else if(bRecording && vidRecorder.isInitialized()) {
            vidRecorder.setPaused(false);
        }
    }
    if(key=='c'){
        bRecording = false;
        vidRecorder.close();
        vidRecorderMP4.close();
        vidRecorderMP4Distort.close();
        
        //race condition!
        ofSleepMillis(750);
        
        //try and load the video
        recordedVideoPlayback.load(lastFile);
        recordedVideoPlayback.play();
        
        //signal via osc that we've saved a new set of videos
    }if(key=='q'){
        ofExit();
    }
    if(key=='h'){
        hideGui = !hideGui;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}
