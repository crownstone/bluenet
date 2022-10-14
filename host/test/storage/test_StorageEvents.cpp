/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 14, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <events/cs_EventListener.h>
#include <testaccess/cs_Storage.h>

class TestListener : public EventListener {
	CS_TYPE _expectedEvent                   = CS_TYPE::CONFIG_DO_NOT_USE;
	std::vector<CS_TYPE> _receivedEventTypes = {};

public:
	void setExpect(CS_TYPE expect) {
		_expectedEvent = expect;
		_receivedEventTypes.clear();
	}

	bool receivedExpectedEvent() {
		using namespace std;
		auto& v = _receivedEventTypes;
		return find(begin(v), end(v), _expectedEvent) != end(v);
	}

	bool receivedOtherEvents() {
		if (receivedExpectedEvent()) {
			return _receivedEventTypes.size() > 1;
		}
		else {
			return _receivedEventTypes.size() > 0;
		}
	}

	void handleEvent(event_t& event) { _receivedEventTypes.push_back(event.type); }
};


bool checkEvent(TestListener& listener, TestAccess<Storage>& tester, fds_evt_id_t fdsEvent, CS_TYPE csEvent) {
	listener.setExpect(csEvent);
	fds_evt_t writeEvent{.id = fdsEvent, .result = ERR_SUCCESS};
	tester.emulateStorageEvent(writeEvent);

	if (!listener.receivedExpectedEvent()) {
		LOGw("did not receive %u after emulating %u",
				static_cast<uint16_t>(csEvent), static_cast<uint16_t>(fdsEvent));
		return false;
	}

	if (listener.receivedOtherEvents()) {
		LOGw("received other events besides EVT_STORAGE_WRITE_DONE");
		return false;
	}

	LOGd("all good");
	return true;
}

int main() {

	Storage& storage = Storage::getInstance();
	TestAccess<Storage> tester(storage);

	TestListener listener;
	listener.listen();
	storage.init();

	if (!checkEvent(listener, tester, FDS_EVT_WRITE, CS_TYPE::EVT_STORAGE_WRITE_DONE)) { return 1; }
	if (!checkEvent(listener, tester, FDS_EVT_UPDATE, CS_TYPE::EVT_STORAGE_WRITE_DONE)) { return 1; }
	if (!checkEvent(listener, tester, FDS_EVT_DEL_RECORD, CS_TYPE::EVT_STORAGE_REMOVE_DONE)) { return 1; }
	if (!checkEvent(listener, tester, FDS_EVT_DEL_FILE, CS_TYPE::EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE)) { return 1; }
	if (!checkEvent(listener, tester, FDS_EVT_INIT, CS_TYPE::EVT_STORAGE_INITIALIZED)) { return 1; }

	// FDS_EVT_GC result depends on internal state of storage.
	if (!checkEvent(listener, tester, FDS_EVT_GC, CS_TYPE::EVT_STORAGE_GC_DONE)) { return 1; }
	storage.factoryReset();
	if (!checkEvent(listener, tester, FDS_EVT_GC, CS_TYPE::EVT_STORAGE_FACTORY_RESET_DONE)) { return 1; }


	return 0;
}
