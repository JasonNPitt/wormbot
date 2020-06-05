//============================================================================
// Name        : experimentbrowser.cpp
// Author      : Jason N Pitt and Nolan Strait
// Version     :
// Copyright   : MIT LICENSE
// Description : Hello World in C++, Ansi-style
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


#define WELL_STATE_START 2
#define WELL_STATE_ACTIVE 1
#define WELL_STATE_STOP 0


using namespace std;
using namespace cgicc;


//globals
Cgicc cgi;
string datapath;

const std::string css = 
"table {font-family: \"Lato\", sans-serif;border-collapse: collapse;width: 88%;margin:1% 6%}\n"
"td,th {border: 1px solid #ddd;padding: 8px;}\n"
"tr:nth-child(even) {background-color: #f2f2f2;}\n"
"tr:hover {background-color: #ddd;}\n"
"body {margin: 0;font-family: Arial, Helvetica, sans-serif;}\n"
".topnav {overflow: hidden;background-color: #333;display: flex;position: fixed;width: 100%;}\n"
".topnav a { float: left;color: #f2f2f2;text-align: center;padding: 14px 16px;text-decoration: none;font-size: 17px;}\n"
".topnav a:hover {background-color: #ddd;color: black;}\n"
".topnav a.active {background-color: #4CAF50;color: white;}\n";


string checkForArchive(int id) {
	
	stringstream archivefilename;
	archivefilename << datapath << id << ".tar.gz";
	
	ifstream checkArch(archivefilename.str().c_str());
	if (checkArch.good()){
		stringstream linkname;
		linkname << "/wormbot/" << id << ".tar.gz";
		return linkname.str();
	} else {
		return string("");
	}



}//end checkforarchive



