#include <common/cs_Types.h>
#include <localisation/cs_TrackableEvent.h>

TrackableEvent::TrackableEvent() : event_t(CS_TYPE::EVT_TRACKABLE,
		this, sizeof(TrackableEvent)) {
}
