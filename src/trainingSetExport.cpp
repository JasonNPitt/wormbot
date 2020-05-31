//============================================================================
// Name        : trainingSetExport.cpp
// Author      : Jason N Pitt
// Version     :
// Copyright   : MIT LICENSE
// Description : command line utility to dump a subset of image files for neural network training
//============================================================================

#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/stat.h>
#include <utime.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <random>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>   


#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>

#define BADFRAMES 20


using namespace std;
using namespace cv;
namespace fs = boost::filesystem;



string directory("");
stringstream root_dir("/var/www/html/wormbot/");

string currfilename;



void readDirectory(string fulldirectory, string outputdir, long exp, int framesperexp, long randLow, long randHigh, long segment);




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




void scanRobotDir(string robotdir,string outputdir, int framesperexp, int numexps, int maxframes,long randLow, long randHigh, long expLow, long expHigh, long segment, bool processall){
	

	
	

	vector<long> expIDs;
	
	stringstream makedir;
	
	//make the output directory
	cout << "making directory:" << outputdir << endl;
	makedir << "mkdir " << outputdir;
	system(makedir.str().c_str());
	

	if (!processall){
		std::default_random_engine generator;
		std::uniform_int_distribution<int> distribution(expLow,expHigh);
	
		

		for (long i=0; i < 100000; i++){
			stringstream ss;
			long anexp = distribution(generator);
			ss << robotdir << "/" << anexp;			
			if (fs::exists(ss.str().c_str())) expIDs.push_back(anexp); else {cout << "DIR NOT FOUND\n";i--;}
			cout << "scanexpID:" << anexp << endl; 
	
		}//end for each random expID
	} else { //if processall

		for (long i=0; i < 100000; i++){
			stringstream ss;
			ss << robotdir << "/" << i;
			if (fs::exists(ss.str().c_str())) expIDs.push_back(i);
		}//for each experiment	

	}


	for (vector<long>::iterator it = expIDs.begin(); it != expIDs.end(); it++){
    		
		stringstream ss;
		ss << robotdir << (*it) << "/";
		cout << "reading expID:" << ss.str() << endl;
		readDirectory(ss.str(),outputdir,(*it),framesperexp,randLow,randHigh,segment);

	}//end for each ID in the directory


}//end scanrobotdir

long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;
	cout << "setframetime:" << time << endl;
	return (time);

}//end getFileCreationTime

long getLifespan(string filename, string fullpath){
	stringstream s;
	s << fullpath << "description.txt";
	ifstream ifile(s.str().c_str());
	string inputline;
	long expstarttime=0;
	int daysold=0;

	long frametime;

	frametime = getFileCreationTime(filename);

	int i=0;
	while (getline(ifile,inputline)){
		if (i==5){
			expstarttime=atol(inputline.c_str());
		}//end process experiment start time
		if (i==11){
            daysold = atoi(inputline.c_str());
            cout << "daysold:" << daysold << endl;
            cout << "expstarttime:" << expstarttime << endl;
            cout << "frametime:" << frametime << endl;
             return (frametime-(expstarttime -(86400 * daysold)));

		}//end process starting age
		i++;
	}


}//end getexperimentstarttime

