#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

enum Event {
	ARRIVAL = 0,
	PREEMPTION = 1,
	IOREQUEST = 2,
	IODONE = 3,
	TERMINATION = 4,
	IONOTDONE = 5
};

struct ProcessControlBlock {
	int processID = 0;
	int arrivalTime = 0;
	int numCPUBursts = 0;
	vector<int> cpuBursts;
	vector<int> ioBursts;
	int turnAroundTime = 0;
	int readyWait = 0;
	int ioWait = 0;
	int cpuUtilization = 0;
	int ioUtilization = 0;
};

struct EventNode {
	int time = 0; /* when the event will occur */
	Event type; /* type of event */
	int processID = 0; /* to whom this event applies */
};

bool eventComparator(const EventNode& a, const EventNode& b) {
	if (a.time != b.time)
		return a.time < b.time;

	static const vector<Event> eventOrder = {
		ARRIVAL, TERMINATION, IODONE, PREEMPTION, IOREQUEST, IONOTDONE
	};

	auto aPosition = find(eventOrder.begin(), eventOrder.end(), a.type);
	auto bPosition = find(eventOrder.begin(), eventOrder.end(), b.type);
	return aPosition < bPosition;
}

vector<EventNode> createEventList(const vector<ProcessControlBlock>& processes) { 
	vector<EventNode> eventList;
	for (const auto& process : processes) {
		EventNode newEvent;
		newEvent.time = process.arrivalTime;
		newEvent.type = ARRIVAL;
		newEvent.processID = process.processID;
		eventList.push_back(newEvent);
	}
	return eventList;
}

void logEvent(const string& message, ofstream& logFile) {
	logFile << message;
}

void resetCpuOrIoProcess(ProcessControlBlock& pcb) {
	pcb.processID = -1;
	pcb.arrivalTime = -1;
	pcb.numCPUBursts = -1;
	pcb.cpuBursts.clear();
	pcb.ioBursts.clear();
}

