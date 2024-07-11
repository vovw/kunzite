#ifndef LLM_H
#define LLM_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class LLM {
public:
    LLM(const std::string& url = "http://localhost:8080/completion");
    void stop();
    void send_message(const std::string& message);
    void run();

    std::function<void(const std::string&)> on_token_received;
    std::function<void(const std::string&)> on_response_complete;

private:
    std::string api_url;
    std::queue<std::string> message_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool should_stop;
};

#endif // LLM_H