int main(int argc, char **argv) {
  
	  //get the wormbot path
	  ifstream readpath("data_path");
	  readpath >> datapath;

//head of HTML File

	cout << HTTPHTMLHeader() << endl;
      cout << html() << endl << endl;
      cout << head(title("WormBot Experiment Browser")) << endl << endl;
      cout <<   "<script type=\"text/javascript\" src=\"//code.jquery.com/jquery-1.9.1.js\"></script>" << endl;	
      cout << "<style>" << css << "</style>" << endl << endl;
      cout << body() << endl;
      cout << br() <<endl;

 try {
	//read in the delete and archive commands and act on them
	if (!cgi("delete").empty()){	
		int expToDelete = atoi(cgi("delete").c_str());
		stringstream deleter;
		deleter << "/wormbot/cgi-bin/deleteexperiment " << expToDelete;
		cout << "<H1>Deleting...</H1>\n";	
		//if archive exists remove it		
		stringstream delarch;	
		delarch << datapath << expToDelete << ".tar.gz";		
		remove(delarch.str().c_str());
		system(deleter.str().c_str());
	}
	if (!cgi("backup").empty()){	
		int expToBackup = atoi(cgi("backup").c_str());
		stringstream backer;
		backer << "/wormbot/cgi-bin/backupexperiment " << expToBackup;	
		cout << "<H1>Compressing...</H1>\n";
		system(backer.str().c_str());
	}
} catch (exception& e) {
      // handle any errors - omitted for brevity
}

	  // get current experiment ID
	  long expID;
	  string filename;
	  filename = datapath + string("currexpid.dat");
	  ifstream ifile(filename.c_str());
	  ifile >> expID;

      // Set up the HTML document
      

      cout << "<div class=\"topnav\">"
      	   << "<a href=\"./Scheduler.html\">Scheduler</a>"
           << "<a href=\"#\">Retrograde</a>"
           << "<a href=\".\">Experiment Browser</a>"
           << "<a href=\"#\">Status</a>"
           << "</div>"




           << "<div style=\"padding-top: 3%; padding-left:1%\"><h1>Experiment Browser</h1><div>\n"
    	   << "<table>\n"
		   << "  <tr>\n"
		   << "    <th>Exp ID</th>\n"
		   << "    <th>Title</th>\n"
		   << "    <th>Description</th>\n"
		   << "    <th>Worms Scored</th>\n"
		   << "		<th> Delete </th>\n"
		   << "		<th> Backup </th>\n"
		   << "		<th> Archive </th>\n"
		   << "  </tr>" << endl;

      // create HTML table entries for each experiment
      for (int i = 0; i <= expID; i++) {

	  stringstream dfile;
    	  dfile << datapath << i << "/description.txt";
    	  ifstream readdesc(dfile.str().c_str());
	  if (!readdesc.good()) {
		if (!checkForArchive(i).empty()) ; else continue;
		 //if the experiment is empty skip
	  }

    	  // create table element for experiment ID
    	  cout << "  <tr>\n"
    	       << "    <td><a href=\"/cgi-bin/marker?loadedexpID=" << i << "\" target=\"_blank\">" << i << "</a></td>" << endl;

    	  // get title and description of this experiment from description.txt
    	  // ***REPLACE WITH JSON***
    	  
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
    	  cout << "    <td><a href=\"/cgi-bin/marker?loadedexpID=" << i << "\" target=\"_blank\">" << title << "</a></td>" << endl;
    	  cout << "    <td><a href=\"/cgi-bin/marker?loadedexpID=" << i << "\" target=\"_blank\">" << description << "</a></td>" << endl;

    	  // get a count of worms in this experiment
    	  stringstream wfile;
    	  wfile << datapath << i << "/wormlist.csv";
    	  ifstream wfilestream(wfile.str().c_str());
    	  int num_worms = 0;
    	  string aworm;
    	  while (getline(wfilestream,aworm)) ++num_worms;
    	  wfilestream.close();

    	  // create table element for worm count
    	  cout << "    <td><a href=\"/cgi-bin/marker?loadedexpID=" << i << "\" target=\"_blank\">" << num_worms << "</a></td>" << endl;

    	  // create buttons for deleting and downloading this experiment's data
    	  cout << "    <td><button type=\"button\" onclick=\"deleteExp(" << i << ")\"><img src=\"/wormbot/img/icon_delete.png\"</button></td>" << endl;
    	  cout << "    <td><button type=\"button\" onclick=\"downloadExp(" << i << ")\"><img src=\"/wormbot/img/icon_download.png\"</button></td>" << endl;

	  // create download links if they exist for the experiments

	  cout << "<td> <a href=\"" << checkForArchive(i) << "\"> " << checkForArchive(i) << "</a></td>" << endl; 

    	  // end this table row
    	  cout << "  </tr>" << endl;
      }

      // end table
      cout << "</table>\n" << endl;

      // add script functions for deleting and downloading experimental data
      cout << "<script>\n"
    	   << "  function deleteExp(id) {\n"
		   << "    if (confirm(\"Delete all data for this experiment?\")) { filename = \"/cgi-bin/experimentbrowser\";" 	<< endl
		   << " $.ajax({ " 													<< endl
		   << " type : \"POST\", " 												<< endl
                   << " url  : filename," 												<< endl
                   << "	data : { \"delete\":id},"											<< endl
                   << "	success : function (){ "											<< endl
		   << "	alert( \"experiment:\" + id + \" deleted\");"									<< endl	
		   << " window.location.reload(true); }"										<< endl
                   << " });"														<< endl
		   << "  }}\n"														<< endl
		   << "  function downloadExp(id) {\n"
		   << "    if (confirm(\"Make archive of all data for this experiment?\")) { filename = \"/cgi-bin/experimentbrowser\";" 	<< endl
		   << " $.ajax({ " 													<< endl
		   << " type : \"POST\", " 												<< endl
                   << " url  : filename," 												<< endl
                   << "	data : { \"backup\":id},"											<< endl
                   << "	success : function (){ "											<< endl
		   << "	alert( \"experiment:\" + id + \" compressed\");"								<< endl	
		   << " window.location.reload(true); }"										<< endl
                   << " });"														<< endl
		   << "  }\n"														<< endl
		   << "  }\n"
		   << "</script>" << endl;

      cout << br() <<  endl;
     // cout << img().set("src", "http://undergradrobot.dynu.com/Bender.png").set("width","100") << endl;
    //  cout << form().set("action", "http://undergradrobot.dynu.com/cgi-bin/robotscheduler").set("method", "POST") << endl;

      // Close the HTML document
      cout << body() << endl << endl << html() << endl;

   
}
