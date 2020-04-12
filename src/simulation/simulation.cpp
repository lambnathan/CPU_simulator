#include <fstream>
#include <iostream>

#include "algorithms/fcfs/fcfs_algorithm.hpp"
#include "algorithms/rr/rr_algorithm.hpp"
#include "algorithms/priority/priority_algorithm.hpp"
#include "algorithms/mlfq/mlfq_algorithm.hpp"
#include "algorithms/custom/custom_algorithm.hpp"

#include "simulation/simulation.hpp"
#include "types/enums.hpp"

#include "utilities/flags/flags.hpp"

Simulation::Simulation(FlagOptions flags) {
    // Hello!
    if (flags.scheduler == "FCFS") {
        // Create a FCFS scheduling algorithm
        this->scheduler = std::make_shared<FCFSScheduler>();
    } else if (flags.scheduler == "RR") {
        // Create a RR scheduling algorithm
        this->scheduler = std::make_shared<RRScheduler>(flags.time_slice);
    } else if (flags.scheduler == "PRIORITY") {
        // Create a PRIORITY scheduling algorithm
        this->scheduler = std::make_shared<PRIORITYScheduler>();
    } else if (flags.scheduler == "MLFQ") {
        // Create a MLFQ scheduling algorithm
    } else if (flags.scheduler == "CUSTOM") {
        // Create a custom scheduling algorithm
    }
    this->flags = flags;
    this->logger = Logger(flags.verbose, flags.per_thread, flags.metrics);
}

void Simulation::run() {
    this->read_file(this->flags.filename);
    
    while (!this->events.empty()) {
        auto event = this->events.top();
        this->events.pop();

        // Invoke the appropriate method in the simulation for the given event type.

        switch(event->type) {
            case THREAD_ARRIVED:
                this->handle_thread_arrived(event);
                break;

            case THREAD_DISPATCH_COMPLETED:
            case PROCESS_DISPATCH_COMPLETED:
                this->handle_dispatch_completed(event);
                break;

            case CPU_BURST_COMPLETED:
                this->handle_cpu_burst_completed(event);
                break;

            case IO_BURST_COMPLETED:
                this->handle_io_burst_completed(event);
                break;
            case THREAD_COMPLETED:
                this->handle_thread_completed(event);
                break;

            case THREAD_PREEMPTED:
                this->handle_thread_preempted(event);
                break;

            case DISPATCHER_INVOKED:
                this->handle_dispatcher_invoked(event);
                break;
        }

        // If this event triggered a state change, print it out.
        if (event->thread && event->thread->current_state != event->thread->previous_state) {
            this->logger.print_state_transition(event, event->thread->previous_state, event->thread->current_state);
        }
        this->system_stats.total_time = event->time;
        event.reset();
    }
    // We are done!

    std::cout << "SIMULATION COMPLETED!\n\n";

    for (auto entry: this->processes) {
        this->logger.print_per_thread_metrics(entry.second);
    }

    logger.print_simulation_metrics(this->calculate_statistics());
}

//==============================================================================
// Event-handling methods
//==============================================================================

void Simulation::handle_thread_arrived(const std::shared_ptr<Event> event) {
    event->thread->arrival_time = event->time; //set the thread's arrival time
    event->thread->set_ready(event->time); //set thread to ready
    scheduler->add_to_ready_queue(event->thread); //add thread to the ready queue
    //check if cpu is idle
    if(active_thread == nullptr){
        //create new dispatcher invoked event
        event_num++;
        std::shared_ptr<Event> e = std::make_shared<Event>(DISPATCHER_INVOKED, event->time, event_num, nullptr, nullptr);
        //add new event to event queue
        events.push(e);
    }
    return;
}

void Simulation::dispatch_completed_helper(const std::shared_ptr<Event> event, const std::shared_ptr<Burst> b){
    event_num++;
    if(event->thread->bursts.size() == 0){
        //no more bursts for this thread, create THREAD_FINISHED event
        std::shared_ptr<Event> e = std::make_shared<Event>(THREAD_COMPLETED, event->time + b->length, event_num, event->thread, event->scheduling_decision);
        events.push(e);
    }
    else{
        //still more bursts, create cpu burst completed event
        std::shared_ptr<Event> e = std::make_shared<Event>(CPU_BURST_COMPLETED, event->time + b->length, event_num, event->thread, event->scheduling_decision);
        events.push(e);
    }
    //update time spent on CPU
    event->thread->service_time += b->length;
}

