//============================================================================
// Name        : controller.cpp  | with  dot following
// Authors     : Jason N Pitt and Nolan Strait
// Version     :
// Copyright   : MIT LICENSE
// Description : robot controller for worm lifespans
//============================================================================

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

uint8_t *buffer;

using namespace std;
using namespace LibSerial;


#define TESTING false // speeds up process for faster debugging

#define BLANKUPDATE ",,,,,,,"

#define PORT "/dev/ttyUSB0" //This is system-specific"/tmp/interceptty" we need to make this configurable


#define MAX_PLATES 12
#define MAX_WELLS 12
#define WELL_WAIT_PERIOD 2  //pause between wells
#define SCAN_PERIOD (TESTING ? 30 : 600)   // time between scans (default 600sec/10min)
#define LOAD_WAIT_PERIOD (TESTING ? 20 : 120) // default 120sec/2min
#define SCAN_COMPLETE_TIMEOUT 1800//maximum time to wait for a scan before resetting robot state 30 min

#define WELL_STATE_START 2
#define WELL_STATE_ACTIVE 1
#define WELL_STATE_STOP 0

#define ROBOT_STATE_SCANNING 1
#define ROBOT_STATE_WAIT 2
#define ROBOT_STATE_LOAD 3

#define MONITOR_STATE_OFF -1 // experimenter doesn't want monitoring
#define MONITOR_STATE_START -2 // experimenter requests monitoring

#define NUM_WELLS 144

#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600

#define ACCEPTABLE_JITTER 2
#define JITTER_WAIT 500
#define CALIBRATE_FREQ 36 //number of scans between calibration runs, 144 once per day



//GLOBALS

stringstream root_dir;

SerialStream ardu;
string datapath;
int cameranum=0;
string logfilename = datapath + "/robot.log";
ofstream logfile(logfilename.c_str(), ofstream::app);
streambuf *coutbuf = std::cout.rdbuf(); //save old buf
string VERSION = "Release 1.01";

int calibration_counter=0;
int currMonitorSlot;

bool doDotFollow = false;

//color variables for finding pink markers
int iLowH = 152;
int iHighH = 179;
int iLowS = 123; 
int iHighS = 255;
int iLowV = 36;
int iHighV = 255;



//returns the number of seconds since midnight
time_t getSecondsSinceMidnight() {
    time_t t1, t2;
    struct tm tms;
    time(&t1);
    localtime_r(&t1, &tms);
    tms.tm_hour = 0;
    tms.tm_min = 0;
    tms.tm_sec = 0;
    t2 = mktime(&tms);
    return t1 - t2;
}


// calculates current monitor slot based on time of day
int calcCurrSlot() {
	
	int currTime = getSecondsSinceMidnight() ; // get time of day
	//cout << "calccurrtime currtime =" << currTime << endl; 
	return (int) ((double) currTime / SECONDS_IN_DAY * NUM_WELLS);
}


int sendCommand(string command){
	string read;
	ardu << command << endl;
	//wait for readysignal
	while (read.find("RR") == string::npos){ getline(ardu,read);}
	return 0;
}

//returns the average of a vector of int
double avg1(std::vector<int> const& v) {
    return 1.0 * std::accumulate(v.begin(), v.end(), 0LL) / v.size();
}



void setCameraSaturation(int sat){
	


	//set camera to color:

		string camfile(datapath + string("camera.config"));
		ifstream inputfile(camfile.c_str());
		string cameradevice;
		getline(inputfile,cameradevice);
		stringstream camerasettings;
		camerasettings << "v4l2-ctl -d " << cameradevice <<" -c saturation=" << sat  << endl;
		cout << "set sat command:" << camerasettings.str() << endl;
		system(camerasettings.str().c_str());
}//end setcameratocolor



class Timer {
public:
	long systemstarttime,seconds,delay;
	double msdelay;
	struct timeval start;
	bool ms;

	Timer(long sseconds){
		gettimeofday(&start, NULL);
		delay = sseconds;
		ms=0;
		msdelay=0;
	}

	Timer(void){
		gettimeofday(&start, NULL);
		delay = 0;
		ms=0;
		msdelay=0;
	}

	Timer(double msec,bool mms){
		gettimeofday(&start,NULL);
		msdelay=msec;
		ms=1;
	}

	void startTimer(long time){
		gettimeofday(&start, NULL);
		if (ms){
			msdelay=time;
		}else{
			delay=time;
		}
	}

	void startTimer(double time){
		gettimeofday(&start, NULL);
		if (ms){
			msdelay=time;
		}else{
			delay=time;
		}
	}

	double getTimeElapsed(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		stringstream ss;
		ss << currtime.tv_sec;

		string seconds(ss.str());
		ss.str("");
		ss.clear();
		ss << (double)currtime.tv_usec/(double)1000000;
		string micros(ss.str());

		micros.erase(0,1);
		seconds.append(micros);
		double curr = atof(seconds.c_str());
		ss.str("");
		ss.clear();

		ss << start.tv_sec;

		string sseconds(ss.str());
		ss.str("");
		ss.clear();
		ss << (double)start.tv_usec/(double)1000000;
		string smicros(ss.str());
		smicros.erase(0,1);
		sseconds.append(smicros);
		double scurr = atof(sseconds.c_str());

		return curr-scurr;
	}

