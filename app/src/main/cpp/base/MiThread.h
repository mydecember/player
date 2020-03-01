//
// Created by zz on 19-11-20.
//

#ifndef DEXCAM_MEDIASOURCE_H
#define DEXCAM_MEDIASOURCE_H
#include <stdint.h>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>
#include <deque>
#include "list"
#include <condition_variable>

struct Message;
class MessageData
{
public:
    MessageData() {}
    virtual ~MessageData() {}
};

class MessageHandler
{
public:
    virtual ~MessageHandler(){}
    virtual void OnMessage(Message* msg) = 0;

protected:
    MessageHandler() {}

private:
};


struct Message {
    uint32_t message_id;
    MessageHandler *phandler;
    MessageData *pdata;
    int arg0;
    int arg1;
    std::string tag;
    Message():phandler(NULL), arg0(0), arg1(0){}
};

class Semaphore
{
public:
    explicit Semaphore(unsigned int count = 0);
    ~Semaphore();

public:
    void wait();
    void signal();
private:
    int m_count;
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
};

class MiThread {
public:
    MiThread();
    ~MiThread();
    void Start();
    void Stop();
    virtual void Post(MessageHandler *phandler, uint32_t id = 0,
                      MessageData *pdata = NULL, int arg0 = 0 , int arg1 = 0, std::string tag = "");
private:
    void Run();
    void ProcessMessages();

private:
    std::unique_ptr<std::thread>  thread_;
    std::deque<Message> msgq_;
    std::mutex lock_;
    std::condition_variable condition_;
    bool running_;
};


#endif //DEXCAM_MEDIASOURCE_H
