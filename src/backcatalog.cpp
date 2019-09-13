//============================================================================
// Name        : backcatalog.cpp
// Author      : Jason N Pitt 
// Version     :
// Copyright   : MIT LICENSE
// Description : Builds a table of contents for a backup drive
//============================================================================


#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/stat.h>
#include <utime.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>


#define MAXID 20000

using namespace std;
using namespace cgicc;


//globals

string datapath;








int main(int argc, char **argv) {
  
	  //get the wormbot path
	   datapath = argv[1];

	ofstream ofile("catalog.csv");


 // create table element for experiment ID
    	  ofile << "ExpID, Title, Description, Strain, number frames, Worm count"  << endl;

      // create HTML table entries for each experiment
      for (int i = 0; i <= MAXID; i++) {

	  stringstream dfile;
    	  dfile << datapath << i << "/description.txt";
    	  ifstream readdesc(dfile.str().c_str());
	  if (!readdesc.good()) {
		 continue;
		 //if the experiment is empty skip
	  }

    	 

    	  // get title and description of this experiment from description.txt
    	  // ***REPLACE WITH JSON***
    	  
    	  string aline;
    	  string title;
    	  string description;
	  string expid;
	  string strain,numframes;

    	  getline(readdesc,aline); //discard first line
    	  getline(readdesc,title); //grab title
    	  getline(readdesc,aline); //discard third line
    	  getline(readdesc,aline); //discard fourth line
    	  getline(readdesc,description); //grab descr
	  getline(readdesc,aline); //discard sixth line
	  getline(readdesc,numframes); //grab numframes
	  getline(readdesc,strain); //grab numframes
    	  readdesc.close();

    	  // create table elements for title and description of this experiment
    	  ofile << i << "," << title << "," << description << "," << strain << "," << numframes << "," ;

    	  // get a count of worms in this experiment
    	  stringstream wfile;
    	  wfile << datapath << i << "/wormlist.csv";
    	  ifstream wfilestream(wfile.str().c_str());
    	  int num_worms = 0;
    	  string aworm;
    	  while (getline(wfilestream,aworm)) ++num_worms;
    	  wfilestream.close();

    	  // create table element for worm count
	  ofile << num_worms << endl;
	}//end for each expid

   
}//end main

