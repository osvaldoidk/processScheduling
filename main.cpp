/*
    Author: Osvaldo 

    This program simulates process schedluing of a mode with one core, one disk,
    one ready queue, one disk queue, and unlimitd user i/o devices

    Date completed: 6/29/2019

    Known bug: - the last line in the input file should not be an empty line (should be the last record)
               - the last output line of the program is not part of the simulation (completion flag)
*/

#include <iostream>
#include <vector>
#include <string>
#include <queue>
//#include <bits/stdc++.h>

using namespace std;

//contents for dataTable
struct dataTableContent
{
    string keyword;
    int time;
};

//contents for processTable
struct processTableContent
{
    int startTime;
    int firstLine;
    int lastLine;
    int currentLine;
    string processState;
    int userDisplayCompletionTime; // user input/output completion times
};

// contents for deviceTable
struct devices
{
    int processId;
    int deviceUsedCompletionTime;
    bool busy;
};

// Function prototypes

void initializeDataTableProcessTableDeviceTable(vector<dataTableContent> &dTable, vector<processTableContent> &pTable, devices dviceTable[]);

void displayTables(vector<dataTableContent> &dTable, vector<processTableContent> &pTable, devices dviceTable[]);

void coreDiskRequestRoutine(int deviceId, int howLong, int procId, devices dviceTable[], queue<int> cDiskReadyQueues[], vector<processTableContent> &pTable, vector<dataTableContent> &dTable);

void coreDiskCompletionRoutine(int deviceId, queue<int> cDiskReadyQueues[], devices dviceTable[], vector<dataTableContent> &dTable,
                               vector<processTableContent> &pTable);

void userRequestRoutine(int howLong, int procId, vector<processTableContent> &pTable);

void userCompletionRoutine(int procId, vector<processTableContent> &pTable, devices dviceTable[], queue<int> cDiskReadyQueues[], vector<dataTableContent> &dTable);

void nextJumpTimeVersion2(vector<processTableContent> &pTable, devices dviceTable[], int &table, int &row, int &column);

void decodeScheduleNextRequest(vector<dataTableContent> &dTable, vector<processTableContent> &pTable, int procId, devices dviceTable[], queue<int> cDiskReadyQueues[]);

void displayProcessTermination(int procId, vector<processTableContent> &pTable, devices dviceTable[], vector<dataTableContent> &dTable);

void zeroDelayDiskAccess(int procId, vector<processTableContent> &pTable, devices dviceTable[], queue<int> cDiskReadyQueues[], vector<dataTableContent> &dTable); 

string deviceString(int dviceId);

string setProcessState(int dviceId);

// Global variables 

int currentJumpTime = 0;
const int defaultCompletionTime = 1000000; // an arbitrary high number that determines when all processes are done

int main()
{

    // Main data structures
    vector<dataTableContent> dataTable;
    vector<processTableContent> processTable;
    devices deviceTable[2]; // One Core and one Disk
    queue<int> coreDiskReadyQueues[2];

    int table, row, column;
    
    initializeDataTableProcessTableDeviceTable(dataTable, processTable, deviceTable);
    displayTables(dataTable, processTable, deviceTable);

    while (currentJumpTime != defaultCompletionTime)
    {
        nextJumpTimeVersion2(processTable, deviceTable, table, row, column);

        if (table == 0) // next jump time in process table
        {
            switch (column)
            {
            case 0: // case when jump time is start time
                decodeScheduleNextRequest(dataTable, processTable, row, deviceTable, coreDiskReadyQueues);
                break;
            case 1: // case when jump time is user I/O completion time
                userCompletionRoutine(row, processTable, deviceTable, coreDiskReadyQueues, dataTable);
                break;
            }
        }
        else // next jump time in devices table (core(0) or disk(1))
        {
            coreDiskCompletionRoutine(row, coreDiskReadyQueues, deviceTable, dataTable, processTable);
        }

    }

    return 0;
}

