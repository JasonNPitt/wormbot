////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <stdlib.h>
#include <inttypes.h>
#include <vector>
#include <SerialStream.h>
#include <fcntl.h>
#include <linux/kd.h>

#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <iomanip>
#include <cstdio>
#include <ctime>
#include <numeric>

#include <errno.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <opencv2/opencv.hpp>
#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>

#include "constants.h"




using namespace cv;
using namespace std;
using namespace LibSerial;

SerialStream ardu;
string machineZero("ZZ");
string machineMax("LL");
string readit;

int targetx,targety;
long framecounter=0;

vector <int> meanx;
vector <int> meany;

int originx=3000;
int originy=3000;




int sendCommand(string command){
	string readit;
	ardu << command << endl;
	//wait for readysignal
	while (readit.find("RR") == string::npos){ 
		getline(ardu,readit);
		cout << readit << endl;
	}
	cout << "final read:" << readit << endl; 
	return 0;
}

double avg1(std::vector<int> const& v) {
    return 1.0 * std::accumulate(v.begin(), v.end(), 0LL) / v.size();
}



 int main( int argc, char** argv )
 {

stringstream mvcmd;

mvcmd << "MX" << originx;

cout << "1" << endl;

	ardu.Open("/dev/ttyUSB0");
	ardu.SetBaudRate(SerialStreamBuf::BAUD_9600);
	ardu.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);
	cout << "2" << endl;
	sendCommand(machineMax);
	sendCommand(machineZero);
	sendCommand(mvcmd.str());
	mvcmd.str("");
	mvcmd << "MY" << originx;
	sendCommand(mvcmd.str());
	int movecounter=-1;
	
	
	

cout << "3" << endl;



    VideoCapture cap(0); //capture the video from web cam


    if ( !cap.isOpened() )  // if not success, exit program
    {
         cout << "Cannot open the web cam" << endl;
         return -1;
    }

cap.set(CV_CAP_PROP_FRAME_WIDTH, 1920);
cap.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);



    namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"

 int iLowH = 152;
 int iHighH = 179;

 int iLowS = 123; 
 int iHighS = 255;

 int iLowV = 36;
 int iHighV = 255;

 //Create trackbars in "Control" window
 //cvCreateTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
 //cvCreateTrackbar("HighH", "Control", &iHighH, 179);

 //cvCreateTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
 //cvCreateTrackbar("HighS", "Control", &iHighS, 255);

 //cvCreateTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
 //cvCreateTrackbar("HighV", "Control", &iHighV, 255);

    while (true)
    {
        Mat imgOriginal;

        bool bSuccess = cap.read(imgOriginal); // read a new frame from video

         if (!bSuccess) //if not success, break loop
        {
             cout << "Cannot read a frame from video stream" << endl;
             break;
        }

  Mat imgHSV;

  cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
 
  Mat imgThresholded;

  inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

 //morphological opening (remove small objects from the foreground)
  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );
  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) ); 

//morphological closing (fill small holes in the foreground)
  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );


// Setup SimpleBlobDetector parameters.
	SimpleBlobDetector::Params params;

	// Change thresholds
	//params.minThreshold = 10;
	//params.maxThreshold = 255;

	// Filter by Area.
	//params.filterByArea = true;
	//params.minArea = 100;

	// Filter by Circularity
	//params.filterByCircularity = true;
	//params.minCircularity = 0.1;

	

	// Filter by Inertia
	params.filterByInertia = false;
	params.filterByConvexity = false;
	//params.minInertiaRatio = 0.01;


	// Storage for blobs
	vector<KeyPoint> keypoints;



	// Set up detector with params
	Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);   

	// Detect blobs
	detector->detect( imgThresholded, keypoints);


	// Draw detected blobs as red circles.
	// DrawMatchesFlags::DRAW_RICH_KEYPOINTS flag ensures
	// the size of the circle corresponds to the size of blob

	Mat im_with_keypoints;
	drawKeypoints( imgThresholded, keypoints, im_with_keypoints, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );

	 Moments m = moments(imgThresholded, true);
	int currentx= m.m10/m.m00;
	int currenty= m.m01/m.m00;

	if (framecounter < 25 && framecounter >=0) {
		meanx.push_back(currentx);
		meany.push_back(currenty);
	}//end if first second
	else if (framecounter == 25){
		targetx = avg1(meanx);
		targety = avg1(meany);
		cout << "target is at:" << targetx << "," << targety << " for motor coordinate:" << originx << "," << originy << endl;
	}else if (framecounter > 26){
		//cout << "movecount:" << movecounter << endl;
		/*originx-=100;
		originy-=100;
		mvcmd.str("");
		mvcmd << "MX" << originx;
		sendCommand(mvcmd.str());
		mvcmd.str("");
		mvcmd << "MY" << originy;
		sendCommand(mvcmd.str());
		cout << "sent move to x,y:" << originx << "," << originy << endl;
		framecounter=-100;
		meanx.clear();
		meany.clear();


*/

		int dx = currentx - targetx; 
		int dy = currenty - targety;

		if (countNonZero(imgThresholded) < 20) cout << "No target observed" << endl; else {
	   
		   /*if(movecounter-- < 0){
			cout << currentx - targetx << "," << currenty - targety << endl;
			int dx = currentx - targetx; 
			int dy = currenty - targety;

			if (abs(dx) > 3 ){
				int my = originy + ((float)(dx)*1.14);
				stringstream cmd;
				cmd << "MY" << my;
				sendCommand(cmd.str());
				cout << "sent y command" << endl;
				movecounter=100;

			}
			if (abs(dy) > 3) {
				int mx = originx + ((float)(dy)*1.25);  //optical and robot axes are transposed
				stringstream cmd;
				cmd << "MX" << mx;
				sendCommand(cmd.str());
				cout << "sent x command" << endl;
				movecounter=100;
			}
		}*/

		
			if (dx <-10 ) for (int i=0; i < log(abs(dx)); i++ ){sendCommand("W");cout << "w:" << i << "dx:" << dx << endl;}
			if (dx > 10 ) for (int i=0; i < log(abs(dx)); i++ ){sendCommand("S");cout << "s:" << i << "dx:" << dx << endl;}
			if (dy < -10) for (int i=0; i < log(abs(dy)); i++ ){sendCommand("A");cout << "a:" << i << "dy:" << dy << endl;}
			if (dy > 10 ) for (int i=0; i < log(abs(dy)); i++ ){sendCommand("D");cout << "d:" << i << "dy:" << dy << endl;} 
			if (dx <-3 ) sendCommand("W");	
			if (dx > 3 ) sendCommand("S");
			if (dy < -3) sendCommand("A");
			if (dy > 3 ) sendCommand("D");

			cout << "target mass:" << countNonZero(imgThresholded) << endl;
		}//end if we saw a target
			
	}//do target following
	else if (framecounter > 26){
		
	}		
		
	
	    	

      
 


  

  imshow("Thresholded Image", im_with_keypoints); //show the thresholded image
 // imshow("Original", imgOriginal); //show the original image

        if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
       {
            cout << "esc key is pressed by user" << endl;
            break; 
       }


	framecounter++;
    }//end main loop

   return 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
