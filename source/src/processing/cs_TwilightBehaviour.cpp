/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 6, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_TwilightBehaviour.h>

TwilightBehaviour::TwilightBehaviour(
            uint8_t intensity,
            DayOfWeekBitMask activedaysofweek,
            TimeOfDay from, 
            TimeOfDay until) :
        Behaviour(Behaviour::type::Twilight,intensity,activedaysofweek,from,until)
    {
}
