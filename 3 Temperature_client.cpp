// Client side
/*
🚀 Level 3 Learning Objectives

You will understand:
request_service() — how client asks middleware to find service
Availability handler — how client knows service is ready
create_request() — how request is formed
Session ID generation (done automatically)
Async response handling
What happens if service is NOT available
Timeout behavior
*/

/*
🧠 Now Think From Client Perspective
Client wants to send request.
But what if:
Service is not running yet?S ervice not offered yet? Network not ready?
Client cannot just blindly send request.
So client must wait for something.

🎯 The Correct Concept
Client must wait until:
Service becomes AVAILABLE.
Not registered.
Not started.
But AVAILABLE.

This is detected using: "register_availability_handler()"
*/
// his handler tells client: Service (SERVICE_ID, INSTANCE_ID) is now available.
// Only after that → client sends request. ⚠️⚠️⚠️⚠️


/*
✅ Proper Explanation

Before the client can request a service:
It must be registered with the routing manager.
The routing manager must know: This client exists, Its client ID, Its communication endpoint
That registration happens when: app_->start();
And confirmed when state becomes: ST_REGISTERED
*/


#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <memory>
#include <vector>

#define SERVICE_ID  0x1234
#define INSTANCE_ID 0x5678
#define METHOD_ID   0x0421

class TemperatureClient {
public:
    TemperatureClient() {
        app_ = vsomeip::runtime::get()->create_application("TemperatureClient");
    }

    bool init() {
        if (!app_->init()) {
            std::cerr << "Client init failed" << std::endl;
            return false;
        }

        // State handler
        app_->register_state_handler(
            [this](vsomeip::state_type_e state) {
                this->on_state(state);
            });

        // Availability handler - If the service appears → call my function. // meaning : vsomeip → call my function when service availability changes
        // LAMBDA : This is the callback function signature.
        /* Meaning vsomeip will pass 3 things when calling it. 
        1. service_t = Service ID
        2. instance_t = Instance ID
        3. bool is_available = True = available, False = unavailable */

        app_->register_availability_handler(
            SERVICE_ID,
            INSTANCE_ID,
            [this](vsomeip::service_t,
                   vsomeip::instance_t,
                   bool is_available) {
                this->on_availability(is_available);
            });

        // Message handler (for response) - If a response arrives → call my function.
        app_->register_message_handler(
            SERVICE_ID,
            INSTANCE_ID,
            METHOD_ID,
            [this](const std::shared_ptr<vsomeip::message> &response) {
                this->on_message(response);
            });

        return true;
    }

    void start() {
        app_->start();
    }

private:
    std::shared_ptr<vsomeip::application> app_;

    void on_state(vsomeip::state_type_e state) {
        if (state == vsomeip::state_type_e::ST_REGISTERED) {
            std::cout << "Client registered. Requesting service..." << std::endl;

            app_->request_service(SERVICE_ID, INSTANCE_ID);
        }
    }

    void on_availability(bool is_available) {
        if (is_available) {
            std::cout << "Service available. Sending request..." << std::endl;

            auto request = vsomeip::runtime::get()->create_request();
            request->set_service(SERVICE_ID);
            request->set_instance(INSTANCE_ID);
            request->set_method(METHOD_ID);

            app_->send(request);
        } else {
            std::cout << "Service NOT available." << std::endl;
        }
    }

    void on_message(const std::shared_ptr<vsomeip::message> &response) {
        std::cout << "Response received!" << std::endl;

        auto payload = response->get_payload();
        auto data = payload->get_data();

        if (!data.empty()) {
            std::cout << "Temperature value: "
                      << static_cast<int>(data[0])
                      << std::endl;
        }
    }
};

int main() {
    TemperatureClient client;

    if (!client.init())
        return 1;

    client.start();
    return 0;
}