#ifndef MFLQ_ALGORITHM_HPP
#define MFLQ_ALGORITHM_HPP

#include <memory>
#include "algorithms/scheduling_algorithm.hpp"

/*
    MFLQScheduler:
        A representation of a multi-level feedback queue scheduling algorithm.

        You are free to add any member functions or member variables that you
        feel are helpful for implementing the algorithm.
*/

class MFLQScheduler : public Scheduler {
public:
    
    //==================================================
    //  Member variables
    //==================================================

    // Add any member variables you may need.
    std::queue<std::shared_ptr<Thread>> queue_0;
    std::queue<std::shared_ptr<Thread>> queue_1;
    std::queue<std::shared_ptr<Thread>> queue_2;
    std::queue<std::shared_ptr<Thread>> queue_3;
    std::queue<std::shared_ptr<Thread>> queue_4;
    std::queue<std::shared_ptr<Thread>> queue_5;
    std::queue<std::shared_ptr<Thread>> queue_6;
    std::queue<std::shared_ptr<Thread>> queue_7;
    std::queue<std::shared_ptr<Thread>> queue_8;
    std::queue<std::shared_ptr<Thread>> queue_9;


    //==================================================
    //  Member functions
    //==================================================

    MFLQScheduler(int slice = -1);

    std::shared_ptr<SchedulingDecision> get_next_thread();

    void add_to_ready_queue(std::shared_ptr<Thread> thread);

    size_t size() const;

};

#endif