	int getSeconds(void){
		struct timeval currtime;
		gettimeofday(&currtime,NULL);
		return (int)(currtime.tv_sec-start.tv_sec);
	}

	void printTimer(void){
		struct timeval currtime;
		gettimeofday(&currtime,NULL);
		cout << (currtime.tv_sec-start.tv_sec);
	}

	bool checkTimer(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		if(ms){
			if (getTimeElapsed() >= msdelay)
				return true;
			else
				return false;
		}else{
			if (currtime.tv_sec-start.tv_sec >= delay)
				return true;
			else
				return false;
		}
	}

};


class Well {
public:
	string email;
	string title;
	string investigator;
	string description;
	//struct timeval starttime;
	time_t starttime;
	long currentframe;
	int active;
	int status;
	string directory;
	int startingN;	   //number of worms put onto plate
	int startingAge;   //in days;
	int busy;
	int finished;
	long expID;
	int xval;
	int yval;
	int plate;
	int rank;
	bool transActive;
	bool gfpActive;
	bool timelapseActive;
	int monitorSlot;
	string wellname;
	string strain;
	int targetx; //hold the centroid coordinates of pink target
	int targety; //	
	bool hasTracking;


	Well(void){}
	//int getRank(string thewelltorank);

	Well(string inputline) {
		stringstream ss(inputline);
		string token;

		busy = 0;
		finished = 0;
		active = 1;

		//gettimeofday(&starttime, NULL);
		getline(ss, token, ',');
		expID = atol(token.c_str());
		getline(ss, token, ',');
		status = atoi(token.c_str());
		getline(ss, token, ',');
		plate = atoi(token.c_str());
		getline(ss, wellname, ',');
		getline(ss, token, ',');
		xval = atoi(token.c_str());
		getline(ss, token, ',');
		yval = atoi(token.c_str());
		getline(ss, directory, ',');
		getline(ss, token, ',');
		timelapseActive = atoi(token.c_str());
		getline(ss, token, ',');
		monitorSlot = atoi(token.c_str());
		getline(ss, email, ',');
		getline(ss, investigator, ',');
		getline(ss, title, ',');
		getline(ss, description, ',');
		getline(ss, token, ',');
		startingN = atoi(token.c_str());
		getline(ss, token, ',');
		startingAge = atoi(token.c_str()); //days old the worms are at start of recording
		getline(ss, strain, ',');
		getline(ss, token, ',');
		currentframe = atol(token.c_str());
		token = "";
		getline(ss, token, ',');
		if (token != "") {
			starttime = (time_t) atol(token.c_str());
		} else { // this is only here for backwards compatilibity
			string fileToCheck;
			if (timelapseActive)
				fileToCheck = directory + string("/frame000000.png");
			else
				fileToCheck = directory + string("/day0.avi");
			struct stat t_stat;
			stat(fileToCheck.c_str(), &t_stat);
			starttime = t_stat.st_mtime;
		}

		string torank = boost::lexical_cast<string>(plate) + wellname;
		rank = getRank(torank);

		//set default target locks to -1
		targetx=-1;
		targety=-1;

		//check for presence of a loc-nar file
		string locfile;
		string targets;
		locfile= directory + "loc-nar.csv";			
		ifstream locnar(locfile.c_str());
		if (locnar.good()){
			token="";
			getline(locnar,token, ',');
			targetx = atol(token.c_str());
			token="";
			getline(locnar,token, ',');
			targety = atol(token.c_str());
			hasTracking=true;
						
		} else hasTracking=false;//end if there was a loc-nar
			
		//printDescriptionFile();
	}   //end construct

	/**
	 * Creates a new Well object.
	 * @filename : the JSON file to extract well data from
	 */
	/*Well (string filename) {
		// read in JSON from file
		ifstream s(filename);
		json j;
		s >> j;

		gettimeofday(&starttime, NULL);

		expID = j["expID"];
		status = j["status"];
		plate = j["plate"];
		wellname = j["name"];
		xval = j["x"];
		yval = j["y"];
		directory = j["dir"];
		timelapseActive = j["TLA"];
		dailymonitorActive = j["DLA"];
		email = j["email"];
		investigator = j["investigator"];
		title = j["title"];
		description = j["description"];
		startingN = j["startNum"];
		startingAge = j["startAge"];
		strain = j["strain"];
		currentframe = j["frame"];

		string torank = boost::lexical_cast<string>(plate) + wellname;
		rank = getRank(torank);
	}*/

	void setStatus(int setstat){
		status = setstat;
	}

    string printWell(void){
    	stringstream ss;
    	ss << expID << "," << status << "," << plate << "," << wellname << "," << xval
				<< "," << yval<< "," << directory
    			<< "," << timelapseActive << "," << monitorSlot << "," << email
    			<< "," << investigator << "," << title << "," << description << "," << startingN
				<< "," << startingAge << "," << strain
    			<<  "," << currentframe << "," << starttime << endl;
    	return (ss.str());
    }