void Simulation::handle_dispatch_completed(const std::shared_ptr<Event> event) {
    //thread transitions from READY to RUNNING
    if(event->thread->previous_state == NEW){ //first time thread starts running, set start time
        event->thread->start_time = event->time;
    }
    event->thread->set_running(event->time);
    //check if using a preemptive algorithm
    if(flags.scheduler != "FCFS"){ //is a preemptive algo
        std::shared_ptr<Burst> current_burst = event->thread->get_next_burst(CPU);
        if(current_burst->length - event->scheduling_decision->time_slice <= 0){//can finish the burst
            event->thread->pop_next_burst(CPU);
            dispatch_completed_helper(event, current_burst);
            return;
        }
        else{//cant finish the burst, so preemp it
            event_num++;
            std::shared_ptr<Event> e = std::make_shared<Event>(THREAD_PREEMPTED, event->time + event->scheduling_decision->time_slice,
                event_num, event->thread, event->scheduling_decision);
            events.push(e);
            //update time on CPU it was able to spend
            event->thread->service_time += event->scheduling_decision->time_slice;
            return;
        }
    }
    else{ //non-preemptive, so complete a cpu burst
        std::shared_ptr<Burst> b = event->thread->get_next_burst(CPU);
        event->thread->pop_next_burst(CPU);
        dispatch_completed_helper(event, b);
        return;
    }
}

void Simulation::handle_cpu_burst_completed(const std::shared_ptr<Event> event) {
    //transition thread from RUNNING to BLOCKED 
    event->thread->set_blocked(event->time);
    prev_thread = active_thread;
    active_thread = nullptr; //thread is blocked, so is not active
    //create new IO burst event and update time
    event_num++;
    std::shared_ptr<Burst> b = event->thread->get_next_burst(IO);
    event->thread->pop_next_burst(IO);
    if(b != nullptr){//got next io burst
        //also make new dispatcher invoked event, since cpu burst just completed
        std::shared_ptr<Event> di = std::make_shared<Event>(DISPATCHER_INVOKED, event->time, event_num, nullptr, nullptr);
        events.push(di);
        event_num++;
        std::shared_ptr<Event> e = std::make_shared<Event>(IO_BURST_COMPLETED, event->time + b->length, event_num, event->thread, event->scheduling_decision);
        events.push(e);
        //update time spent on IO
        event->thread->io_time += b->length;
    }
    else{//should techinally be thread completed if this occurs
        return;
    }
}

void Simulation::handle_io_burst_completed(const std::shared_ptr<Event> event) {
    //thread transitions from blocked to ready
    event->thread->set_ready(event->time);
    //put thread back in ready queue
    scheduler->add_to_ready_queue(event->thread);

    if(active_thread == nullptr){
        //create new dispatcher invoked event
        event_num++;
        std::shared_ptr<Event> e = std::make_shared<Event>(DISPATCHER_INVOKED, event->time, event_num, nullptr, nullptr);
        events.push(e);
    }
    return;
}

void Simulation::handle_thread_completed(const std::shared_ptr<Event> event) {
    //transition from RUNNING TO EXIT
    event->thread->set_finished(event->time);
    //update thread end time
    event->thread->end_time = event->time;

    prev_thread = active_thread;
    active_thread = nullptr;

    //create new dispatcher invoked event
    if(active_thread == nullptr){
        event_num++;
        std::shared_ptr<Event> e = std::make_shared<Event>(DISPATCHER_INVOKED, event->time, event_num, nullptr, nullptr);
        events.push(e);
    }
}

void Simulation::handle_thread_preempted(const std::shared_ptr<Event> event) {
    //set status of current thread from running to ready
    event->thread->set_ready(event->time);
    //update remaining burst time of thread
    std::shared_ptr<Burst> current_burst = event->thread->get_next_burst(CPU);
    current_burst->update_time(event->scheduling_decision->time_slice);
    //save current status of thread and add to back of thread queue
    scheduler->add_to_ready_queue(event->thread);
    //create new DISPATCHER_INVOKED event
    event_num++;
    std::shared_ptr<Event> e = std::make_shared<Event>(DISPATCHER_INVOKED, event->time, event_num, nullptr, nullptr);
    events.push(e);
}

void Simulation::handle_dispatcher_invoked(const std::shared_ptr<Event> event) {
    //check if cpu is idle
    if(active_thread != nullptr){ //cpu is not idle
        //set previous thread to active thread
        prev_thread = active_thread;
    }
    //try to get the next thread from the scheduling algo
    std::shared_ptr<SchedulingDecision> sd = scheduler->get_next_thread();

    //check if we got a thread
    if(sd->thread != nullptr){
        this->logger.print_verbose(event, sd->thread, sd->explanation);

        //set the active cpu thread to the new thread
        active_thread = sd->thread;
        event_num++;
        //check if the new thread is from the same process as previous thread
        if(prev_thread != nullptr && active_thread->process_id == prev_thread->process_id){ //same parent process
            //next event will be a thread dispatch
            std::shared_ptr<Event> e = std::make_shared<Event>(THREAD_DISPATCH_COMPLETED, event->time + thread_switch_overhead,
                 event_num, active_thread, sd);
            events.push(e);
            this->system_stats.dispatch_time += thread_switch_overhead;
        }
        else{
            //next event will be a process dispatch
            std::shared_ptr<Event> e = std::make_shared<Event>(PROCESS_DISPATCH_COMPLETED, event->time + process_switch_overhead,
                 event_num, active_thread, sd);
            events.push(e);
            this->system_stats.dispatch_time += process_switch_overhead;
        }
        return;
    }
    else{
        //set cpu to idle
        active_thread = nullptr;
        return;
    }
}