int jobSchedulerSimulation(vector<ProcessControlBlock>& processes, vector<EventNode>& eventList, int timeQuantum, ofstream& logFile, ofstream& visualFile) {
	bool CPU_IDLE = true, IO_IDLE = true;
	int SIM_TIME = 0;
	queue<ProcessControlBlock> readyQueue;
	queue<ProcessControlBlock> ioQueue;
	ProcessControlBlock cpuProcess;
	ProcessControlBlock ioProcess;
	int SIM_START_TIME = eventList.front().time;

	while (!eventList.empty()) {
		if (eventList.size() > 1)
			sort(eventList.begin(), eventList.end(), eventComparator);
		EventNode currentEvent = eventList.front();
		eventList.erase(eventList.begin());
		int pid = currentEvent.processID;

		// While simulation time is less than the first event arrival time,
		while (SIM_TIME < currentEvent.time) {
			SIM_TIME++;
			if (SIM_TIME - 1 == 0) {
				logEvent("TIME QUANTUM: " + to_string(timeQuantum) + "\nTime: 0 Simulation started.\n", logFile);
			}
			else
				logEvent("Time: " + to_string(SIM_TIME - 1) + " No event\n", logFile);

			// When theres a process in the CPU, decrement the cpu burst by 1
			if (cpuProcess.cpuBursts.size() > 0) {
				for (int i = 0; i < processes[cpuProcess.processID].cpuBursts.size(); i++) {
					if (processes[cpuProcess.processID].cpuBursts[i] > 0) {
						processes[cpuProcess.processID].cpuBursts[i]--;
						break;
					}
				}
			}

			// When theres a process in the Blocked state, decrement the I/O burst by 1
			if (ioProcess.ioBursts.size() > 0) {
				for (int i = 0; i < processes[ioProcess.processID].ioBursts.size(); i++) {
					if (processes[ioProcess.processID].ioBursts[i] > 0) {
						processes[ioProcess.processID].ioBursts[i]--;
						break;
					}
				}
			}

			if (SIM_TIME % 5 == 0 && SIM_TIME != 0) {
				logEvent("-------------------\nREADY QUEUE: \n", logFile);
			}
			// Increment ReadyWait time for each process in the ready queue
			queue<ProcessControlBlock> tempRQ = readyQueue; 
			while (!tempRQ.empty()) { 
				ProcessControlBlock pcb = tempRQ.front();
				tempRQ.pop();
				processes[pcb.processID].readyWait++; 
				if (SIM_TIME % 5 == 0 && SIM_TIME != 0) {
					logEvent("Process: " + to_string(pcb.processID) + ", # CPU Bursts: " + to_string(pcb.numCPUBursts) + ", CPU Bursts: ", logFile);
					for (auto& cpuBurst : pcb.cpuBursts) {
						logEvent(to_string(cpuBurst) + ", ", logFile);
					}
					logEvent("I/O Bursts: ", logFile);
					for (auto& ioBurst : pcb.ioBursts) {
						logEvent(to_string(ioBurst) + ", ", logFile);
					}
				}
			}
			if (SIM_TIME % 5 == 0 && SIM_TIME != 0) {
				logEvent("\n-------------------\nI/O QUEUE: \n", logFile);
			}
			// Increment I/O-Wait time for each process in the I/O queue
			queue<ProcessControlBlock> ioQ = ioQueue; 
			while (!ioQ.empty()) { 
				ProcessControlBlock pcb = ioQ.front();
				ioQ.pop();
				processes[pcb.processID].ioWait++; 
				if (SIM_TIME % 5 == 0 && SIM_TIME != 0) {
					logEvent("Process: " + to_string(pcb.processID) + ", # CPU Bursts: " + to_string(pcb.numCPUBursts) + ", CPU Bursts: ", logFile); 
					for (auto& cpuBurst : pcb.cpuBursts) { 
						logEvent(to_string(cpuBurst) + ", ", logFile); 
					}
					logEvent("I/O Bursts: ", logFile); 
					for (auto& ioBurst : pcb.ioBursts) { 
						logEvent(to_string(ioBurst) + ", ", logFile); 
					}
				}
			}
			if (SIM_TIME % 5 == 0 && SIM_TIME != 0) {
				logEvent("\n-------------------\n", logFile);
			}

			// VISUALIZATION OF THE PCB EVERY 15 SECONDS!
			if (SIM_TIME % 15 == 0 && SIM_TIME != 0) {
				queue<ProcessControlBlock> tmpRdy = readyQueue; 
				queue<ProcessControlBlock> tmpIO = ioQueue; 
				int currCpuTime = 0;
				int currIoTime = 0;
				for (int i = 0; i < processes.size(); i++) {
					currCpuTime += processes[i].cpuUtilization;
					currIoTime += processes[i].ioUtilization;
				}
				double currCpuUtil = ((double)SIM_START_TIME / (double)currCpuTime) * 100.00;
				double currIoUtil = ((double)SIM_START_TIME / (double)currIoTime) * 100.00;
				visualFile << "Time: " + to_string(SIM_TIME) + ", Statistics for simulation with " + to_string(processes.size()) + " jobs.\n";
				visualFile << "Time quantum: " + to_string(timeQuantum) + ", Current CPU Utilization: " + to_string(currCpuUtil) + "%, ";
				visualFile << "Current I/O Utilization: " + to_string(currIoUtil) + "%\n\nREADY QUEUE\n---------------\n";
				while (!tmpRdy.empty()) {
					ProcessControlBlock pcb = tmpRdy.front();
					tmpRdy.pop();
					visualFile << "Process ID: " + to_string(pcb.processID) + ", Arrival Time: " + to_string(pcb.arrivalTime) + ", Number of CPU Bursts: "
						+ to_string(pcb.numCPUBursts) + ", CPU Bursts: ";
					for (int i = 0; i < pcb.cpuBursts.size(); i++) {
						visualFile << to_string(pcb.cpuBursts[i]) + ", ";
					}
					visualFile << "I/O Bursts: ";
					for (int i = 0; i < pcb.ioBursts.size(); i++) {
						visualFile << to_string(pcb.ioBursts[i]) + ", ";
					}
					visualFile << "\n";
				}
				visualFile << "\nCPU PROCESS\n---------------\n";
				if (cpuProcess.processID > 0) {
					visualFile << "Process ID: " + to_string(cpuProcess.processID) + ", Arrival Time: " + to_string(cpuProcess.arrivalTime) +
						", Number of CPU Bursts: " + to_string(cpuProcess.numCPUBursts) + ", CPU Bursts: ";
					for (int i = 0; i < cpuProcess.cpuBursts.size(); i++) {
						visualFile << to_string(cpuProcess.cpuBursts[i]) + ", ";
					}
					visualFile << "I/O Bursts: ";
					for (int i = 0; i < cpuProcess.ioBursts.size(); i++) {
						visualFile << to_string(cpuProcess.ioBursts[i]) + ", ";
					}
					visualFile << "\n";
				}
				visualFile << "\nI/O PROCESS\n---------------\n";
				if (ioProcess.processID > 0) {
					visualFile << "Process ID: " + to_string(ioProcess.processID) + ", Arrival Time: " + to_string(ioProcess.arrivalTime) +
						", Number of CPU Bursts: " + to_string(ioProcess.numCPUBursts) + ", CPU Bursts: ";
					for (int i = 0; i < ioProcess.cpuBursts.size(); i++) {
						visualFile << to_string(ioProcess.cpuBursts[i]) + ", ";
					}
					visualFile << "I/O Bursts: ";
					for (int i = 0; i < ioProcess.ioBursts.size(); i++) {
						visualFile << to_string(ioProcess.ioBursts[i]) + ", ";
					}
					visualFile << "\n";
				}
				visualFile << "\nI/O QUEUE\n---------------\n";
				while (!tmpIO.empty()) {
					ProcessControlBlock pcb = tmpIO.front();
					tmpIO.pop();
					visualFile << "Process ID: " + to_string(pcb.processID) + ", Arrival Time: " + to_string(pcb.arrivalTime) + ", Number of CPU Bursts: "
						+ to_string(pcb.numCPUBursts) + ", CPU Bursts: ";
					for (int i = 0; i < pcb.cpuBursts.size(); i++) {
						visualFile << to_string(pcb.cpuBursts[i]) + ", ";
					}
					visualFile << "I/O Bursts: ";
					for (int i = 0; i < pcb.ioBursts.size(); i++) {
						visualFile << to_string(pcb.ioBursts[i]) + ", ";
					}
					visualFile << "\n";
				}
				visualFile << "\n---------------\n";
			}
		}

		// When it is the events time, enter the type of event
		switch (currentEvent.type) {
			case 0:	// ARRIVAL
			{
				// create a new (simulated) process and place it in the Ready Queue
				readyQueue.push(processes[pid]);
				logEvent("Time: " + to_string(SIM_TIME) + " Arrival event for process: " + to_string(pid) + "\n", logFile);	// log the arrival event
				break;
			}

			case 1: // PREEMPTION
			{
				// put the current process in the Ready Queue
				readyQueue.push(processes[pid]);
				CPU_IDLE = true;
				resetCpuOrIoProcess(cpuProcess);
				logEvent("Time: " + to_string(SIM_TIME) + " Preemption event for process: " + to_string(pid) + "\n", logFile);	// log the preemption event
				break;
			}

			case 2: // I/O REQUEST
			{
				// free CPU and move the current process into the I/O Queue
				ioQueue.push(processes[pid]);
				CPU_IDLE = true;
				resetCpuOrIoProcess(cpuProcess);
				logEvent("Time: " + to_string(SIM_TIME) + " I/O request event for process: " + to_string(pid) + "\n", logFile);	// log the I/O request event
				break;
			}

			case 3: // I/O DONE
			{
				// free I/O device and put the process in the Ready Queue
				readyQueue.push(processes[pid]);
				IO_IDLE = true;
				resetCpuOrIoProcess(ioProcess); 
				logEvent("Time: " + to_string(SIM_TIME) + " I/O done event for process: " + to_string(pid) + "\n", logFile);	 // log the I/O done event
				break;
			}

			case 4: // TERMINATION
			{
				// free up the CPU, update statistics of the entire simulation
				CPU_IDLE = true;
				resetCpuOrIoProcess(cpuProcess); 
				logEvent("Time: " + to_string(SIM_TIME) + " Termination event for process: " + to_string(pid) + "\n", logFile);	// log the termination event
				break;
			}

			case 5: // I/O NOT DONE, RETURN TO I/O QUEUE
			{
				ioQueue.push(processes[pid]);
				IO_IDLE = true;
				resetCpuOrIoProcess(ioProcess);
				logEvent("Time: " + to_string(SIM_TIME) + " I/O not done, return to I/O queue for process: " + to_string(pid) + "\n", logFile);
				break;
			}
		}

		if (CPU_IDLE && !readyQueue.empty()) {
			CPU_IDLE = false;
			// perform dispatch a process routine - remove the first process from the Ready Queue
			cpuProcess = readyQueue.front();
			int cpuPID = cpuProcess.processID;
			readyQueue.pop();
			for (int i = 0; i < cpuProcess.cpuBursts.size(); i++) {
				if (cpuProcess.cpuBursts[i] > 0) {
					if (cpuProcess.cpuBursts[i] > timeQuantum) {
						currentEvent.type = PREEMPTION;	// Preemption because the cpuBurst is greather than the timeQuantum
						currentEvent.time = timeQuantum + SIM_TIME;
						currentEvent.processID = cpuPID;
						processes[cpuPID].cpuUtilization += timeQuantum;
						eventList.push_back(currentEvent);
						break;
					}
					else if (cpuProcess.cpuBursts[i] <= timeQuantum) {
						currentEvent.time = cpuProcess.cpuBursts[i] + SIM_TIME;
						processes[cpuPID].cpuUtilization += cpuProcess.cpuBursts[i];
						if (i + 1 <= cpuProcess.ioBursts.size())
							currentEvent.type = IOREQUEST;
						else {
							currentEvent.type = TERMINATION;
							processes[cpuPID].turnAroundTime = currentEvent.time - processes[cpuPID].arrivalTime;
						}
						currentEvent.processID = cpuPID;
						eventList.push_back(currentEvent);
						break;
					}
				}
			}
			logEvent("Time: " + to_string(SIM_TIME) + " Job dispatch event for process: " + to_string(cpuProcess.processID) + "\n", logFile);
		}

		if (IO_IDLE && !ioQueue.empty()) {
			IO_IDLE = false;
			// perform IO operation routine - remove the first process from the I/O Queue
			ioProcess = ioQueue.front();
			int ioPID = ioProcess.processID;
			ioQueue.pop();
			for (int i = 0; i < ioProcess.ioBursts.size(); i++) {
				if (ioProcess.ioBursts[i] > 0) {
					if (ioProcess.ioBursts[i] > timeQuantum) {
						currentEvent.type = IONOTDONE;
						currentEvent.time = timeQuantum + SIM_TIME;
						currentEvent.processID = ioPID;
						processes[ioPID].ioUtilization += timeQuantum;
						eventList.push_back(currentEvent);
						break;
					}
					else if (ioProcess.ioBursts[i] <= timeQuantum) {
						currentEvent.time = ioProcess.ioBursts[i] + SIM_TIME;
						currentEvent.type = IODONE;
						currentEvent.processID = ioPID;
						processes[ioPID].ioUtilization += ioProcess.ioBursts[i];
						eventList.push_back(currentEvent);
						break;
					}
				}
			}
			logEvent("Time: " + to_string(SIM_TIME) + " I/O operation event for process: " + to_string(ioProcess.processID) + "\n", logFile);
		}
	}
	return SIM_TIME; 
}

