

#include "llm.h"
#include <curl/curl.h>
#include <json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

const char* LLM_API_URL = "http://localhost:8080/v1/chat/completions";

size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*)contents, newLength);
    return newLength;
}

LLM::LLM(const std::string& url) : api_url(url), should_stop(false) {}

void LLM::stop() {
    should_stop = true;
    cv.notify_one();
}

void LLM::send_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    message_queue.push(message);
    cv.notify_one();
}

void LLM::run() {
    while (!should_stop) {
        std::string message;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return !message_queue.empty() || should_stop; });
            if (should_stop) break;
            message = message_queue.front();
            message_queue.pop();
        }

        CURL* curl = curl_easy_init();
        if (curl) {
            std::string response_string;
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            json request_data = {
                {"messages", json::array({
                    {{"role", "user"}, {"content", message}}
                })},
                {"stream", true}
            };

            std::string request_body = request_data.dump();

            curl_easy_setopt(curl, CURLOPT_URL, LLM_API_URL);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

            CURLcode res = curl_easy_perform(curl);

            if (res == CURLE_OK) {
                std::istringstream response_stream(response_string);
                std::string line;
                std::string accumulated_response;
                
                while (std::getline(response_stream, line)) {
                    // Trim leading and trailing whitespace
                    line.erase(0, line.find_first_not_of(" \n\r\t"));
                    line.erase(line.find_last_not_of(" \n\r\t") + 1);

                    if (line.empty()) continue;

                    if (line.compare(0, 6, "data: ") == 0) {
                        std::string json_str = line.substr(6);
                        
                        // Handle special case for "[DONE]"
                        if (json_str == "[DONE]") {
                            std::cout << "Received [DONE] signal" << std::endl;
                            break;
                        }

                        try {
                            json response_json = json::parse(json_str);
                            if (response_json.contains("choices") && 
                                !response_json["choices"].empty() && 
                                response_json["choices"][0].contains("delta") &&
                                response_json["choices"][0]["delta"].contains("content")) {
                                
                                std::string token = response_json["choices"][0]["delta"]["content"];
                                accumulated_response += token;

                                // Notify GUI of new token
                                if (on_token_received) {
                                    on_token_received(token);
                                }
                            }
                        } catch (json::parse_error& e) {
                            std::cerr << "JSON parse error: " << e.what() << std::endl;
                            std::cerr << "Problematic line: " << line << std::endl;
                        } catch (std::exception& e) {
                            std::cerr << "Error processing response: " << e.what() << std::endl;
                        }
                    } else {
                        std::cout << "Received non-data line: " << line << std::endl;
                    }
                }

                // Notify GUI of complete response
                if (on_response_complete) {
                    on_response_complete(accumulated_response);
                }
            } else {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
    }
}


