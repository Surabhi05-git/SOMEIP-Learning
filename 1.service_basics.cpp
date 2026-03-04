// Learing from CHATGPT

/* 
* Production-Ready = NO
* It is simplified for learning.
* Missing State Handler (Important in real systems)
* Missing Availability Handling (Not Needed For Server)
* No Error Handling
* Payload Type = Directly taken hexa value here
*/

/*
 1. This is a SOME/IP service application built using vsomeip.
 2. It does 3 big things:
 3. Creates a SOME/IP application
 4. Offers a service on the network
 5. Waits for a client request
 6. Sends a response back
*/

Client  --->  SOME/IP Network  --->  Your Service
                (UDP/TCP)

/* Your job:
 1. register service
 2. define what happens when request comes */          

#include <vsomeip/vsomeip.hpp>
#include <iostream>

#define SERVICE_ID  0x1234
#define INSTANCE_ID 0x5678
#define METHOD_ID   0x0421




std::shared_ptr<vsomeip::application> app; // This is a shared pointer to a vsomeip application object. // Global Application Pointer
                                            // vsomeip::application is the core object.
                                            // It represents your service process inside SOME/IP world.
 



                                           
//The callback funtion  - this is automatically called when user sends a request

void on_message(const std::shared_ptr<vsomeip::message> &request)  // const - means you are not allowed to modify the request object.  // EX. if (a=b) //it is a function // ex. int add(int a, int b)
{
    std::cout << "Request received" << std::endl;
    auto response = vsomeip::runtime::get()->create_response(request); 
                                                         /* Step A — Create Response
                                                         What happens here = 
                                                          1. You pass the request
                                                          2. vsomeip copies: client ID, session ID, protocol version
                                                          3. It builds proper response header
     */
    std::vector<uint8_t> payload_data = {0x10}; // step C: Fill Payload
    auto payload = vsomeip::runtime::get()->create_payload();   // Step B : Create Payload. Header is automatic. Payload is your responsibility.
    payload->set_data(payload_data); //  Now payload contains that byte.
    response->set_payload(payload);  // Now response message is complete.
    app->send(response); // Step E — Send It
}

int main() {

    app = vsomeip::runtime::get()->create_application("TemperatureService");   // This registers your process in vsomeip runtime. The string name is used for: configuration, logging, identification.

    app->init(); // loads configuration, setup routing, prepares communication

    app->register_message_handler(   // You are telling vsomeip: “When request comes matching these IDs, PASS function on_message.”
        SERVICE_ID,
        INSTANCE_ID,
        METHOD_ID,
        on_message
    );

    //You passed: on_message , NOT - on_message()
    // on_message = function address
    // on_message() = function call
    // We are giving pointer to function.
    // This is callback registration.

    app->offer_service(SERVICE_ID, INSTANCE_ID);

    app->start(); // Step 5 — Start Event Loop. This starts: worker threads, message loop, network listening
    // After this line: Program waits for messages forever.
    // Your code runs only when callback is triggered. (Event-driven architecture.)
    // After app->start(), control goes to vsomeip.
    // It is blocked forever (as long as the application is running).

    return 0;
}


/*
🔥 Execution Flow When Client Sends Request

  1. Client sends SOME/IP request
  2. vsomeip receives packet
  3. vsomeip parses header
  4. Checks service/method ID
  5. Finds your registered handler
  6. Calls on_message
  7. You create response
  8. vsomeip sends response

You only control step 6 and 7.
Everything else is middleware.*/
----------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------

/*
🧠 STEP 0 — When You Run The Program
           - OS creates a process.
           - memory is allocated
           - Global variables are created.
this line exist globaly - std::shared_ptr<vsomeip::application> app;
right now = app == null ptr
----------------------------------------------------------------------------

🧠 STEP 1 — main() Starts

A. Create vsomeip Application LINE
* What happens internally:
vsomeip::runtime::get() ----Returns singleton runtime object.
create_application("TemperatureService") -----Creates vsomeip::application object , Assigns name , Prepares internal routing structures , Returns shared_ptr
Now: app != nullptr
 ------- You now own an application object.

B. Initialize
Inside init():
Configuration file is loaded
Routing manager is connected
Network endpoints prepared
Internal thread structures created
Logging initialized

If config missing → init fails.
 --------After this: Application is ready but not running.


C. Register Message Handler

D. Offer Service
Now vsomeip:
Announces service to routing manager
Makes it discoverable
Other applications can find it
--------Now network knows:
      “Service 0x1234 instance 0x5678 is available.”
       But still nothing is running.

E. Start Event Loop -- After app->start(), control goes to vsomeip.
When app->start() runs, vsomeip roughly does this internally:

while (application_is_running)
{
    wait_for_network_message();
    if (message_received)
    {
        parse_header();
        find_matching_handler();
        call_that_handler();
    }
}
---------------Now your program is idle, waiting.
---------------main() is blocked here.

🧠 STEP 2 : NOW CLIENT SENDS A REQUEST
Client sends SOME/IP packet:
header contains :
Service ID  = 0x1234
Instance ID = 0x5678
Method ID   = 0x0421
Session ID  = something
Client ID   = something

🧠 STEP 3 — vsomeip Receives Packet
Internal worker thread:
Socket receives bytes
SOME/IP header parsed
IDs extracted
Message object created

---------Now vsomeip has: std::shared_ptr<vsomeip::message> request;
---------Filled with: Header, Payload, Client ID, Session ID

🧠 STEP 4 — Handler Lookup - 
vsomeip checks its internal handler map:   Does (0x1234,0x5678,0x0421) exist?
yes
It finds: &on_message

🧠 STEP 5 — vsomeip Calls Your Function

🧠 STEP 6 — Inside on_message()
Now execution enters:
Inside Function:

A) Create Response
auto response = vsomeip::runtime::get()->create_response(request);
What this does:
Copies client ID
Copies session ID
Sets message type to RESPONSE
Prepares header correctly
------------This ensures response goes back to correct client.

B) Create Payload
auto payload = vsomeip::runtime::get()->create_payload();
-----------Empty payload object created.

C) Fill Data
std::vector<vsomeip::byte_t> data;
data.push_back(0x42);
payload->set_data(data);
----------Now payload contains 1 byte.

D) Attach Payload
response->set_payload(payload);
Now response message is complete.
Header + payload

E) Send
app->send(response);
vsomeip:
Serializes message
Builds SOME/IP packet
Sends via socket
Routes to correct client

🧠 STEP 7 — Client Receives Response
Client receives response.
Communication complete.
Your callback ends.
Control returns to vsomeip event loop.
Program continues waiting for next request.
*/
----------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------
KEY   → (SERVICE_ID, INSTANCE_ID, METHOD_ID)
VALUE → pointer to function on_message

on_message → means “address of function”
on_message() → means “execute the function right now”