void initializeDataTableProcessTableDeviceTable(vector<dataTableContent> &dTable, vector<processTableContent> &pTable, devices dviceTable[])
{
    int lineIndex = 0;

    while (!cin.eof())
    {

        dataTableContent dataRecord;
        cin >> dataRecord.keyword >> dataRecord.time;
        dTable.push_back(dataRecord);

        if (dataRecord.keyword == "NEW")
        {
            processTableContent processRecord;
            processRecord.startTime = dataRecord.time;
            processRecord.firstLine = lineIndex;
            processRecord.currentLine = lineIndex;
            processRecord.processState = " ";
            processRecord.userDisplayCompletionTime = defaultCompletionTime;
            pTable.push_back(processRecord);
        }

        lineIndex++;
    }

    //assign lastLine for each process record
    int numOfProcesses = pTable.size();
    for (int i = 0; i < numOfProcesses; i++)
    {
        if (i != numOfProcesses - 1)
        {
            pTable[i].lastLine = pTable[i + 1].firstLine - 1;
        }
        else
        {
            pTable[i].lastLine = lineIndex - 1;
        }
    }

    //initialize Device Table

    dviceTable[0].deviceUsedCompletionTime = defaultCompletionTime;
    dviceTable[0].busy = false;
    dviceTable[1].deviceUsedCompletionTime = defaultCompletionTime;
    dviceTable[1].busy = false;
}

void displayTables(vector<dataTableContent> &dTable, vector<processTableContent> &pTable, devices dviceTable[])
{

    // Display Data table

    cout << "THE DATA TABLE" << endl << endl;
    cout << "Row    Keyword    Time" << endl;
    for (int i = 0; i < dTable.size(); i++)
    {
        cout << "Row " << i << " " << dTable[i].keyword << " " << dTable[i].time << endl;
    }

    // Display initial process table

    cout << endl << endl << "THE PROCESS TABLE" << endl << endl;
    cout << "Row  StartTime  FirstLine LastLine CurrentLine ProcessState DisplayCompleTIme" << endl;
    for (int i = 0; i < pTable.size(); i++)
    {
        cout << i << " " << pTable[i].startTime << " " << pTable[i].firstLine
             << " " << pTable[i].lastLine << " " << pTable[i].currentLine << " "
             << pTable[i].processState << " " << pTable[i].userDisplayCompletionTime << endl;
    }

    // Display DEvice busy state
    cout << endl << endl << "DEVICES" << endl;
    cout << "CPU is busy?  : " << dviceTable[0].busy << endl;
    cout << "Disk is busy?  : " << dviceTable[1].busy << endl;
}

void coreDiskRequestRoutine(int deviceId, int howLong, int procId, devices dviceTable[], queue<int> cDiskReadyQueues[], vector<processTableContent> &pTable,
                            vector<dataTableContent> &dTable)
{
    cout << "-- Process " << procId << " requests " << deviceString(deviceId) << " at time " << currentJumpTime << " for " << howLong << " ms" << endl;

    if (howLong > 0)
    {
        if (dviceTable[deviceId].busy == false)
        {
            dviceTable[deviceId].busy = true;
            dviceTable[deviceId].deviceUsedCompletionTime = currentJumpTime + howLong;
            dviceTable[deviceId].processId = procId;
            pTable[procId].processState = "RUNNING";

            cout << "-- Process " << procId << " will release " << deviceString(deviceId) << " at time " 
                << dviceTable[deviceId].deviceUsedCompletionTime << " ms" << endl;
        }
        else
        {
            cDiskReadyQueues[deviceId].push(procId);
            pTable[procId].processState = setProcessState(deviceId);
            cout << "-- Process " << procId << " must wait for " << deviceString(deviceId) << endl;
            cout << "-- " << deviceString(deviceId) << " queue now contains " << cDiskReadyQueues[deviceId].size() << " process(es) waiting" << endl;
        } // end inner if
    }
    else
    {
        zeroDelayDiskAccess(procId, pTable, dviceTable, cDiskReadyQueues, dTable);
    } // end outer if
}

