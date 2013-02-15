#define CHORDS 50
#define THRESHOLD 50 

#define FID_WIDTH 5
#define FID_LENGTH 23
#define SOLAR_RADIUS 105
#define FID_ROW_THRESH 5
#define FID_COL_THRESH 0
#define FID_MATCH_THRESH 5

#define NUM_LOCS 20

#define FRAME_PERIOD 500

#include <opencv.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <ImperxStream.hpp>
#include <processing.hpp>

cv::Mat frame;
cv::Point center;
bool enable = 1;
std::mutex enableMutex, frameMutex, centerMutex, fiducialMutex;
Semaphore frameReady, frameProcessed;
cv::Point fiducialLocations[NUM_LOCS];
int numFiducials;

void stream_image()
{    
    cv::Mat localFrame;
    int width, height;
    ImperxStream camera;
    if (camera.Connect() != 0)
    {
	std::cout << "Error connecting to camera!\n";	
    }
    else
    {
	camera.ConfigureSnap(width, height);
	localFrame.create(height, width, CV_8UC1);
	camera.Initialize();
	do
	{
	    enableMutex.lock();
	    if(!enable)
	    {
		enableMutex.unlock();
		camera.Stop();
		camera.Disconnect();
		std::cout << "Stream thread stopped\n";
		return;
	    }
	    enableMutex.unlock();
	    
	    camera.Snap(temp);

	    frameMutex.lock();
	    localFrame.copyTo(frame);
	    frameMutex.unlock();

	    frameReady.increment();
	    fine_wait(0,FRAME_PERIOD,0,0);

	} while (true);
    }
}

void process_image()
{
    cv::Size frameSize;
    cv::Mat localFrame;
    double chordOutput[6];

    cv::Mat kernel;
    cv::Mat subImage;
    int height, width;
    cv::Range rowRange, colRange;
    matchKernel(kernel);

    cv::Point localFiducialLocations[NUM_LOCS];
    int localNumFiducials;
    
    do
    {
	while(true)
	{
	    enableMutex.lock();
	    if(!enable)
	    {
		enableMutex.unlock();
		std::cout << "Chord thread stopped.\n";
		return;
	    }
	    enableMutex.unlock();
	    
	    try
	    {
		frameReady.decrement();
		break;
	    }
	    catch(const char* e)
	    {
		fine_wait(0,10,0,0);
	    }
	}

	frameMutex.lock();
	localFrame = frame.getMat();
	frameMutex.unlock();
	

	frameSize = localFrame.size();
	height = frameSize.height;
	width = frameSize.width;
	chordCenter((const unsigned char*) localFrame.data, height, width, CHORDS, THRESHOLD, chordOutput);
       
	centerMutex.lock();
	center.x = chordOutput[0];
	center.y = chordOutput[1];
	centerMutex.unlock();

	if (chordOutput[0] > 0 && chordOutput[1] > 0 && chordOutput[0] < width && chordOutput[1] < height)
	{
	    rowRange.end = (((int) chordOutput[1]) + SOLAR_RADIUS < height-1) ? (((int) chordOutput[1]) + SOLAR_RADIUS) : (height-1);
	    rowRange.start = (((int) chordOutput[1]) - SOLAR_RADIUS > 0) ? (((int) chordOutput[1]) - SOLAR_RADIUS) : 0;
	    colRange.end = (((int) chordOutput[0]) + SOLAR_RADIUS < width) ? (((int) chordOutput[0]) + SOLAR_RADIUS) : (width-1);
	    colRange.start = (((int) chordOutput[0]) - SOLAR_RADIUS > 0) ? (((int) chordOutput[0]) - SOLAR_RADIUS) : 0;
	    subImage = localFrame(rowRange, colRange);
	    localNumFiducials = matchFindFiducials(subImage, kernel, FID_MATCH_THRESH, localFiducialLocations, NUM_LOCS);
	}
	
	
	fiducialMutex.lock();
	numFiducials = localNumFiducials;
	for (int k = 0; k < localNumFiducials; k++)
	{
	    fiducialLocations[k].x = localFiducialLocations[k].x + colRange.start;
	    fiducialLocations[k].y = localFiducialLocations[k].y + rowRange.start;
	}
	fiducialMutex.unlock();
	
	//frameProcessed.increment();
    } while(true);		        
}

void display_image()
{
    cv::Mat frame;

    do
    {
	while(true)
	{
	    (enableMutex).lock();
	    if(!enable)
	    {
		(enableMutex).unlock();			     
		std::cout << "Display thread stopped\n";
		return;
	    }
	    enableMutex.unlock();	

	}

    } while(true);
}

int main()
{

    std::thread stream(stream_image);
    std::thread process(process_image);
    std::thread show(display_image);
    
    fine_wait(30,0,0,0);
    
    en_mtx.lock();
    en = 0;
    en_mtx.unlock();

    stream.join();
    process.join();
    show.join();

    std::cout << "All threads stopped. Exiting\n";
    return 0;
}
