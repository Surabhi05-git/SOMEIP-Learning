// learining With production-style architecture
// this service register itslf with arouting manager
// offer a service, and when request arrives -> sends a temperature value.

#include <vsomeip/vsomeip.hpp>  //Main vsomeip API          // 1
#include <iostream>     //std::cout, std::cerr              // 2
#include <memory>   //std::shared_ptr                       // 3
#include <vector>   //payload container                     // 4

#define SERVICE_ID  0x1234  //identifies service type      // 5
#define INSTANCE_ID 0x5678  //identifies specific instance   // 6
#define METHOD_ID   0x0421  // identifies specific funcyion inside a service   // 7

/* 
class for - 
* Encapsulation - means access control to things by private, public
* OOP bcz - we r dealing with a real world objects, so we have to control Internal state, lifetime cycle, Behaviour. Real Projects Have Multiple Services
* State  +  Behavior  =  Object
* so instead of using globals, we r using class - Encapsulation, Lifecycle management,Clean architecture,Scalability
* 
* In vsomeip, a service has internal state (like registration state), handlers, and lifecycle management.
Using a class helps encapsulate the application object, state handler, and message handler inside one logical unit.
his avoids global variables and makes the design modular and scalable for real automotive projects.

*/

class TemperatureService {    // 8
public:                        // 11
    TemperatureService() {      // 12
        // Create the vsomeip application object inside the class
        // vsomeip::runtime::get() = returns singleton routing manager
        // create_application("TemperatureService") = create application object inside a middleware
        // NOTE :"TemperatureService" Must match the name in vsomeip.json
        app_ = vsomeip::runtime::get()->create_application("TemperatureService");  // 13
    }
   // At this point: ⚠️ Application is created , ⚠️but not intialized, ⚠️Not registered , ⚠️Not running


    bool init() {               // 14  This function prepares everything before starting.
        // A.  Initialize vsomeip middleware (load config, prepare network, Connects to routing manager)
        if (!app_->init()) {
            /* Means:
            1. First → call app_->init()
            2. Then → take its return value
            3. Then → apply ! (logical NOT)
            4. Then → check condition
          So the function app_->init() is ALWAYS executed.
          */
         // If we wrote: VOID instead of BOOL, then If initialization fails, main will not know, main will not know, start() may be called, undefined behaviour ! ⚠️ That is dangerous in production systems.
            std::cerr << "Init failed" << std::endl;
            return false;
        }

        // B. Register state handler to track service lifecycle  // 15
        // this is a callback function that vsomeip calls automatically when the application state changes
        // The Lambda : [this](vsomeip::state_type_e state) :
        // This means: Capture this class instance, call a member function on_state()
        app_->register_state_handler([this](vsomeip::state_type_e state) {
            this->on_state(state);
        });

        // 18 .  Register message handler for requests          // 18
        // when a client sends a request for this service ID + instance + methid id , then call this function
        app_->register_message_handler(
            SERVICE_ID,
            INSTANCE_ID,
            METHOD_ID,
            [this](const std::shared_ptr<vsomeip::message> &request) {
                this->on_message(request);
            }
        );

        return true;
    }

    void start() {          // 20
        // Start event loop (blocks main thread)
        app_->start();      
    }

private:                                              // 9
    std::shared_ptr<vsomeip::application> app_;  // 10 👉 It represents your application inside vsomeip middleware.

    // Called automatically when service state changes
    void on_state(vsomeip::state_type_e state) {    // 16
        std::cout << "State changed: " << state << std::endl;

        // Offer service only when routing manager confirms registration
        if (state == vsomeip::state_type_e::ST_REGISTERED) {    // 17
            std::cout << "Service is now registered. Offering..." << std::endl;
            app_->offer_service(SERVICE_ID, INSTANCE_ID);
        }
    }

    // Called automatically when a request is received
    void on_message(const std::shared_ptr<vsomeip::message> &request) {     // 19
        std::cout << "Request received" << std::endl;

        // Step A: Create response message
        auto response = vsomeip::runtime::get()->create_response(request);

        // Step B: Create payload object
        auto payload = vsomeip::runtime::get()->create_payload();

        // Step C: Fill payload (example: 1-byte temperature)
        std::vector<vsomeip::byte_t> data = {0x42}; // placeholder temperature value
        payload->set_data(data);

        // Step D: Attach payload to response
        response->set_payload(payload);

        // Step E: Send response back to client
        app_->send(response);
    }
};


// FULL EXECUTION FLOW  👉👉👉👉👉👉

int main() {
    TemperatureService service;    // Constructor runs. & application object is created.

    if (!service.init()) {
        return 1; // initialization failed
    }

    service.start(); // Event loop starts; callback handles requests
    return 0;
}




/// ⚙️  Startup Flow
/*
Constructor → create_application()
init() → app_->init() + register handlers
start() → event loop starts
ST_REGISTERED → offer_service()
*/