void coreDiskCompletionRoutine(int deviceId, queue<int> cDiskReadyQueues[], devices dviceTable[], vector<dataTableContent> &dTable,
                               vector<processTableContent> &pTable)
{

    int procId = dviceTable[deviceId].processId;
    cout << endl << endl << "-- " << deviceString(deviceId) << " completion event for a process " << procId << " at time " << currentJumpTime << " ms" << endl;

    if (!cDiskReadyQueues[deviceId].empty())
    {

        int newProcId = cDiskReadyQueues[deviceId].front();
        dviceTable[deviceId].processId = newProcId;
        int procCurrentLine = pTable[newProcId].currentLine;
        int howLong = dTable[procCurrentLine].time;

        pTable[newProcId].processState = "RUNNING";

        dviceTable[deviceId].deviceUsedCompletionTime = currentJumpTime + howLong;

        cout << "-- Process " << newProcId << " will release " << deviceString(deviceId) << " at time " << dviceTable[deviceId].deviceUsedCompletionTime << " ms" << endl;
        cDiskReadyQueues[deviceId].pop();
    }
    else
    {
        dviceTable[deviceId].busy = false;
        dviceTable[deviceId].deviceUsedCompletionTime = defaultCompletionTime;
    } // end if

    pTable[procId].currentLine++;

    // Process is done when currentline > lastline
    if (pTable[procId].currentLine > pTable[procId].lastLine)
    {
        displayProcessTermination(procId, pTable, dviceTable, dTable);
    }
    else
    {
        decodeScheduleNextRequest(dTable, pTable, procId, dviceTable, cDiskReadyQueues);
    } // end if
}

void userRequestRoutine(int howLong, int procId, vector<processTableContent> &pTable)
{
    pTable[procId].userDisplayCompletionTime = currentJumpTime + howLong;
    pTable[procId].processState = "BLOCKED";
    cout << "-- Process " << procId << " will complete input from user at time " << pTable[procId].userDisplayCompletionTime << " ms" << endl;
}

void userCompletionRoutine(int procId, vector<processTableContent> &pTable, devices dviceTable[], queue<int> cDiskReadyQueues[], vector<dataTableContent> &dTable)
{
    pTable[procId].userDisplayCompletionTime = defaultCompletionTime;
    cout << endl << endl << "-- USER completion event for process " << procId << " at time " << currentJumpTime << " ms" << endl;
    pTable[procId].currentLine++;

    // Process is  done when currentline > lastline
    if (pTable[procId].currentLine > pTable[procId].lastLine)
    {
        displayProcessTermination(procId, pTable, dviceTable, dTable); 
    }
    else
    {
        decodeScheduleNextRequest(dTable, pTable, procId, dviceTable, cDiskReadyQueues); 
    }
}

// Driver of program
void nextJumpTimeVersion2(vector<processTableContent> &pTable, devices dviceTable[], int &table, int &row, int &column)
{
    // min priority queue
    // stores scanned start times, i/o completions, user and core completion times
    // only stores times bigger than current jump time and the time on top is the next jump time
    priority_queue<int, vector<int>, greater<int> > candidateJumpTimesList; 
   
    cout << endl << endl << "##########################################" << endl; // for debugging purposes
    cout << "Jump time when entering: " << currentJumpTime << endl;               // for debugging purposes
    
    // get times in the process table
    for (int i = 0; i < pTable.size(); i++)
    {
        if (pTable[i].currentLine <= pTable[i].lastLine) // get times only from non terminated processes
        {
            if (pTable[i].startTime > currentJumpTime)
            {
                candidateJumpTimesList.push(pTable[i].startTime);
            }
            if (pTable[i].userDisplayCompletionTime > currentJumpTime)
            {
                candidateJumpTimesList.push(pTable[i].userDisplayCompletionTime);
            }
        }
    }

    // get times in the device table
    for (int i = 0; i < 2; i++)
    {
        if (dviceTable[i].deviceUsedCompletionTime > currentJumpTime)
        {
            candidateJumpTimesList.push(dviceTable[i].deviceUsedCompletionTime);
        }
    }

    currentJumpTime = candidateJumpTimesList.top();

    // if time in process table, get its location
    for (int i = 0; i < pTable.size(); i++)
    {
        if (pTable[i].currentLine <= pTable[i].lastLine)
        {

            if (currentJumpTime == pTable[i].startTime)
            {
                table = 0;
                row = i;
                column = 0;
            }
            else if (currentJumpTime == pTable[i].userDisplayCompletionTime)
            {
                table = 0;
                row = i;
                column = 1;
            }
        }
    }

    // if time in device table, get its location
    for (int i = 0; i < 2; i++)
    {
        if (currentJumpTime == dviceTable[i].deviceUsedCompletionTime)
        {
            table = 1;
            row = i;
            column = 10;
        }
    }

    cout << "Table: " << table << " Row: " << row << " Column: " << column << endl; // for debugging purposes

    while (candidateJumpTimesList.empty() == false)
    {

        cout << candidateJumpTimesList.top() << " ";
        candidateJumpTimesList.pop();
    }

    cout << endl << "Jump time when exiting: " << currentJumpTime << endl; // for debugging purposes
    cout << endl << "##################################" << endl;
}