void readDirectory(string fulldirectory, string outputdir, long exp, int framesperexp, long randLow, long randHigh,long segment){

	glob_t glob_result;
	glob_t aligned_result;
	vector<string> filelist;
	vector<string> alignedlist;
	long numframes =0;
	long numaligned=0;
	
	default_random_engine generator;
	generator.seed();
	uniform_int_distribution<int> distribution(randLow,randHigh);


	stringstream globpattern;
	globpattern  << fulldirectory.c_str() << string("aligned*.png");
     //cout << "directory:" << directory << "<br>" << endl;
	cout <<"globpat:" << globpattern.str() << endl;

	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&glob_result);
	
	for (unsigned int i=0; i < glob_result.gl_pathc; i++){
		filelist.push_back(string(glob_result.gl_pathv[i]));
		numframes++;
	}//end for each glob
	

	if (numframes < randLow || numframes ==0) return;
	cout << "number of frames in exp: " << numframes << endl;

	if (segment==0){

		for (int i=0; i < framesperexp; i++){
		
			//system
			stringstream ss;
			int bad =0;
		
			//get random filename
			long thefile = rand() % (randHigh-randLow) + randLow;

			//get file age
			long frameage = getLifespan(filelist[thefile],fulldirectory);
			cout << "thefile = " << thefile << endl;
			while (thefile < randLow || thefile >= randHigh || thefile >= numframes) {
				thefile = rand() % (randHigh-randLow) + randLow; 
				cout << "out of range!\n";
				if (bad++ > BADFRAMES) break;
			} //while not in range find random nums
			if (bad++ > BADFRAMES){ break; }
			else { 
				ss << "cp " << filelist[thefile] << " " << outputdir << "exp" << exp << "_frame" << thefile << "_age" << frameage << ".png" << endl;
				cout << ss.str();		
				system(ss.str().c_str());
			}
		}//end foreach frame

	}//end if segment ==0
	else { //segment > 0

		//system
		
		int bad =0;
		if (segment >= numframes) return; 
		//get segment size
		long segsize = numframes/segment;

		cout << "segment size=" << segsize << endl;

		//get random startframe
		long startfile = rand() % (segsize);

		

		

		for (int i =0; i < segment; i ++) {
			stringstream ss;

		        long thefile = (segsize * i ) + startfile;

			//get frame age
			long frameage = getLifespan(filelist[thefile],fulldirectory);
			cout << "thefile = " << thefile << endl;

			ss << "cp " << filelist[thefile] << " " << outputdir << "exp" << exp << "_frame" << thefile << "_age" << frameage << ".png" << endl;
			cout << ss.str();		
			system(ss.str().c_str());
		
			

		}//end for each segment


	}//end if segments specified

	stringstream sss; 
	sss << "cp " << fulldirectory << "description.txt " << outputdir << "exp" << exp << "_description.txt" << endl;
	system(sss.str().c_str());
	
	stringstream ssss; 
	ssss << "cp " << fulldirectory << "wormlist.csv " << outputdir << "exp" << exp << "_wormlist.csv" << endl;
	system(ssss.str().c_str());


}//end readDirectory



int main(int argc, char **argv) {
	ifstream t("var/root_dir");
	root_dir << t.rdbuf();

	
	int setExp=-1;

	
	string standardfile;




	cout << "argc=" << argc << endl;


	//command line variables
	stringstream apath("/wormbot"); //path to experiments
	stringstream outputpath("testset"); //path to output set
	int fperExp=1; //frames per experiment default 1
	int numexps=0;
	long randLow=0; //starting frame
	long randHigh=2000; //end frame
	long expLow=1; 
	long expHigh=1;
	int maxframes = 100; //total number of frames to export
	long segment = 0; //number of temporal segments to sample evenly, zero leads to random sampling from entire experiment
	bool all = false; //flag to parse each experiment in the target directory

	
	


	 for (int i=1; i < argc; i+=2){
	char sw = argv[i][1];
	cout << "arg i=" << sw << endl;
	
	switch (sw) {
		case 'd':
		case 'D':
		case 'p':
		case 'P':
			apath.str("");
			apath << argv[i+1];
		break;

		case 'n':
		case 'N':
			fperExp = atoi(argv[i+1]);
		break;

		case 'x':
		case 'X':
			numexps = atoi(argv[i+1]);
		break;

		case 'm':
		case 'M':
			maxframes = atoi(argv[i+1]);
		break;

		case 'l':
		case 'L':
			randLow = atol(argv[i+1]);
		break;

		case 'h':
		case 'H':
			randHigh = atol(argv[i+1]);
		break;

		case 'b':
		case 'B':
			expLow = atol(argv[i+1]);
		break;

		case 'e':
		case 'E':
			expHigh = atol(argv[i+1]);
		break;

		case 'o':
		case 'O':
			outputpath.str("");
			outputpath << argv[i+1];
		break;

		case 's':
		case 'S':
			segment = atol(argv[i+1]);
		break;

		case 'a':
		case 'A':
			
			if (atoi(argv[i+1])) all=true;
		break;

		

			

	}//end argument swtch


    }// end for each argument

	
	    scanRobotDir(apath.str(),outputpath.str(),fperExp,numexps,maxframes,randLow,randHigh, expLow, expHigh, segment, all);

    exit(EXIT_SUCCESS);

	return 0;
}