//==============================================================================
// Utility methods
//==============================================================================

SystemStats Simulation::calculate_statistics() {
    //Implement functionality for calculating the simulation statistics
    //loop over every process, then loop over every thread of each process and get stats that were stored in each thread
    std::vector<int> total_repsonse_times = {0, 0, 0, 0};
    std::vector<int> total_turnaround_times = {0, 0, 0, 0};
    for(auto it = processes.begin(); it != processes.end(); it++){
        std::shared_ptr<Process> p = it->second; //get process
        for(int i = 0; i < p->threads.size(); i++){ //go over each thread in process
            std::shared_ptr<Thread> thread = p->threads[i];
            this->system_stats.io_time += thread->io_time; //update io time
            this->system_stats.service_time += thread->service_time; //update CPU time
            this->system_stats.total_cpu_time = this->system_stats.service_time; //service time same as total cpu time?
            if(this->system_stats.total_time < thread->end_time){
                this->system_stats.total_time = thread->end_time;
            }
            //update thread type counts
            this->system_stats.thread_counts[thread->priority]++;

            //add to total repsonse times and turnaround times
            total_repsonse_times[thread->priority] += thread->response_time();
            total_turnaround_times[thread->priority] += thread->turnaround_time();
        }
    }
    for(int i = 0; i < 4; i++){
        if(this->system_stats.thread_counts[i] != 0){
            this->system_stats.avg_thread_response_times[i] = (double)total_repsonse_times[i] / this->system_stats.thread_counts[i];
            this->system_stats.avg_thread_turnaround_times[i] = (double)total_turnaround_times[i] / this->system_stats.thread_counts[i];
        }
    }
    this->system_stats.total_idle_time = this->system_stats.total_time - this->system_stats.service_time - this->system_stats.dispatch_time;
    this->system_stats.cpu_utilization = ((double)(this->system_stats.dispatch_time + this->system_stats.service_time) / this->system_stats.total_time) * 100;
    this->system_stats.cpu_efficiency = ((double)this->system_stats.service_time / this->system_stats.total_time) * 100;
    return this->system_stats;
}

void Simulation::add_event(std::shared_ptr<Event> event) {
    if (event != nullptr) {
        this->events.push(event);
    }
}

void Simulation::read_file(const std::string filename) {
    std::ifstream input_file(filename.c_str());

    if (!input_file) {
        std::cerr << "Unable to open simulation file: " << filename << std::endl;
        throw(std::logic_error("Bad file."));
    }

    int num_processes;

    input_file >> num_processes >> this->thread_switch_overhead >> this->process_switch_overhead;

    for (int proc = 0; proc < num_processes; ++proc) {
        auto process = read_process(input_file);

        this->processes[process->process_id] = process;
    }
}

std::shared_ptr<Process> Simulation::read_process(std::istream& input) {
    int process_id, priority;
    int num_threads;

    input >> process_id >> priority >> num_threads;

    auto process = std::make_shared<Process>(process_id, (ProcessPriority) priority);

    // iterate over the threads
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        process->threads.emplace_back(read_thread(input, thread_id, process_id, (ProcessPriority) priority));
    }

    return process;
}

std::shared_ptr<Thread> Simulation::read_thread(std::istream& input, int thread_id, int process_id, ProcessPriority priority) {
    // Stuff
    int arrival_time;
    int num_cpu_bursts;

    input >> arrival_time >> num_cpu_bursts;

    auto thread = std::make_shared<Thread>(arrival_time, thread_id, process_id, priority);

    for (int n = 0, burst_length; n < num_cpu_bursts * 2 - 1; ++n) {
        input >> burst_length;

        BurstType burst_type = (n % 2 == 0) ? BurstType::CPU : BurstType::IO;

        thread->bursts.push(std::make_shared<Burst>(burst_type, burst_length));
    }

    this->events.push(std::make_shared<Event>(EventType::THREAD_ARRIVED, thread->arrival_time, this->event_num, thread, nullptr));
    this->event_num++;

    return thread;
}