void decodeScheduleNextRequest(vector<dataTableContent> &dTable, vector<processTableContent> &pTable, int procId, devices dviceTable[], queue<int> cDiskReadyQueues[])
{
    string kWord = dTable[pTable[procId].currentLine].keyword;

    if (kWord == "NEW")
    {
        cout << endl << endl << "-- ARRIVAL event for process " << procId << " at time " << currentJumpTime << " ms" << endl;
        cout << "-- Process " << procId << " starts at time " << currentJumpTime << " ms" << endl;
        pTable[procId].currentLine++;
        decodeScheduleNextRequest(dTable, pTable, procId, dviceTable, cDiskReadyQueues); // recursive

        return;
    }
    else if (kWord == "CORE")
    {
        coreDiskRequestRoutine(0, dTable[pTable[procId].currentLine].time, procId, dviceTable, cDiskReadyQueues, pTable, dTable);
    }
    else if (kWord == "DISK")
    {
        coreDiskRequestRoutine(1, dTable[pTable[procId].currentLine].time, procId, dviceTable, cDiskReadyQueues, pTable, dTable);
    }
    else if (kWord == "USER")
    {
        userRequestRoutine(dTable[pTable[procId].currentLine].time, procId, pTable);
    }
}

void displayProcessTermination(int procId, vector<processTableContent> &pTable, devices dviceTable[], vector<dataTableContent> &dTable)
{
    // This if condition fixes a bug where an extra process termination event is displayed. Specifically the last process record in process table
    if (currentJumpTime != defaultCompletionTime){
        int procIndexB = pTable[procId].firstLine;
        int procIndexE = pTable[procId].lastLine;
        int numOfDisks = 0;
        int userCalls = 0;
        string coreState = "IDLE";
        string diskState = "IDLE";

        for (int i = procIndexB; i <= procIndexE; i++)
        {
            if (dTable[i].keyword == "DISK")
            {
                numOfDisks++;
            }
            if (dTable[i].keyword == "USER")
            {
                userCalls++;
            }
        }

        // state of core and disk
        if (dviceTable[0].busy == true)
        {
            coreState = "BUSY";
        }
        if (dviceTable[1].busy == true)
        {
            diskState = "BUSY";
        }

        cout << endl << endl << endl << "Process " << procId << " TERMINATES at time " << currentJumpTime << " ms" << endl;
        cout << "Process " << procId << " ran for " << currentJumpTime - pTable[procId].startTime << " ms, "
            << "performed " << numOfDisks << " disk access(es) and requested " << userCalls << " user input(s)" << endl;

        cout << "Core is " << coreState << endl;
        cout << "Disk is " << diskState << endl;

        // Display process table

        cout << "THE PROCESS TABLE" << endl;
        pTable[procId].processState = "TERMINATED";
        cout << "Process " << procId << " started at time " << pTable[procId].startTime << " ms and is " << pTable[procId].processState << endl;

        for (int i = 0; i < pTable.size(); i++)
        {
            if (pTable[i].processState != "TERMINATED")
            {
                cout << "Process " << i << " started at time " << pTable[i].startTime << " ms and is " << pTable[i].processState << endl;
            }
        }
    }
    
  
}

// handles case when disk time is 0
void zeroDelayDiskAccess(int procId, vector<processTableContent> &pTable, devices dviceTable[], queue<int> cDiskReadyQueues[], vector<dataTableContent> &dTable)
{
    pTable[procId].userDisplayCompletionTime = defaultCompletionTime;
    cout << endl << endl << "-- Process " << procId << " performs a zero-delay disk access at time " << currentJumpTime << " ms" << endl;
    pTable[procId].currentLine++;

    // Process is done when currentline > lastline
    if (pTable[procId].currentLine > pTable[procId].lastLine)
    {
        displayProcessTermination(procId, pTable, dviceTable, dTable); 
    }
    else
    {
        decodeScheduleNextRequest(dTable, pTable, procId, dviceTable, cDiskReadyQueues); 
    }

}

string deviceString(int dviceId)
{
    if (dviceId == 0)
    {
        return "CORE";
    }
    else // 1 represents disk
    {
        return "DISK";
    }
}

string setProcessState(int dviceId)
{

    if (dviceId == 0)
    {
        return "READY";
    }
    else
    {
        return "BLOCKED";
    }
}