	int getRank(string thewelltorank) {

		vector<string> wellorder;

		string filename;
		filename = datapath + string("/platecoordinates.dat");
		ifstream ifile(filename.c_str());
		string readline;

		string token;

		while (getline(ifile, readline)) {
			stringstream awell(readline);
			string thewell;
			getline(awell, thewell, ',');

			wellorder.push_back(thewell);

		}   //end while lines in the file

		for (int i = 0; i < (int) wellorder.size(); i++) {
			if (thewelltorank.find(wellorder[i]) == 0)
				return (i);
		}   //end for each well

		return (0);
	}


	bool operator <(const Well str) const {
		return (rank < str.rank);
	}


	void printDescriptionFile(void){
		string filename;
		filename = directory + string("description.txt");
		ofstream ofile(filename.c_str());

		ofile << "****************************************************************\n";
		ofile << title << endl;
		ofile << email << endl;
		ofile << investigator << endl;
		ofile << description << endl;
		ofile << starttime << endl;
		ofile << currentframe << endl;
		ofile << strain << endl;
		ofile << active << endl;
		ofile << directory << endl;
		ofile << startingN << endl;	   //number of worms put onto plate
		ofile << startingAge << endl;   //in days;
		ofile << "expID:" << expID << endl;
		ofile << xval << endl;
		ofile << yval << endl;
		ofile << plate << endl;
		ofile << wellname << endl;
		ofile << "****************************************************************\n";
		ofile << "::br::" << endl;
		ofile.close();
	}


	void printSpecs(void) {
		cout << "****************************************************************\n";
		cout << title << endl;
		cout << email<< endl;
		cout << investigator<< endl;
		cout << description<< endl;
		cout << starttime << endl;
		cout <<currentframe<< endl;
		cout << active<< endl;
		cout << directory<< endl;
		cout << startingN<< endl;	   //number of worms put onto plate
		cout << startingAge<< endl;   //in days;
		cout << "expID:" << expID<< endl;
		cout << xval<< endl;
		cout << yval<< endl;
		cout << plate<< endl;
		cout << wellname<< endl;
		cout << "rank " << rank <<endl;
		cout << "****************************************************************\n";
	}


	// returns age of worms in days
	int getCurrAge(void) {
		time_t current;
		time(&current);
		return (int)(current - starttime + SECONDS_IN_HOUR) / SECONDS_IN_DAY + startingAge;
	}


	int capture_frame(int doAlign){

		try {

			vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION); //(CV_IMWRITE_PXM_BINARY);
			compression_params.push_back(0);

			setCameraSaturation(100);

			VideoCapture cap(cameranum); // open the default camera

			long c = 0;
			while (!cap.isOpened()) { // check if we succeeded
				if (c < 3) sleep(1);
				else {
					cout << "  camera could not be opened" << endl;
					return -1;
				}
				c++;
			} //end while not opened

			cap.set(CV_CAP_PROP_FRAME_WIDTH, 1920);
			cap.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);
			cout << "cv sat set:" << cap.get(CV_CAP_PROP_SATURATION) << endl;

			if ((int)cap.get(CV_CAP_PROP_FRAME_WIDTH) != 1920
				|| (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT) != 1080)
				cout << "  cannot adjust video capture properties!" << endl;

			Mat frame;
			Mat frame_gray;

			// capture frame
			cap >> frame;
			int i = 0;
			while (frame.empty() && i < 3) {
				sleep(1);
				cap >> frame;
				i++;
			}

			if (i == 3) { // no frame captured
				cout << "  unable to capture frame! (expID: " << expID << ")" << endl;
				return 0;
			}

		    

			Mat lastframe;
			Mat im2_aligned;

			stringstream filename;
			stringstream lastfilename;
			stringstream number;
			number << setfill('0') << setw(6) << currentframe;
			
			filename << directory << "frame" << number.str() << ".png";

