//============================================================================
// Name        : experimentbrowser.cpp
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
#include <vector>
#include <fcntl.h>
//#include <linux/kd.h>
#include <sys/ioctl.h>
//#include <boost/algorithm/string.hpp> // include Boost, a C++ library
//#include <boost/lexical_cast.hpp>

#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <cstdio>

#include <errno.h>
//#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>
#include <stdio.h>

//#include "cgic.h"
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>

#define WELL_STATE_START 2
#define WELL_STATE_ACTIVE 1
#define WELL_STATE_STOP 0


using namespace std;
using namespace cgicc;
//using namespace boost;

//globals
Cgicc cgi;
string datapath;

int main(int argc, char **argv) {
   try {

	  ifstream readpath("robot_data_path");
	  readpath >> datapath;

	  // get current experiment ID
	  long expID;
	  string filename;
	  filename = datapath + string("currexpid.dat");
	  ifstream ifile(filename.c_str());
	  ifile >> expID;

      // Set up the HTML document
      cout << HTTPHTMLHeader() << endl;
      cout << html() << endl << endl;
      cout << head(title("Kaeberlein Lab Worm Lifespan Experiment Browser")) << endl << endl;
      cout << "<style>table, th, td {border: 1px solid black;}</style>" << endl << endl;
      cout << body() << endl;
      //cout << img().set("src","http://kaeberleinlab.org/images/kaeberlein-lab-logo-2.png") << endl;
      cout << br() <<endl;

      cout << "<h1>Experiment Browser</h1>\n"
    	   << "<table>\n"
		   << "  <tr>\n"
		   << "    <th>Title</th>\n"
		   << "    <th>Exp ID</th>\n"
		   << "    <th>Description</th>\n"
		   << "    <th>Worms Scored</th>\n"
		   << "  </tr>" << endl;

      // create HTML table entries for each experiment
      for (int i = 0; i <= expID; i++) {

    	  // create table element for experiment ID
    	  cout << "  <tr>\n"
    	       << "    <td><a href=\"/cgi-bin/imagealigner?loadedexpID=" << i << "\" target=\"_blank\">" << i << "</a></td>" << endl;

    	  // get title and description of this experiment from description.txt
    	  // ***REPLACE WITH JSON***
    	  stringstream dfile;
    	  dfile << datapath << i << "/description.txt";
    	  ifstream readdesc(dfile.str().c_str());
    	  string aline;
    	  string title;
    	  string description;
    	  getline(readdesc,aline); //discard first line
    	  getline(readdesc,title); //grab title
    	  getline(readdesc,aline); //discard third line
    	  getline(readdesc,aline); //discard fourth line
    	  getline(readdesc,description); //grab descr
    	  readdesc.close();

    	  // create table elements for title and description of this experiment
    	  cout << "    <td><a href=\"/cgi-bin/imagealigner?loadedexpID=" << i << "\" target=\"_blank\">" << title << "</a></td>" << endl;
    	  cout << "    <td><a href=\"/cgi-bin/imagealigner?loadedexpID=" << i << "\" target=\"_blank\">" << description << "</a></td>" << endl;

    	  // get a count of worms in this experiment
    	  stringstream wfile;
    	  wfile << datapath << i << "/wormlist.csv";
    	  ifstream wfilestream(wfile.str().c_str());
    	  int num_worms = 0;
    	  string aworm;
    	  while (getline(wfilestream,aworm)) ++num_worms;
    	  wfilestream.close();

    	  // create table element for worm count
    	  cout << "    <td><a href=\"/cgi-bin/imagealigner?loadedexpID=" << i << "\" target=\"_blank\">" << num_worms << "</a></td>" << endl;

    	  // create buttons for deleting and downloading this experiment's data
    	  cout << "    <td><button type=\"button\" onclick=\"deleteExp()\"><img src=\"icon_delete.png\"</button></td>" << endl;
    	  cout << "    <td><button type=\"button\" onclick=\"downloadExp()\"><img src=\"icon_download.png\"</button></td>" << endl;

    	  // end this table row
    	  cout << "  </tr>" << endl;
      }

      // end table
      cout << "</table>\n" << endl;

      // add script functions for deleting and downloading experimental data
      cout << "<script>\n"
    	   << "  function deleteExp() {\n"
		   << "    confirm(\"Delete all data for this experiment?\");\n"
		   << "  }\n"
		   << "  function downloadExp() {\n"
		   << "    confirm(\"Download all data for this experiment?\");\n"
		   << "  }\n"
		   << "</script>" << endl;

      cout << br() <<  endl;
     // cout << img().set("src", "http://undergradrobot.dynu.com/Bender.png").set("width","100") << endl;
    //  cout << form().set("action", "http://undergradrobot.dynu.com/cgi-bin/robotscheduler").set("method", "POST") << endl;

      // Close the HTML document
      cout << body() << endl << endl << html() << endl;

   } catch (exception& e) {
      // handle any errors - omitted for brevity
   }
}
