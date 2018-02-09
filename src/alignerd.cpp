//============================================================================
// Name        : imagealigner.cpp
// Author      : Jason N Pitt
// Version     :
// Copyright   : MIT LICENSE
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>
#include <stdlib.h>
#include <inttypes.h>
#include <glob.h>
#include <vector>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>




#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <cstdio>
#include <errno.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>
#include <stdio.h>



#include <opencv2/opencv.hpp>
#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>

#define DELAY_TIME 1800



using namespace std;

using namespace cv;
using namespace boost;

//globals

string directory("");

string currfilename;
ofstream logfile("/disk1/robot_data/alignerd.log", ofstream::app);
streambuf *coutbuf = std::cout.rdbuf(); //save old buf




void readDirectory(string fulldirectory);


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
	}//end consntructor

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
		//cout << "stmstimer" << delay << "." << msdelay << "\n";
	}//end start timer
	void startTimer(double time){

			gettimeofday(&start, NULL);
			if (ms){
				msdelay=time;
			}else{
				delay=time;
			}
			//cout << "stmstimer" << msdelay << "\n";
		}//end start timer
	double getTimeElapsed(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		stringstream ss;
		ss << currtime.tv_sec;

		string seconds(ss.str());
		//cout << "currsec" << seconds << "\t";
		ss.str("");
		ss.clear();
		ss << (double)currtime.tv_usec/(double)1000000;
		string micros(ss.str());

		micros.erase(0,1);
		//cout << "currmicros" << micros << "\t";
		seconds.append(micros);
		//cout << "appended" << seconds << "\t";
		double curr = atof(seconds.c_str());
		//cout << "currtime" << curr << "\t";
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
				//cout << "starttime" << scurr << "\n";
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
			//cout << getTimeElapsed() << "\n";
			if (getTimeElapsed() >= msdelay)return true; else return false;
			//cout << "check" << (currtime.tv_sec-start.tv_sec) << "." << (currtime.tv_usec-start.tv_usec) << "\n";
			//if ((currtime.tv_sec-start.tv_sec) >= delay && (currtime.tv_usec-start.tv_usec) >= msdelay) return true; else return false;
		}else{
			if (currtime.tv_sec-start.tv_sec >= delay) return true; else return false;
		}
	}//end checkTimer

};



string swapTimes(string filename, string newfilename){
	stringstream outtime;

	struct stat attr;
	struct utimbuf thetimev;
	stat(filename.c_str(),&attr);

	//cout << "setframetime:" << time << endl;
	thetimev.actime=attr.st_mtim.tv_sec;
	thetimev.modtime=attr.st_mtim.tv_sec;
	//TIMESPEC_TO_TIMEVAL(&thetimev, &attr.st_mtim);
	outtime << ctime(&thetimev.modtime);

	utime(newfilename.c_str(), &thetimev);

	return (outtime.str());
}


long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;
	cout << "setframetime:" << time << endl;
	return (time);

}


