/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 31, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <logging/cs_Logger.h>

#include <util/cs_Coroutine.h>

#define LOGCoroutineDebug LOGd

void Coroutine::startSingleMs(uint32_t ms) {
	_nextCallTickcount = delayMs(ms);
	LOGCoroutineDebug("coroutine start single");
	_mode = Mode::StartedSingle;
}

void Coroutine::startSingleS(uint32_t s) {
	startSingleMs(s*1000);
}

void Coroutine::startRepeatMs(uint32_t ms) {
	_nextCallTickcount = delayMs(ms);
	LOGCoroutineDebug("coroutine start repeat");
	_mode = Mode::StartedRepeat;
}

void Coroutine::startRepeatS(uint32_t s) {
	startRepeatMs(s*1000);
}

void Coroutine::pause() {
	LOGCoroutineDebug("coroutine pause");
	_mode = Mode::Paused;
}

void Coroutine::onTick(uint32_t currentTickCount) {
	switch(_mode) {
		case Mode::Repeat: {
			if (shouldRunAction(currentTickCount)) {
				auto ticksToWait = _action();
				_nextCallTickcount = currentTickCount + ticksToWait;
			}
			return;
		}
		case Mode::Single: {
			if (shouldRunAction(currentTickCount)) {
				_mode = Mode::Paused; // must be done before action() to prevent overwriting it
				_action();
			}
			return;
		}
		case Mode::Paused: {
			return;
		}
		case Mode::StartedSingle: {
			// _nextCallTickcount stored the intended delay after StartedSingle.
			// now we know the current tick count, so update it an progress to
			// mode single.
			_nextCallTickcount = currentTickCount + _nextCallTickcount;
			_mode = Mode::Single;
			return;
		}
		case Mode::StartedRepeat: {
			// _nextCallTickcount stored the intended delay after StartedSingle.
			// now we know the current tick count, so update it an progress to
			// mode repeat.
			_nextCallTickcount = currentTickCount + _nextCallTickcount;
			_mode = Mode::Repeat;
			return;
		}
	}
}



bool Coroutine::handleEvent(event_t& evt) {
	if (evt.type == CS_TYPE::EVT_TICK) {
		this->onTick(*reinterpret_cast<TYPIFY(EVT_TICK)*>(evt.data));
		return true;
	}
	return false;
}


bool Coroutine::shouldRunAction(uint32_t currentTickCount) {
		return currentTickCount >= _nextCallTickcount && _action;
	}
