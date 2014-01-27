



//    displayService.createCharacteristic<int>()
//                  .setUUID(UUID(displayService.getUUID(), 0x125))
//                  .setName("Owner Age")
//                  .setDefaultValue(39);
//
//    displayService.createCharacteristic<float>()
//                   .setUUID(UUID(displayService.getUUID(), 0x126))
//                   .setName("Yaw")
//                   .setDefaultValue(22.2);
//
//    Characteristic<double>& pitch =
//             displayService.createCharacteristic<double>()
//                           .setUUID(UUID(displayService.getUUID(), 0x127))
//                           .setName("Pitch")
//                           .setDefaultValue(22.2);

//bool on = false;

//    Characteristic<MidiNoteMessage>& note =
//             displayService.createCharacteristic<MidiNoteMessage>()
//                           .setName("midi")
//                           .setUUID(UUID(displayService.getUUID(), 0x127))
//                           .onRead([&] {
//                                on = !on;
//                                return MidiNoteMessage()
//                                        .setNoteOn(on)
//                                        .setChannel(0)
//                                        .setNote(60) // C4
//                                        .setVelocity(100);
//                            })
//                           .setUpdateIntervalMSecs(1000)
//                           ;
