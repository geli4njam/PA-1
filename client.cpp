/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Chesna Nwoke
	UIN: 734002070
	Date: 09/17/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <iostream>
#include <fstream> 

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int m = MAX_MESSAGE;
	bool new_chan = false;
	vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	//give arguments for the server
	//server needs './server', '-m', '<val for -m arg>' 'NULL'
	//fork
	//in the child, run execvp using the server

	string m_str = to_string(m);
	char* cmd1[] = {(char*) "./server", (char*) "-m", (char*) m_str.c_str(), nullptr};

    pid_t pid = fork();

    if (pid == -1) {
    cerr << "fork failed\n";
    return 1;
    }    
	
    if (pid == 0){
		execvp(cmd1[0], cmd1);
		cerr << "exec failed\n";
		return 1;
    }
	

    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);


	if(new_chan){
		// send request to the server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
    	cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		// create a variable to hold the name
		char c_name[MAX_MESSAGE];
		// create the response from the server
		cont_chan.cread(c_name, sizeof(c_name));
		// call the FIFORequestChannel constructor with the name from the server
		FIFORequestChannel* new_channel = new FIFORequestChannel(c_name, FIFORequestChannel::CLIENT_SIDE);
		//adding to channels vector
		channels.push_back(new_channel);
	}

	FIFORequestChannel chan = *(channels.back());

	
	// Single datapoint, only run p,t,e != -1
	// example data point request
    char buf[MAX_MESSAGE]; // 256

	if(p != -1 && t != -1.0 && e != -1){
    datamsg x(p, t, e); //change from hard coding to users values
	memcpy(buf, &x, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg)); // question
	double reply;
	chan.cread(&reply, sizeof(double)); //answer
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

	}else if(p != -1){
		//Else if p != -1, request 1000 datapoints
	// Loop over 1st 1000 lines 
	// send request for ecg 1
	// send request for ecg 2
	// wirte line to recieved/x1.csv
	ofstream outfile("received/x1.csv");
    
    // Loop over first 1000 time points (0.000 to 3.996 in 4ms increments)
    for (int i = 0; i < 1000; i++) {
        double time = i * 0.004;  // 4ms increments
        
        // Request ECG1 value
        datamsg msg1(p, time, 1);
        memcpy(buf, &msg1, sizeof(datamsg));
        chan.cwrite(buf, sizeof(datamsg));
        double ecg1;
        chan.cread(&ecg1, sizeof(double));
        
        // Request ECG2 value  
        datamsg msg2(p, time, 2);
        memcpy(buf, &msg2, sizeof(datamsg));
        chan.cwrite(buf, sizeof(datamsg));
        double ecg2;
        chan.cread(&ecg2, sizeof(double));
        
        // Write to file in same format as original: time,ecg1,ecg2
        outfile << time << "," << ecg1 << "," << ecg2 << endl;
    }
    outfile.close();
	}

	if (!filename.empty()){
	// sending a non-sense message, you need to change this
	filemsg fm(0, 0);
	string fname = filename;

	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);  // I want the file length;

	int64_t filesize = 0;
	chan.cread(&filesize, sizeof(int64_t));

	char* buf3 = new char[m];//create buffer of size buff capacity(m)

	ofstream outfile("received/" + fname, ios::binary);
	// loop ovee the segments in the file filesize / buff capacity(m).
	// create filemsg insance
	int64_t offset = 0;
	while (offset < filesize){
	filemsg* file_req = (filemsg*) buf2; 
	file_req->offset = offset;//set offset in the file
	file_req->length = min((int64_t)m, filesize-offset);//set the length. Be careful of the last segment
	//send the request (buf2)
	chan.cwrite(buf2, len);
	// recieve the response
	// cread into buf3 length file_req->len
	chan.cread(buf3, file_req->length);
	// write buf3 into file: received/filename
	outfile.write(buf3, file_req->length);
	
	offset += file_req->length;
	}
	outfile.close();
	

	delete[] buf2;
	delete[] buf3; 
	}
	
	// if necessary, close and delete the new channel
	if(new_chan){
		// do your close and deletes
    MESSAGE_TYPE qm = QUIT_MSG;
    channels.back()->cwrite(&qm, sizeof(MESSAGE_TYPE));
	delete channels.back();
	channels.pop_back();

	}
	
	// closing the channel    
    MESSAGE_TYPE qm = QUIT_MSG;
    chan.cwrite(&qm, sizeof(MESSAGE_TYPE));
}