			// for first frame try to find a pink bead and if found generate a loc-nar.csv file
			cout << "wellname:" << wellname << " currentframe:" << currentframe << endl;
			if (currentframe ==0){
				cap.set(CV_CAP_PROP_SATURATION,100);
				cout << "frame 0 cv sat set:" << cap.get(CV_CAP_PROP_SATURATION) << endl;
				
				cap >> frame;
				i = 0;
				while (frame.empty() && i < 3) {
					sleep(1);
					cap >> frame;
					i++;
				}
				Mat imgHSV;

				cvtColor(frame, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
 
  				Mat imgThresholded;

  				inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

				//morphological opening (remove small objects from the foreground)
				erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
				dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 

				//morphological closing (fill small holes in the foreground)
				dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
				erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
				
				

				if (countNonZero(imgThresholded) > 300 && doDotFollow) { //if saw a bead
					string dotfile = directory + string("legendarydots.png");
					cout << "dotfile:" << dotfile << endl;	
					imwrite(dotfile, imgThresholded, compression_params);
					Moments m = moments(imgThresholded, true);
					targetx= m.m10/m.m00;
					targety= m.m01/m.m00;
					string denofearth = directory + string("loc-nar.csv");
					cout << "making locnar file:" << denofearth << endl;
					ofstream den(denofearth.c_str());
					den << targetx << "," << targety << endl;
					den.close();					
				}//end if saw a bead
				else cout << "mass was:" << countNonZero(imgThresholded);


				setCameraSaturation(0);								
				
				 


			}//end if first frame

			
			cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray




			if (doAlign == 0) {
				imwrite(filename.str(), frame_gray, compression_params); //frame vs frame_gray
			}

			// for other frames
			else {
				number.str("");
				number << setfill('0') << setw(6) << currentframe - 1;
			
				lastfilename << directory << "frame" << number.str() << ".pgm";
				Mat im1 = imread(lastfilename.str());
				Mat im1_gray;
				cvtColor(im1, im1_gray, CV_BGR2GRAY);

				// Define the motion model
				const int warp_mode = MOTION_TRANSLATION;

				// Set a 2x3 warp matrix
				Mat warp_matrix;
				warp_matrix = Mat::eye(2, 3, CV_32F);

				// Specify the number of iterations.
				int number_of_iterations = 5000;

				// Specify the threshold of the increment
				// in the correlation coefficient between two iterations
				double termination_eps = 1e-10;

				// Define termination criteria
				TermCriteria criteria(TermCriteria::COUNT + TermCriteria::EPS,
						number_of_iterations, termination_eps);

				// Run the ECC algorithm. The results are stored in warp_matrix.
				findTransformECC(im1_gray, frame_gray, warp_matrix, warp_mode, criteria);
				warpAffine(frame, im2_aligned, warp_matrix, im1.size(),
						INTER_LINEAR + WARP_INVERSE_MAP);
				Mat im2_aligned_gray;
				cvtColor(im2_aligned, im2_aligned_gray, CV_BGR2GRAY);
				imwrite(filename.str(), im2_aligned_gray, compression_params);
				cap.release();

			}

			currentframe++;
			cap.release();
		} catch (cv::Exception ex) {
			cout << " frame was bad! try again" << endl;
			return (0);
		} //end caught exception trying to load

