#include <cassert>
#include <cstddef>
#include <stdexcept>
#include "types/thread/thread.hpp"

void Thread::set_ready(int time) {
    if(current_state == NEW || current_state == BLOCKED || current_state == RUNNING){
        set_state(READY, time);
    }
    else{
        printf("Error: Ivalid transitionn\n");
        exit(1); 
    }
}

void Thread::set_running(int time) {
    if(current_state == READY){
        set_state(RUNNING, time);
    }
    else{
        printf("Error: Ivalid transitionn\n");
        exit(1); 
    }
}

void Thread::set_blocked(int time) {
    if(current_state == READY || current_state == RUNNING){
        set_state(BLOCKED, time);
    }
    else{
        printf("Error: Ivalid transitionn\n");
        exit(1);
    }
}

void Thread::set_finished(int time) {
    if(current_state == RUNNING){
        set_state(EXIT, time);
    }
    else{
        printf("Error: Ivalid transitionn\n");
        exit(1);
    }
}

int Thread::response_time() const {
    return start_time - arrival_time;
}

int Thread::turnaround_time() const {
    return end_time - arrival_time;
}

void Thread::set_state(ThreadState state, int time) {
    state_change_time = time;
    previous_state = current_state;
    current_state = state;
}

std::shared_ptr<Burst> Thread::get_next_burst(BurstType type) {
    // TODO
    int num_bursts = bursts.size();
    int i = 0;
    while(i != num_bursts){
        if(bursts.front()->burst_type == type){
            return bursts.front();
        }
        else{
            std::shared_ptr<Burst> b = bursts.front();
            bursts.pop();
            bursts.push(b);
            i++;
        }
    }
    //if cant find burst of correct type, return nullptr
    return nullptr;

}

std::shared_ptr<Burst> Thread::pop_next_burst(BurstType type) {
    int num_bursts = bursts.size();
    int i = 0;
    while(i != num_bursts){
        if(bursts.front()->burst_type == type){
            std::shared_ptr<Burst> b = bursts.front();
            bursts.pop();
            return b;
        }
        else{
            std::shared_ptr<Burst> b = bursts.front();
            bursts.pop();
            bursts.push(b);
            i++;
        }
    }
    return nullptr;
}