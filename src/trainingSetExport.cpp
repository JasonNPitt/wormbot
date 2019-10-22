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

#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>

#define BADFRAMES 20


using namespace std;
using namespace cv;
using namespace boost;



string directory("");
stringstream root_dir("/var/www/html/wormbot/");

string currfilename;



void readDirectory(string fulldirectory, string outputdir, long exp, int framesperexp, long randLow, long randHigh);




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



void scanRobotDir(string robotdir,string outputdir, int framesperexp, int numexps, int maxframes,long randLow, long randHigh, long expLow, long expHigh){
	stringstream ss;

	//read in the current expID
	ss << robotdir << "currexpid.dat";
	string curridfilename = ss.str();
	ifstream ifile(curridfilename.c_str());
	string currstring;
	getline(ifile,currstring);
	long totalexperiments = atol(currstring.c_str());

	vector<long> expIDs;
	
	stringstream makedir;
	
	//make the output directory
	cout << "making directory:" << outputdir << endl;
	makedir << "mkdir " << outputdir;
	system(makedir.str().c_str());
	
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(expLow,expHigh);
	
	if (expHigh > totalexperiments) return;
	if (expLow > totalexperiments) return;

	for (int i=0; i < numexps; i++){
		long anexp = distribution(generator);
		expIDs.push_back(anexp);
		cout << "scanexpID:" << anexp << endl; 
	
	}//end for each random expID


	for (vector<long>::iterator it = expIDs.begin(); it != expIDs.end(); it++){
    		
		ss.str("");
		ss << robotdir << (*it) << "/";
		cout << "reading expID:" << ss.str() << endl;
		readDirectory(ss.str(),outputdir,(*it),framesperexp,randLow,randHigh);

	}//end for each ID in the directory


}//end scanrobotdir

void readDirectory(string fulldirectory, string outputdir, long exp, int framesperexp, long randLow, long randHigh){

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
	globpattern  << fulldirectory.c_str() << string("frame*.png");
     //cout << "directory:" << directory << "<br>" << endl;
	cout <<"globpat:" << globpattern.str() << endl;

	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&glob_result);
	
	for (unsigned int i=0; i < glob_result.gl_pathc; i++){
		filelist.push_back(string(glob_result.gl_pathv[i]));
		numframes++;
	}//end for each glob
	

	if (numframes < randLow || numframes ==0) return;
	cout << "number of frames in exp: " << numframes << endl;

	for (int i=0; i < framesperexp; i++){
		
		//system
		stringstream ss;
		int bad =0;
		
		//get random filename
		long thefile = rand() % (randHigh-randLow) + randLow;
		cout << "thefile = " << thefile << endl;
		while (thefile < randLow || thefile >= randHigh || thefile >= numframes) {
			thefile = rand() % (randHigh-randLow) + randLow; 
			cout << "out of range!\n";
			if (bad++ > BADFRAMES) break;
		} //while not in range find random nums
		if (bad++ > BADFRAMES){ break; }
		else { 
			ss << "cp " << filelist[thefile] << " " << outputdir << "exp" << exp << "_random" << i << ".png" << endl;
			cout << ss.str();		
			system(ss.str().c_str());
		}
	}//end foreach frame

	stringstream sss; 
	sss << "cp " << fulldirectory << "description.txt " << outputdir << "exp" << exp << "_description.txt" << endl;
	system(sss.str().c_str());


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
	long expLow=0; 
	long expHigh=1;
	int maxframes = 100; //total number of frames to export
	
	


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

		

		

			

	}//end argument swtch


    }// end for each argument

	
	    scanRobotDir(apath.str(),outputpath.str(),fperExp,numexps,maxframes,randLow,randHigh, expLow, expHigh);

    exit(EXIT_SUCCESS);

	return 0;
}
