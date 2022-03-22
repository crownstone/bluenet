#include <iostream>

#include <events/cs_Event.h>
#include <events/cs_EventDispatcher.h>
#include <events/cs_EventListener.h>

#include <util/cs_Error.h>
#include <typeinfo>



class TestListener : public EventListener {
	private:
	bool _receivedEvent = false;
	std::string _name = "";
	public:

	TestListener(std::string name) : _name(name) {}
	virtual void handleEvent(event_t & event) override {
		printf("handle event in %s\n", _name.c_str());
		_receivedEvent = true;
	}

	bool receivedEvent () {
		return _receivedEvent;
	}
};

int main() {
	TestListener a("a"), b("b");

	EventDispatcher& dispatcher = EventDispatcher::getInstance();

	dispatcher.addListener(&a);
	dispatcher.addListener(&b);
	event_t evt(CS_TYPE::EVT_GENERIC_TEST);
	evt.dispatch();

	assert(a.receivedEvent(), "test listener a didn't receive anything");
	assert(b.receivedEvent(), "test listener b didn't receive anything");

	return 0;
}
