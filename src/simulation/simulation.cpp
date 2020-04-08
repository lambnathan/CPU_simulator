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

    event->thread->set_ready(event->time); //set thread to ready
    scheduler->add_to_ready_queue(event->thread); //add thread to the ready queue
    //check if cpu is idle
    if(active_thread == nullptr){
        //create new dispatcher invoked event
        event_num++;
        // std::shared_ptr<SchedulingDecision> sd = scheduler->get_next_thread();
        // std::shared_ptr<Thread> next_thread = sd->thread;
        std::shared_ptr<Event> e;
        e->type = DISPATCHER_INVOKED;
        e->time = event->time;
        e->event_num = event_num;
        e->thread = nullptr;
        e->scheduling_decision = nullptr;
        //add new event to event queue
        events.push(e);
    }
    return;
}

void Simulation::handle_dispatch_completed(const std::shared_ptr<Event> event) {
    // TODO: Handle this event properly
    std::cout << "TODO: Handle dispatch completed event properly\n\n";
}

void Simulation::handle_cpu_burst_completed(const std::shared_ptr<Event> event) {
    //transition thread from RUNNING to BLOCKED 
    event->thread->set_blocked(event->time);
    //create new IO burst event and update time
    std::shared_ptr<Event> e;
    event_num++;
    e->event_num = event_num;
    e->scheduling_decision = nullptr;
    e->thread = nullptr;
    e->type = IO_BURST_COMPLETED;
    std::shared_ptr<Burst> b = event->thread->get_next_burst(IO);
    event->thread->pop_next_burst;
    if(b != nullptr){//got next io burst
        e->time = event->time + b->length;
        events.push(e);
    }
    else{//should techinally be thread completed if this occurs
        return;
    }

void Simulation::handle_io_burst_completed(const std::shared_ptr<Event> event) {
    // TODO: Handle this event properly
    std::cout << "TODO: Handle io burst completed event properly\n\n";
}

void Simulation::handle_thread_completed(const std::shared_ptr<Event> event) {
    // TODO: Handle this event properly
    std::cout << "TODO: Handle thread completed event properly\n\n";
}

void Simulation::handle_thread_preempted(const std::shared_ptr<Event> event) {
    // TODO: Handle this event properly
    std::cout << "TODO: Handle thread preempted event properly\n\n";
}

void Simulation::handle_dispatcher_invoked(const std::shared_ptr<Event> event) {
    //check if cpu is idle
    if(active_thread != nullptr){ //cpu is not idle
        //set previous thread to active thread
        prev_thread = active_thread;
    }
    //try to get next thread from the scheduling algo
    std::shared_ptr<Thread> new_thread = scheduler->get_next_thread()->thread;
    //check if we got a thread
    if(new_thread != nullptr){
        //set the active cpu thread to the new thread
        active_thread = new_thread;

        event_num++;
        std::shared_ptr<Event> e;
        e->event_num = event_num;
        e->thread = nullptr;
        e->scheduling_decision = nullptr;

        //check if the new thread is from the same process as previous thread
        if(active_thread->process_id == prev_thread->process_id){ //same parent process
            //next event will be a thread dispatch
            e->type = THREAD_DISPATCH_COMPLETED;
            e->time = event->time + thread_switch_overhead;
        }
        else{
            //next event will be a process dispatch
            e->type = PROCESS_DISPATCH_COMPLETED;
            e-> time = event->time + process_switch_overhead;
        }
        events.push(e);
        return;
    }
    else{
        return;
    }
}


//==============================================================================
// Utility methods
//==============================================================================

SystemStats Simulation::calculate_statistics() {
    // TODO: Implement functionality for calculating the simulation statistics
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
