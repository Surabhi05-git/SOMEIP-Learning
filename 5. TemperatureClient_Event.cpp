#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <memory>

#define SERVICE_ID     0x1234
#define INSTANCE_ID    0x5678
#define EVENT_ID       0x4465
#define EVENTGROUP_ID  0x01

class TemperatureClient {

public:

    TemperatureClient() {
        app_ = vsomeip::runtime::get()->create_application("TemperatureClient");
    }

    bool init() {

        if(!app_->init())
            return false;

         //Called whenever the client state changes.
         //Helps detect ST_REGISTERED, after which you can request services.
        app_->register_state_handler(
            [this](vsomeip::state_type_e state){
                on_state(state);
            });

        app_->register_availability_handler(
            SERVICE_ID,
            INSTANCE_ID,
            [this](vsomeip::service_t, vsomeip::instance_t, bool is_available){
                on_availability(is_available);
            });

        app_->register_message_handler(
            SERVICE_ID,
            INSTANCE_ID,
            EVENT_ID,
            [this](const std::shared_ptr<vsomeip::message> &msg){
                on_message(msg);
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

            app_->request_service(SERVICE_ID, INSTANCE_ID);
        }
    }

    void on_availability(bool available) {

        if(available) {

            std::cout<<"Service available\n";

            app_->request_event(
                SERVICE_ID,
                INSTANCE_ID,
                EVENT_ID,
                {EVENTGROUP_ID}
            );

            app_->subscribe(
                SERVICE_ID,
                INSTANCE_ID,
                EVENTGROUP_ID
            );
        }
    }

    void on_message(const std::shared_ptr<vsomeip::message> &msg) {

        auto payload = msg->get_payload();
        auto data = payload->get_data();

        std::cout<<"Temperature Event Received: "
                 <<(int)data[0]<<std::endl;
    }
};

int main() {

    TemperatureClient client;

    if(!client.init())
        return 1;

    client.start();

    return 0;
}