int main() {
	string inputFile;
	string outputFile;
	string logFile;
	string visualFile;
	ifstream checkFileExist;

	cout << "Enter the name of the output file (with .txt at the end): ";
	// error handling to ensure the entered file name isn't already an existing file
	while (true) {
		getline(cin, outputFile);

		checkFileExist.open(outputFile);
		if (checkFileExist.is_open()) {
			cout << "\nFile already exists with the name " << outputFile << ". Please enter a different name: ";
			checkFileExist.close();
		}
		else {
			break;
		}
	}

	cout << "\nEnter the name of the input file (with .txt at the end): ";
	// error handling to ensure the input file name exists
	while (true) {
		getline(cin, inputFile);

		checkFileExist.open(inputFile);
		if (!checkFileExist.is_open()) {
			cout << "\nInput file with the name: " << inputFile << " wasn't found. Please enter the correct input file name: ";
		}
		else {
			checkFileExist.close();
			break;
		}
	}

	cout << "\nEnter the name of the log file (with .txt at the end): ";
	// error handling to ensure the entered file name isn't already an existing file
	while (true) {
		getline(cin, logFile);

		checkFileExist.open(logFile);
		if (checkFileExist.is_open()) {
			cout << "\nFile already exists with the name " << logFile << ". Please enter a different name: ";
			checkFileExist.close();
		}
		else {
			break;
		}
	}

	cout << "\nEnter the name of the visualization file (with .txt at the end): ";
	while (true) {
		getline(cin, visualFile);
		checkFileExist.open(visualFile);
		if (checkFileExist.is_open()) {
			cout << "\nFile already exists with the name " << visualFile << ". Please enter a different name: ";
			checkFileExist.close();
		}
		else
			break;
	}

	int timeQuantum;
	cout << "\nEnter the time quantum for the simulation: ";

	while (true) {
		cin >> timeQuantum; 
		if (timeQuantum > 0)
			break;
		else {
			cout << "Time Quantum must be greater than 0. Please try again: ";
		}
	}

	// gets the process list given the name of the input file
	vector<ProcessControlBlock> processList;
	ifstream input(inputFile);
	string line;
	int pid = 0;
	int totalCpuTime = 0;
	ofstream logs(logFile); 
	ofstream visual(visualFile);

	while (getline(input, line)) {
		bool invalid = false; 
		bool invalidBursts = false;
		stringstream ss(line);
		ProcessControlBlock pcb;
		pcb.processID = pid++;
		ss >> pcb.arrivalTime >> pcb.numCPUBursts; 

		int burstTime; 
		bool isCPUBurst = true; 
		int totalBursts = pcb.numCPUBursts * 2 - 1; 

		for (int i = 0; i < totalBursts; ++i) { 
			ss >> burstTime; 
			if (isCPUBurst) { 
				totalCpuTime += burstTime; 
				pcb.cpuBursts.push_back(burstTime); 
				if (burstTime < 0)
					invalidBursts = true;
			}
			else
				pcb.ioBursts.push_back(burstTime); 
			isCPUBurst = !isCPUBurst; 
		}
		if (pcb.arrivalTime < 1) {
			cout << "Invalid start time, must be > 0! Start time input = " + to_string(pcb.arrivalTime) + "\n";
			invalid = true;
		}
		if (pcb.numCPUBursts < 1) {
			cout << "Invalid number of CPU Bursts, must be > 0! Number of CPU Bursts = " + to_string(pcb.numCPUBursts) + "\n";
			invalid = true;
		}
		if (pcb.numCPUBursts != pcb.cpuBursts.size()) {
			cout << "The number of CPU Bursts and the actual CPU Bursts given doesn't match!\n";
			invalid = true;
		}
		if (invalidBursts)
			cout << "CPU Burst or I/O Burst value must be > 0!\n";

		if (!invalid && !invalidBursts)
			processList.push_back(pcb);
		else
			cout << "Invalid data in this job. Moving to next job.";
	}
	input.close();

	// initialize the events list given the input data
	vector<EventNode> eventList = createEventList(processList);
	int firstArrivalTime = eventList.front().time;
	int totalSimTime = jobSchedulerSimulation(processList, eventList, timeQuantum, logs, visual);
	double totalCpuUtilization = (double)totalCpuTime / ((double)totalSimTime - (double)firstArrivalTime);

	ofstream fileOutput(outputFile);
	if (fileOutput.is_open()) {
		fileOutput << "Results for quantum = " + to_string(timeQuantum) + " CPU utilization = " + to_string(totalCpuUtilization * 100) + "%\n";
		for (auto& process : processList) {
			fileOutput << "P" + to_string(process.processID) + "(TAT = " + to_string(process.turnAroundTime) + ", ReadyWait = " +
				to_string(process.readyWait) + ", I/O-wait = " + to_string(process.ioWait) + ")\n";
		}
		fileOutput.close();
	}
	else {
		cerr << "Error creating the file: " << outputFile << endl;
	}
	return 0;
}