		return (1);

	}


	int captureVideo(Timer* limitTimer) {

		gotoWell();

		// open input from camera
		VideoCapture input(cameranum);

		// wait for camera to open
		int attempt = 1;
		while (!input.isOpened()) {
			if (attempt <= 3) sleep(1);
			else {
				cout << "no camera found!" << endl;
				return -1;
			}
			attempt++;
		}

		input.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_FRAME_WIDTH);
		input.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_FRAME_HEIGHT);
		setCameraSaturation(0);	

		// open output
		VideoWriter output;
		stringstream filename;
		filename << directory << "/day" << getCurrAge() << ".avi";
		Size size = Size(CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT);

		output.open(filename.str().c_str(), VideoWriter::fourcc('M', 'P', '4', '2'),
				input.get(CAP_PROP_FPS), size, true);

		if (!output.isOpened()) {
			cout << "Could not open the output video for write" << endl;
			return -1;
		}

		Timer videoTimer;
		//determine how much time is left before next timelapse scan
		int maxVideoLength = limitTimer->getSeconds();
		cout << "number of second available for video =" << maxVideoLength << endl;
		

		if (maxVideoLength < 30) { videoTimer.startTimer((long)30);}
		else{		
			if (maxVideoLength < VIDEO_DURATION) videoTimer.startTimer((long)maxVideoLength-20); 
			else videoTimer.startTimer((long)VIDEO_DURATION);
		}
		


		do {
			// track length of recording

			Mat frame;
			input >> frame; // get a new frame from camera
			output << frame; // write current frame to output file

		} while (!videoTimer.checkTimer()); // stop recording at 5 minutes

		return 0;
	}


	bool lockOnTarget(void){
		if (targetx < 0 || targety < 0) return(0); //return if no bead found originally
		int dx,dy=1000;
		stringstream cmd("");

		//set camera to color:

		setCameraSaturation(255);

		
		try {

				vector<int> compression_params;
				compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION); //(CV_IMWRITE_PXM_BINARY);
				compression_params.push_back(0);

				VideoCapture cap(cameranum); // open the default camera

				long c = 0;
				while (!cap.isOpened()) { // check if we succeeded
					if (c < 3) sleep(1);
					else {
						cout << "  camera could not be opened" << endl;
						return -1;
					}
					c++;
				} //end while not opened

				cap.set(CV_CAP_PROP_FRAME_WIDTH, 1920);
				cap.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);

				if ((int)cap.get(CV_CAP_PROP_FRAME_WIDTH) != 1920
					|| (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT) != 1080)
					cout << "  cannot adjust video capture properties!" << endl;

	


				int jittercount=0;
		
				while (1 ){		
					//capture frame
			

				
						Mat frame;

						// capture frame
						cap >> frame;
						int i = 0;
						while (frame.empty() && i < 3) {
							sleep(1);
							cap >> frame;
							i++;
						}

						if (i == 3) { // no frame captured
							cout << "  unable to capture lock frame! (expID: " << expID << ")" << endl;
							return 0;
						}

				
			





					//get centroid of pink pixels
					Mat imgHSV;

					  cvtColor(frame, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
					 
					  Mat imgThresholded;

					  inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

					 //morphological opening (remove small objects from the foreground)
					  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
					  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 

					//morphological closing (fill small holes in the foreground)
					  dilate( imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
					  erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

					Moments m = moments(imgThresholded, true);
					int pinkcenterx= m.m10/m.m00;
					int pinkcentery= m.m01/m.m00;


		
					dx= pinkcenterx - targetx;
					dy= pinkcentery - targety;
					if (abs(dx) > 1920 || abs(dy) > 1080) {
						if (jittercount++ > JITTER_WAIT) break;
						continue;  //didn't see a dot try to capture a frame again
					}

					cout << "Well:" << wellname << " targetx:" << targetx << " targety:" << targety << " pinkcenterx:" << pinkcenterx << " pinkcentery:" << pinkcentery << endl; 


					///move the camera to lock in
					if (dx <-10 ) for (int i=0; i < log(abs(dx)); i++ ){sendCommand("W");cout << "w:" << i << "dx:" << dx << endl;}
					if (dx > 10 ) for (int i=0; i < log(abs(dx)); i++ ){sendCommand("S");cout << "s:" << i << "dx:" << dx << endl;}
					if (dy < -10) for (int i=0; i < log(abs(dy)); i++ ){sendCommand("A");cout << "a:" << i << "dy:" << dy << endl;}
					if (dy > 10 ) for (int i=0; i < log(abs(dy)); i++ ){sendCommand("D");cout << "d:" << i << "dy:" << dy << endl;} 
					if (dx <-3 ) sendCommand("W");	
					if (dx > 3 ) sendCommand("S");
					if (dy < -3) sendCommand("A");
					if (dy > 3 ) sendCommand("D");
				/*
					int x1=xval + dx;
					int y1=yval + dy;
					cmd << "M" << x1 << "," << y1;
					sendCommand(cmd.str());
					Timer locktimer;
					locktimer.startTimer((long) TARGET_WAIT_PERIOD);		
					while (!locktime.checkTimer());
				*/

				        if (jittercount++ > JITTER_WAIT || (abs(dx) < ACCEPTABLE_JITTER && abs(dy) < ACCEPTABLE_JITTER)) {
						cout << "final dx,dy:"<< dx << "," << dy << endl;
						if (abs(dx) > ACCEPTABLE_JITTER || abs(dy) > ACCEPTABLE_JITTER){
							string jitterlogname = datapath + string("jitterlog.dat");							
							ofstream jitterlog(jitterlogname.c_str(), fstream::app);
							jitterlog << "Well:" << wellname << ",dx:" << dx << ",dy:" << dy << endl;
							jitterlog.close();
						}//end log bad jitter						
						break;
					}		
				}//end while not locked

		} catch (cv::Exception ex) {

			}//end if caught exception

		//set camera to monochrome
		setCameraSaturation(0);
		
			
		
	}//end lockOnTarget

	int gotoWell(void) {
		stringstream cmd("");
		Timer waittimer;

		cmd << "MX" << xval;
		sendCommand(cmd.str());
		cmd.str("");
		cmd << "MY" << yval;
		sendCommand(cmd.str());
		waittimer.startTimer((long) WELL_WAIT_PERIOD);
		while (!waittimer.checkTimer());
		if (doDotFollow) lockOnTarget();
		return 0; // should change to show success?
	} //end gotoWell

};//end class well


// all wells used in current experimentation
vector <Well*> wells;

// Holds pointers to wells which have been
// requested by experimenter to be video recorded.
Well* monitorSlots[NUM_WELLS];


// Used to sort wells to ensure snake-like pattern for taking pictures
struct rank_sort
{
    inline bool operator() (Well*& struct1, Well*& struct2)
    {
    	int rank1 = struct1->rank;
    	int rank2 = struct2->rank;
        return (rank1 < rank2);
    }
};


void raiseBeep(int pulses){
	for(int j=0; j <pulses; j++){
		for(int i=0; i < 6000; i+=60){
			stringstream beeper;
			beeper << "beep -l 3 -f " << 1000+i << endl;
			system(beeper.str().c_str());
		}
	}//end for j
}//end raise beep


void chordBeep(double octave){
	stringstream beeper;
	cout << (int)((double)370 * octave);
	beeper << "beep  -l 3333 -f " << (int)((double)370 * octave);  //f#
	system(beeper.str().c_str());
	beeper.str("");
	beeper << "beep  -l 3333 -f " << (int)((double)466 * octave);  //a#
	system(beeper.str().c_str());
	beeper.str("");
	beeper << "beep  -l 3333 -f " << (int)((double)554 * octave);  //c#
	system(beeper.str().c_str());
}//end chord beep


