#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#define SERVICE_ID     0x1234
#define INSTANCE_ID    0x5678
#define EVENT_ID       0x4465
#define EVENTGROUP_ID  0x01

class TemperatureService {

public:

    TemperatureService() {
        app_ = vsomeip::runtime::get()->create_application("TemperatureService");
    }

    bool init() {

        if (!app_->init()) {
            std::cerr << "Init failed\n";
            return false;
        }

        app_->register_state_handler(
            [this](vsomeip::state_type_e state){
                on_state(state);
            });

        return true;
    }

    void start() {
        app_->start();
    }

private:

    std::shared_ptr<vsomeip::application> app_;

    void on_state(vsomeip::state_type_e state) {

        if(state == vsomeip::state_type_e::ST_REGISTERED) {

            std::cout<<"Service Registered\n";

            app_->offer_service(SERVICE_ID, INSTANCE_ID);   // This announces your service to the routing manager.
            
            //SOME/IP events are grouped for better management.
            // Here, we create a set of event groups containing our EVENTGROUP_ID (here only 1).
            // Event groups are useful when a client wants to subscribe to multiple events together.
            std::set<vsomeip::eventgroup_t> event_groups;
            event_groups.insert(EVENTGROUP_ID);

            
            //This declares an event that your service can publish.
            // Parameters:
            //SERVICE_ID, INSTANCE_ID → identify which service offers this event
            // EVENT_ID → specific event (e.g., temperature update)
            //event_groups → groups this event belongs to
            //ET_EVENT → type of event (ET_EVENT = standard publish/subscribe event)
            //After this, clients can subscribe to this event to receive notifications.
            app_->offer_event(
                SERVICE_ID,
                INSTANCE_ID,
                EVENT_ID,
                event_groups,
                vsomeip::event_type_e::ET_EVENT
            );

            start_publishing();
        }
    }

    void start_publishing() {

        std::thread([this](){

            while(true) {

                uint8_t temperature = 25;

                std::vector<vsomeip::byte_t> payload_data = {temperature};

                auto payload = vsomeip::runtime::get()->create_payload();
                payload->set_data(payload_data);

                app_->notify(
                    SERVICE_ID,
                    INSTANCE_ID,
                    EVENT_ID,
                    payload
                );

                std::cout<<"Temperature Event Sent\n";

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

        }).detach();
    }
};

int main() {

    TemperatureService service;

    if(!service.init())
        return 1;

    service.start();

    return 0;
}