void alignDirectory(string dirname, vector<string> filelist, int numframes, int numaligned){
	vector<int> compression_params;
	 compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);//(CV_IMWRITE_PXM_BINARY);
	 compression_params.push_back(0);


     //save image0
	 cout << "\n Directory: " << dirname <<endl;
	 if (numaligned ==0){
		 try{
		 Mat frame=imread(filelist[0]);
		 Mat frame_gray;
		 cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray
		 replace_all(filelist[0],"frame","aligned");
		 imwrite(filelist[0], frame_gray, compression_params );
		 numaligned=1;
		 cout <<"Set frame 0." <<endl;
		 } catch (cv::Exception ex){
			cout << "first frame was bad! try copying a later frame to frame 0...I don't feel like writing the code to deal with this rare possibility" << endl;
		 }//end caught exception trying to load

	 }//end if numaligned is zero


	for (int i=numaligned; i < numframes; i++){
					cout << "." << endl; //keep webserver from timing out
					Mat frame;
					Mat frame_gray;
			        Mat im2_aligned;
			        Mat im1_gray;
			        Mat im1;

					try{
		            frame=imread(filelist[i]);

		            cout << "read filelist ";
					cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray

					im1= imread(filelist[0]);//filelist[i-1]);//try warping all to original

					cvtColor(im1, im1_gray, CV_BGR2GRAY);
					cout << "convert color ";
					} catch (cv::Exception ex){
						//frame is bad...overwrite it with the previous frame
							    				 stringstream outss;
							    				 cout << "caught exception:" << ex.code << endl;
							    				 outss << "cp " << filelist[i-1];
							    				 replace_all(filelist[i],"frame","aligned");
							    				 outss << " " << filelist[i] << endl;
							    				 system(outss.str().c_str());
							    				// imwrite(filelist[i], frame_gray, compression_params );
							    				 continue;

					}//end frame was bad
					// Define the motion model
					const int warp_mode = MOTION_TRANSLATION;

					// Set a 2x3 warp matrix
					Mat warp_matrix;
	    			warp_matrix = Mat::eye(2, 3, CV_32F);
	    			 // Specify the number of iterations.
	    			int number_of_iterations = 500;

	    			 // Specify the threshold of the increment
	    			 // in the correlation coefficient between two iterations
	    			 double termination_eps = 1e-8;
	    			 cout << "set warp matrix ";

	    			 // Define termination criteria
	    			 TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, number_of_iterations, termination_eps);

	    			 // Run the ECC algorithm. The results are stored in warp_matrix.
	    			 try{
	    				 findTransformECC(im1_gray,frame_gray,warp_matrix,warp_mode,criteria);
	    			 }
	    			 catch(cv::Exception e){
	    				 //frame is bad...overwrite it with the previous frame
	    				 stringstream outss;
	    				 cout << "caught exception:" << e.code << endl;
	    				 outss << "cp " << filelist[i-1];
	    				 replace_all(filelist[i],"frame","aligned");
	    				 outss << " " << filelist[i] << endl;
	    				 system(outss.str().c_str());
	    				// imwrite(filelist[i], frame_gray, compression_params );
	    				 continue;
	    			 }
	    			 cout << "ECC ";
	    			 warpAffine(frame, im2_aligned, warp_matrix, im1.size(), INTER_LINEAR + WARP_INVERSE_MAP);
	    			 cout << "warp ";
	    			 Mat im2_aligned_gray;
	    			 cvtColor(im2_aligned, im2_aligned_gray, CV_BGR2GRAY);
	    			 cout << "convert color 2";
	    			 string oldname = filelist[i];
	    			 replace_all(filelist[i],"frame","aligned");
	    			 cout << "writing: "<<filelist[i] << endl;
	    			 imwrite(filelist[i], im2_aligned_gray, compression_params );
	    			 cout << "writefile  " << endl;
	    			 cout << "timestamp" <<  swapTimes(oldname,filelist[i]) << endl;

	    			 //add timestamps to the files that are already aligned
	    			 if (i > 2){
	    				Mat towrite=imread(filelist[i-2]);
	    				 //write timestamps
	    				stringstream textadd;
	    				struct utimbuf atime;
	    				atime.modtime = getFileCreationTime(filelist[i-2]);
	    				atime.actime = atime.modtime;

	    				stringstream filetime;


	    				filetime << ctime(&atime.modtime);
	    				string formattedtime(filetime.str());
	    				string fileframenumber;
	    				replace_all(formattedtime,":",".");
                        //cout << "formatted time" << formattedtime << endl;
                       // cout << " filelist-2:" << filelist[i-2];
                        int spot = filelist[i-2].find("aligned");
                        //cout << " spot:" << spot;
	    				fileframenumber = filelist[i-2].substr(spot+7);

	    				replace_all(fileframenumber,".png","");
	    				//cout <<fileframenumber;

	    				textadd << "/disk1/ffmpeg -hide_banner -y -i " << filelist[i-2] <<
	    						" -vf drawtext=\"box=1:fontfile=/usr/share/fonts/truetype/freefont/FreeSansBold.ttf: text='" <<
	    						fileframenumber << " " <<
	    						 formattedtime << "': fontcolor=black: fontsize=24: x=5:y=(h-th)" <<
	    						"\" " << filelist[i-2] << " 2> /dev/null" << endl;
	    				//ofstream testfile("drawtext.txt");
	    				//testfile << textadd.str();
	    				//testfile.close();
	    				system(textadd.str().c_str());
	    				//update the timestamp on the file
	    				utime(filelist[i-2].c_str(), &atime);

	    			 }//end if I greater than 2
	}//end for each frame
}//end alignDirectory



void scanRobotDir(string robotdir){
	stringstream ss;

	//read in the current expID
	ss << robotdir << "currexpid.dat";
	string curridfilename = ss.str();
	ifstream ifile(curridfilename.c_str());
	string currstring;
	getline(ifile,currstring);
	long totalexperiments = atol(currstring.c_str());

	for (int i=0; i <= totalexperiments; i++){
		ss.str("");
		ss << robotdir << i << "/";
		cout << "reading expID:" << ss.str() << endl;
		readDirectory(ss.str());

	}//end for each ID in the directory


}//end scanrobotdir

void readDirectory(string fulldirectory){
	glob_t glob_result;
	glob_t aligned_result;
	vector<string> filelist;
	vector<string> alignedlist;
	long numframes =0;
	long numaligned=0;

	stringstream globpattern;
	globpattern  << fulldirectory.c_str() << string("frame*.png");
     //cout << "directory:" << directory << "<br>" << endl;
	cout <<"globpat:" << globpattern.str() << endl;

	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&glob_result);

	for (unsigned int i=0; i < glob_result.gl_pathc; i++){
		filelist.push_back(string(glob_result.gl_pathv[i]));
		numframes++;
	}//end for each glob

	//scan for aligned files
	globpattern.str("");
	globpattern  << fulldirectory.c_str() << string("aligned*.png");
	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&aligned_result);

	for (unsigned int i=0; i < aligned_result.gl_pathc; i++){
            filelist[i]=aligned_result.gl_pathv[i];
			numaligned++;
		}//end for each glob





     if (numaligned != numframes){
    	 alignDirectory(fulldirectory,filelist,numframes,numaligned);
     }


}//end readDirectory



int main(int argc, char **argv) {

	cout.rdbuf(logfile.rdbuf()); //redirect std::cout to out.txt!

	// setup a timer and get the robot data directory from the commandline
	Timer mytimer;
	string standardfile;
	if (argc < 1) {
		standardfile = "/disk1/robot_data";

	} else {
		standardfile = argv[1];
	}

	daemon(0,1);

	while (1){

		mytimer.startTimer((long)DELAY_TIME);

	    scanRobotDir(standardfile);

	    while(!mytimer.checkTimer());  ///wait


	}//go forever

    exit(EXIT_SUCCESS);

	return 0;
}