// Scan through experiments and capture pictures
void scanExperiments(void) {
	stringstream cmd("");
	int align = 0;  //toggle alignment
	//int count = 0;

	// sort wells
	std::sort(wells.begin(), wells.end(), rank_sort());

	for (vector<Well*>::iterator citer = wells.begin(); citer != wells.end(); citer++) {
		int captured = 0;
		//cout << "count:" << count++ << endl;
		cmd.str("");

		Well* thisWell = *citer;
		if (thisWell->status == WELL_STATE_ACTIVE && thisWell->timelapseActive) {
			thisWell->gotoWell();

			while (captured != 1) {
				captured = thisWell->capture_frame(align);
				
			}
		}
	}
}//end scanExperiments


bool addMonitorJob(Well* well) {
	if (well->monitorSlot == MONITOR_STATE_START) {
		// find open monitor slot starting from the current time
		int i = calcCurrSlot();
		while (i < currMonitorSlot + NUM_WELLS) {
			int thisSlot = i % NUM_WELLS;
			if (monitorSlots[thisSlot] == NULL) {
				monitorSlots[thisSlot] = well;
				well->monitorSlot = thisSlot;
				return true;
			}//end if slot was empty
			i++;
		}//end while slots to scan through
	} else { // slot was already assigned in joblist 
		int slot = well->monitorSlot;
		if (monitorSlots[slot] != NULL)
			cout << "Monitor slot " << slot << " overwritten: two experiments assigned same slot" << endl;
		monitorSlots[slot] = well;
		return true;
	}

	// If this point is reached, all monitor slots must be full
	cout << "No open monitor slot found for experiment " << well->expID << endl;
	return false;
}


void removeMonitorJob(Well* well) {
	monitorSlots[well->monitorSlot] = NULL;
	well->monitorSlot = MONITOR_STATE_OFF;
}

void printCurrExperiments(void){
	for (vector<Well*>::iterator citer = wells.begin(); citer != wells.end(); citer++){
		//(*citer).printSpecs();
		(*citer)->printSpecs();
	}//end for each well
}//end printCurrExperiments


void setVfl2(string setting, string cameradev) {
	stringstream camerasettings;
	camerasettings << "v4l2-ctl -d " << cameradev <<" -c " << setting << endl;
	system(camerasettings.str().c_str());
	cout << setting <<endl;
}


string setupCamera(void) {
	string camfile(datapath + string("camera.config"));
	ifstream inputfile(camfile.c_str());
	string configline;
	string cameradevice;
	getline(inputfile,cameradevice);
	while (getline(inputfile, configline)){
		setVfl2(configline,cameradevice);
	}
	return cameradevice;
}


string fdGetLine(int fd) {
	stringstream ss;
	char c[1];
	while(1){
		if (read(fd,c,1)==0) break;
		ss << c[0];
		if (c[0] == '\n' ) break;
	}
	return ss.str();
}


string fdGetFile(int fd) {
	stringstream ss;
	char c[1];
	while(1){
		if (read(fd,c,1)==0) break;
		ss << c[0];
	}
	return ss.str();
}


int checkJoblistUpdate(void) {

	string filename = datapath + "RRRjoblist.csv";
	cout << "  opening " << filename.c_str() << endl;
	int fd = open (filename.c_str(), O_RDONLY);

	// exit if file not found
	if (fd == -1) {
		cout << "  failed to find joblist\n";  exit(EXIT_FAILURE);
	}

	/* Initialize the flock structure. */
	struct flock lock;
    memset (&lock, 0, sizeof(lock));
	lock.l_type = F_RDLCK;

	/* Place a read lock on the file. Blocks if can't get lock */
	//fcntl (fd, F_SETLKW , &lock);
	//cout <<"locked...checkjoblistupdate\n ";

	//size_t len = 10; //read in first 10 bytes
	 
	string fileheader = fdGetLine(fd);

	lock.l_type = F_UNLCK;
	//fcntl(fd,F_SETLK, &lock);
	close(fd);

	if (fileheader.find("UPDATE") == string::npos)
		return (0);
	else
		return (1);
}



