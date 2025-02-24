//
// main.cpp
// CS3360_Code02_Process
//
// Created by Darren Bowers on 1/30/2025, following Professor Manouchehr Mohandesi's examples.
//

#include <iostream>
#include <thread>
#include <vector>
#include <chrono> // Timing
#include <mutex> // Access to shared data in the memory between processess
#include <memory>

using namespace std; 

mutex coutMutex; // Because several methods were printing and calling each other I added a global mutex to prevent overlap printing. 

// enum to determine the processes current state
enum ProcessState {READY, RUNNING, WAITING, BLOCKING, TERMINATED};
enum ProcessCreator {USER,KERNAL, PROCESS}; // I added an additional enum in order to reduce rundant code in the createProcess methods, so now each type of creator calls one helper method and identifies the type with the enum. 

// Model the process class
class Process {
private: 
    int processID; // unique identifier to recongize the process
    ProcessState state; // State comes from new enum data type defined
    int cpuUsage; 
    int resourceUsage; // Such as memory or I/O parts
    mutex mtx; // Prevents conflicts between processes that use shared data

public:
    // Contstructor
    Process(int id): processID(id), state(READY), cpuUsage(0), resourceUsage(0){
    }

    // C++ 11 constructor - I didn't need this but I wanted to keep it for reference if I need it in the future. 
    /* Process(Process&& other) noexcept: processID(other.processID), state(other.state), cpuUsage(other.cpuUsage), resourceUsage(other.resourceUsage){
    };

    // C++ 11 assignment operator
    Process& operator = (Process&& other) noexcept{
        if(this != &other){
            processID = other.processID;
            state = other.state;
            cpuUsage = other.cpuUsage;
            resourceUsage = other.resourceUsage;
        }
        return *this;
    }; */

    // Get the current state of this process
    ProcessState getState() const{
        return state;
    }

    // Set method for process state
    void setState(ProcessState newState){
        {
            lock_guard<mutex> lock(mtx); // Locks the shared data
            state = newState;
        }
        {
        lock_guard<mutex> lock(coutMutex); // First use in the coe of the global mutex, used because when getStateAsString was called, other prints were injecting their prints in this one. 
        cout << "Process " << processID << " State Changed to: " << getStateAsString() << endl;
        }
    }
    

    // Get method for process ID
    int getProcessID() const { // Marked const to allow calling on const objects 
        return processID;
    }

    // Simulate the process is working
    void run(){ // Set state to running
        setState(RUNNING);

        // Simulate cpu usage
        for (int i=0; i<5; i++){
            this_thread::sleep_for(chrono::milliseconds(500));
            { // Blocked in order to be considered as a singular line of code and prevent usage while this line is running
                // Protect the shared data by mutex
                lock_guard<mutex> lock(mtx);
                cpuUsage+=10;
            }
            {
                lock_guard<mutex> lock(coutMutex);
                cout << "Process " << processID << " is running. CPU usage: " << cpuUsage << endl;
            }
        }

        // Simulate waiting state
        setState(WAITING);
        this_thread::sleep_for(chrono::milliseconds(1000));

        // Simulate return to running mode

        setState(RUNNING); 
        this_thread::sleep_for(chrono::milliseconds(1000));

        // Simulate Terminate process
        terminate();
    }

    // Custom Terminate process
    void terminate(){
    setState(TERMINATED);
    {
        lock_guard<mutex> lock(coutMutex);
        cout<<"Process "<< processID <<" Terminated!\n";
    }
    }
            
    // Return String that represents the process state
    string getStateAsString() const{
            switch(state){
            case READY: return "READY";
            case RUNNING: return "RUNNING";
            case WAITING: return "WAITING";
            case BLOCKING: return "BLOCKING";
            case TERMINATED: return "TERMINATED";
            default: return "UNKNOWN";
        }
    }

    // Disable copy constructor and copy assignment (process cannot be copyable)
        Process( const Process&) = delete;
        Process& operator = (const Process&) = delete; 
};

// Seperate thread managment for each process
class ProcessTheadManager{
    private:
        thread processThread; 
    public: 
        // Attach a process to thread and run it
        void runProcess(Process& process){
            processThread = thread(&Process::run, &process);
        }
        // wait for thread to finish
        void join(){
            if(processThread.joinable()){
                processThread.join();
            }
        }
};

class ProcessManagment{
    private:
        vector<unique_ptr<Process>> processes; // List of Processes
        vector<ProcessTheadManager> threadManager; // Manage threads for processess
        mutex processMutex; // Mutex for process manager (shared data)

        void createProcess(ProcessCreator type, int parentID = 0){ // Helper method called by the other createProcess methods, used to reduce redunant/repeated code. 
            {
                lock_guard<mutex> lock(processMutex);
                int id=processes.size()+1;
                processes.emplace_back(make_unique<Process>(id));
                threadManager.emplace_back(); // add thread manager for new processes
                switch(type){
                    case USER: cout << "Process " << id << " created by User." << endl;
                    break;
                    case KERNAL: cout << "Process " << id << " created by Kernal." << endl;
                    break;
                    case PROCESS: cout << "Process " << id << " created by Process " << parentID << "." << endl;
                    break;
                    default: cout << "Unknown creation!" << endl;
                    break;
                }
                threadManager.back().runProcess(*processes.back()); // run the process in a new thread --- I moved this after the switch because runProcess was causing prints to inject themselves before the switch print was done
            }
        }
    
    public:
        // Create process by user
        void createProcessByUser(){ 
            createProcess(USER);
        }
        // Create a process by another process
        void createProcessByProcess(int parentID){
            createProcess(PROCESS, parentID);
        }
        // Create process by Kernal
        void createProcessByKernal(){
            createProcess(KERNAL);
        }

    // wait for all processess to finish
    void waitForAllProccesses(){
        for (auto& manager: threadManager){
            manager.join(); // wait for each thread to finish
        }
    }
    // Show status of all processes
    void showProcessStatus(){
        {
            lock_guard<mutex> lock(processMutex);
            for (const auto& process: processes){
                cout << "Process " << process->getProcessID() << ": " << process->getStateAsString() << endl;
            }
        }
    }
};

int main(){
    // pm object following the process management structure 
    ProcessManagment pm;

    // User creates new a process
    pm.createProcessByUser();

    // Kernal creates a new process
    pm.createProcessByKernal();

    // Process creates a new process
    pm.createProcessByProcess(1);

    // Waiting for all processes to complete
    pm.waitForAllProccesses();

    // Display the status of processess
    pm.showProcessStatus();

    return 0;
}

