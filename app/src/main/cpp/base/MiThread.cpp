//
// Created by zz on 19-11-20.
//

#include "MiThread.h"
#include"utils.h"
static const int START_SOURCE = 1;
static const int STOP_SOURCE = 2;
static const int PAUSE_SOURCE = 3;
static const int RESUME_SOURCE = 4;

Semaphore::Semaphore(unsigned int count) :m_count(count) {
}

Semaphore::~Semaphore()
{
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> unique_lock(m_mutex);
    --m_count;
    while (m_count < 0) {
        m_condition_variable.wait(unique_lock);
    }
}

void Semaphore::signal() {
    std::lock_guard<std::mutex> lg(m_mutex);
    if (++m_count < 1) {
        m_condition_variable.notify_one();
    }
}

MiThread::MiThread():running_(false) {

}

MiThread::~MiThread() {
    if (running_) {
        Stop();
    }
}

void MiThread::Start() {
    running_ = true;
    thread_.reset(new std::thread(&MiThread::Run, this));
}

void MiThread::Stop() {
    running_ = false;
    condition_.notify_all();
    thread_->join();
}

void MiThread::Post(MessageHandler *phandler, uint32_t id,
                    MessageData *pdata, int arg0, int arg1, std::string tag) {

    Message msg;
    msg.message_id = id;
    msg.phandler = phandler;
    msg.pdata = pdata;
    msg.arg0 = arg0;
    msg.arg1 = arg1;
    msg.tag = tag;
    {
        std::unique_lock<std::mutex> lock(lock_);
        msgq_.push_back(msg);
    }
    condition_.notify_one();

}

void MiThread::Run() {
    ProcessMessages();
}

void MiThread::ProcessMessages() {
    Log("start processMessages");
    while(running_) {
        Message msg;
        bool getMsg = false;
        {
            std::unique_lock<std::mutex> lock(lock_);
            condition_.wait(lock);
            if (msgq_.empty()) {
                continue;
            }
            msg = msgq_.front();
            msgq_.pop_front();
            getMsg = true;
        }

        if (getMsg && msg.phandler)
            msg.phandler->OnMessage(&msg);
    }
    Log("thread exit");
}