// Load experiments from the joblist
// @param init indicates we've just turned on the robot with true
void syncWithJoblist(bool init = false) {

	string filename = datapath + "RRRjoblist.csv";
	cout << filename << endl;
	int fd = open(filename.c_str(), O_RDONLY);

	if (fd == -1) {
		cout << "  failed to find joblist" << endl;
		exit(EXIT_FAILURE);
	} //exit if file gone

	/* Initialize the flock structure. */
	struct flock lock;
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_RDLCK;

	/* Place a read lock on the file. Blocks if can't get lock */
	// fcntl (fd, F_SETLKW, &lock);

	const string file = fdGetFile(fd);

	stringstream ss(file); //dump joblist into stringstream

	// remove the header
	string fileheader;
	getline(ss, fileheader);
	bool updated = false;
	if (fileheader.find("UPDATE") != string::npos) updated = true;

	stringstream oss; // stream to output as joblist
	oss << string(BLANKUPDATE) << endl; // append blank header

	string experimentline;
	while (getline(ss, experimentline)) {

		Well* thisWell = new Well(experimentline);

		if (thisWell->status == WELL_STATE_STOP) {
			if (init) continue;
			// find experiment and remove
			for (vector<Well*>::iterator citer = wells.begin();
					citer != wells.end(); citer++) {
				if (thisWell->expID == (*citer)->expID) {
					(*citer)->printDescriptionFile();
					if ((*citer)->monitorSlot >= 0)
						removeMonitorJob(*citer);
					wells.erase(citer);
					break;
				}
			}
		}

		else if (thisWell->status == WELL_STATE_START) {

			if (thisWell->monitorSlot != MONITOR_STATE_OFF)
				addMonitorJob(thisWell);

			thisWell->setStatus(WELL_STATE_ACTIVE);
			wells.push_back(thisWell);
		}

		else if (init) { // well is active but uninitialized
			wells.push_back(thisWell);

			if (thisWell->monitorSlot != MONITOR_STATE_OFF)
				addMonitorJob(thisWell);
		}
	}

	// iterate over active wells and update joblist and description files
	for (vector<Well*>::iterator citer = wells.begin(); citer != wells.end(); citer++) {
		oss << (*citer)->printWell();
		(*citer)->printDescriptionFile();
	}

	// unlock joblist for read-only
	lock.l_type = F_UNLCK;
	// fcntl (fd, F_SETLKW, &lock);
	close(fd);

	// Open joblist for write-only and lock
	fd = creat(filename.c_str(), O_WRONLY);
	lock.l_type = F_WRLCK;
	// fcntl (fd, F_SETLKW, &lock);

	if (updated) {
		cout << "\n*********UPDATED JOB LIST*********" << endl;
		cout << oss.str();
		cout << "**********************************\n" << endl;
	}

	FILE *stream;
	if ((stream = fdopen(fd, "w")) == NULL) {
		cout << "  joblist not found" << endl;
		close(fd);
		return;
	}
	fputs(oss.str().c_str(), stream); // output to joblist
	fclose(stream);

	// unlock joblist for write-only
	lock.l_type = F_UNLCK;
	// fcntl (fd, F_SETLKW, &lock);
	close(fd);
}

const string getCurrTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}


void writeToLog(string logline){
	string fn = datapath + "/runlog";
	ofstream ofile(fn.c_str(), std::ofstream::app);
	ofile << getCurrTime() << " " << logline << endl;
	ofile.close();
}


void eraseLog(void){
	string fn = datapath + "/runlog";
	ofstream ofile(fn.c_str());
	ofile << "start log" <<endl;
	ofile.close();
}



