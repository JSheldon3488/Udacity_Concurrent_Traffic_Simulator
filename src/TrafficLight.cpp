#include <iostream>
#include <random>
#include <thread>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> ulock(_mutex);
    _cond.wait(ulock, [this](){ return !_queue.empty(); });
    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg;
}


template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock(_mutex);
    //Need to wipe the queue on red lights or else a build up of messages can occur and send a false green
    if (msg == TrafficLightPhase::red) {
        while(!_queue.empty()){
            _queue.pop_back();
        }
    }
    _queue.emplace_back(msg);
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true) {
        if (_messages.receive() == TrafficLightPhase::green)
            return;
    }
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
   
   // Generate Random cycleDuration for the TrafficLights
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(4000,6000);
    auto cycleDuration = uni(rng);

    // Cycle the lights and alert _message queue to process this change
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    while (true) {
        //Calculate difference between start time and current time and toggle light if it has been long enough
        int timeSinceLastChange = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
        if (timeSinceLastChange >= cycleDuration) {
            //Toggle Light
            _currentPhase = _currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red; 
            //Send message
            _messages.send(std::move(_currentPhase));
            // Wait between cyceles and reset timers
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            start = std::chrono::system_clock::now();
            cycleDuration = uni(rng);
        }
    }
}