int main(int argc, char** argv) {

	if (argc >1) {
		
		for (int i=0; i < argc; i++){
			string args(argv[i]);
			//cout << "args:" << args << endl;
			if (args.find("-df") != string::npos){
				 doDotFollow = true; //do dot following if -df flag is present
				cout << "tracking pink dots" << endl;
				}
			if (args.find("-daemon") != string::npos){
				 
			
				cout << "starting as daemon" << endl;
				cout.rdbuf(logfile.rdbuf()); //redirect std::cout to out.txt!
				daemon(0,1);
				}
		}//end for each argument
	} else cout << "not tracking dots, no daemon" << endl;

	//datapath = "/var/www/wormbot/experiments";

//read in path from /usr/lib/cgi-bin/data_path
	ifstream pathfile("/usr/lib/cgi-bin/data_path");
	getline (pathfile,datapath);
	pathfile.close();
		

 
	// testing
	//syncWithJoblist(true);

	ifstream t("var/root_dir");
	root_dir << t.rdbuf();

	bool skipIntro = true;

	if (!skipIntro) {
		raiseBeep(10);
		chordBeep(1);
		string msg = "cd " + datapath + "; ./play.sh mario.song";
		system(msg.c_str());
	}

	//

	string read;
	string camera;
	string machineZero("ZZ");
	string machineMax("LL");
	string machineRestX("MX150");
	string machineRestY("MY150");
	
	cout << "OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << endl;
	string arduinoport(PORT);
	//eraseLog();

	cout << "********************************************************************" << endl;
	cout << "Kaeberlein Robot controller " << VERSION << endl
		 << "Jason N Pitt and Nolan Strait : Kaeberlein Lab : http://wormbot.org" << endl;
	cout << "********************************************************************" << endl;

	cout << "setting camera parameters \n";
	camera = setupCamera();
	cameranum = boost::lexical_cast<int>(camera[camera.length() - 1]);
	cout << "camera number:" << cameranum << " " << camera << endl;


	int portnum = 0;

	int robotfound = 0;

	

	

	ScanPort: while (!robotfound) {

		ardu.Open(arduinoport);

		ardu.SetBaudRate(SerialStreamBuf::BAUD_9600);
		ardu.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);

		cout << "Set baud rate 9600 and char size 8 bits\n Waiting for Robot to be ready." << endl;
		sendCommand("ZZ");
		sendCommand("LL");
		robotfound=true;

		/* wait for scanner
		Timer startupwait;
		startupwait.startTimer(long(120));
		while (read.find("RR") == string::npos) {
			getline(ardu, read);
			if (startupwait.checkTimer()) {
				portnum = (int) arduinoport[11];
				portnum++;
				arduinoport[11] = portnum;

				cout << "Gave up scanning port: " << arduinoport << endl;
				ardu.Close();
				goto ScanPort;
			} // end if waited too long
		}
		robotfound = 1;
		*/

	} 


	// initialize record slots to null
	for (int i = 0; i < NUM_WELLS; i++) monitorSlots[i] = NULL;

	currMonitorSlot = calcCurrSlot();
	writeToLog("startup currMonitorSlot=");
	writeToLog(boost::lexical_cast<string>(currMonitorSlot));

	
	string msg;
	msg = "Loading experiments from joblist...";
	cout << msg << endl;
	writeToLog(msg);

	syncWithJoblist(true);
	


	// ***ROBOT STATE MACHINE***

	Timer loadTimer((long)0);
	Timer scanTimer((long)0);
	int robotstate = ROBOT_STATE_SCANNING;
	bool updated = false;

	while (true) { // no kill state exists

		switch (robotstate) {


		// Cycle through wells and take pictures
		case ROBOT_STATE_SCANNING:
		   {

			if (calibration_counter++ > CALIBRATE_FREQ) {
			msg = "Calibrating...";
			cout << msg << endl;
			writeToLog(msg);
			sendCommand(String("CC")); //run axis calibration in firmware
			calibration_counter=0; //reset the counter


			}//end if need to calibrate

			

			msg = "Scanning...";
			cout << msg << endl;
			writeToLog(msg);

			raiseBeep(3);

			scanTimer.startTimer((long) SCAN_PERIOD);
			

			// iterate over experiments
			scanExperiments();

			// zero the wormbot
			sendCommand(machineZero);

			stringstream pdebg;
			pdebg << "precondition currMonitorSlot=" << currMonitorSlot << " calcurrslot()=" << calcCurrSlot() << " \n";
			writeToLog(pdebg.str());

			
				
			// check monitor slot
				//currMonitorSlot = calcCurrSlot();
				Well* currWell = monitorSlots[currMonitorSlot];
				stringstream debg;
				debg <<	"True : currMonitorSlot=" << currMonitorSlot << " calcurrslot()=" << calcCurrSlot() << " \n";						
				writeToLog(debg.str());
				if (currWell != NULL) {
					cout << "  capturing video for monitor slot " << currMonitorSlot
						 << " (expID: " << currWell->expID << ")" << endl;
					currWell->captureVideo(&scanTimer);

					// start video analysis
					/*
					stringstream cmd;
					cmd << "sudo /usr/lib/cgi-bin/wormtracker " // location of wormtracker
						<< currWell->directory << "/day" << currWell->getCurrAge()
						<< ".avi" << " " // video file
						<< currWell->directory // dir to put analysis data
						<< " &"; // run process in background
					system(cmd.str().c_str());*/

				} else {
					cout << "  skip video for monitor slot " << currMonitorSlot
						 << " (no well found)" << endl;
				}
				if (++currMonitorSlot >= NUM_WELLS) currMonitorSlot=0; 
				debg <<	" post capture currMonitorSlot=" << currMonitorSlot << " \n";
			

			// zero the plotter
			sendCommand(machineZero);

			robotstate = ROBOT_STATE_WAIT;

			break; //end state scanning
		  }


		// robot is in between scans
		case ROBOT_STATE_WAIT:

			

			msg = "Syncing with joblist...";
			cout << msg << endl;
			writeToLog(msg);

			updated = checkJoblistUpdate();

			syncWithJoblist();

			// Check joblist for updates
			if (updated) {
				
				robotstate = ROBOT_STATE_LOAD;
			} else {
				
				cout << "Waiting..." << endl;
				while (!scanTimer.checkTimer()) {}
				robotstate = ROBOT_STATE_SCANNING;
			}

			break; //end state wait


		// Wait for experimenter to load/unload plates
		case ROBOT_STATE_LOAD:

			

			msg = "Preparing to load/unload plates...";
			cout << msg << endl;
			writeToLog(msg);

			sendCommand(machineMax);

			msg = "Please load/unload plates";
			cout << msg << endl;

			loadTimer.startTimer((long) LOAD_WAIT_PERIOD);
			while (!loadTimer.checkTimer()) {}

			for (int j = 1; j < 4; j++) chordBeep(double(j));

			msg = "Ending load period... Keep clear of machine";
			cout << msg << endl;

			raiseBeep(10);

			// sort wells
			std::sort(wells.begin(), wells.end(), rank_sort());

			sendCommand(machineZero);

			robotstate = ROBOT_STATE_SCANNING;

			currMonitorSlot = calcCurrSlot();

			cout << "Waiting..." << endl;
			while (!scanTimer.checkTimer()) {}

			break; //end state load

		}

	} //end while robot not killed

	cout << "shutdown" << endl;

	// the camera will be deinitialized automatically in VideoCapture destructor
	return 0;
}

