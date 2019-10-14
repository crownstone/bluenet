# Behaviour

This document describes the Crownstones' Behaviour concept in more technical detail.

# Communication with (smartphone) applications

To store, update and sync Behaviours, the application can communicate over the Crownstone bluetooth protocol
by sending [Behaviour Packets](PROTOCOL.md#set_behaviour_packet). The API behind this packet is as follows.

The idea behind the API is that an application can only update or change the state of the stored Behaviours
if its current knowledge/model of the state is correct - (which is checked by a hash). When an operation is executed,
a `masterHash` value that is part of the Crownstone's advertised state is updated. An application can check
its current model of the behaviour store by verifying this value. (Hence it can check if it's update was accepted.)

*Question: the master hash is a bit coarse to check for updates being accepted, but it is technically unnecessary to add return values. I'm not sure which route to go here.*

```C++
/**
 * If expected_master_hash is not equal to the current
 * value that hash() would return, nothing happens
 * and 0xffffffff is returned. Else,
 * returns an index in range [0,MaxBehaviours) on succes, 
 * or 0xffffffff if it couldn't be saved.
 * 
 * A hash is computed and saved together with [b].
 */
 size_t save(Behaviour b, uint32_t expected_master_hash);

/**
 * change the behaviour at [index] to [b], if the hash of the 
 * currently saved behaviour at [index] is not equal to 
 * [expected_hash] nothing will happen and false is returned.
 * if these hashes coincide, postcondition is identical to the
 * postcondition of calling save(b) when it returns [index].
 */
bool update(Behaviour b, uint32_t expected_hash, size_t index);

/**
 * deletes the behaviour at [index], if the hash of the 
 * currently saved behaviour at [index] is not equal to 
 * [expected_hash] nothing will happen and false is returned,
 * else the behaviour is removed from storage.
 */
void remove(uint32_t expected_hash, size_t index);

/**
 *  returns the stored behaviour at [index].
 */
Behaviour state(size_t index);

/**
 * returns a map with the currently occupied indices and the 
 * behaviours at those indices.
 */
std::vector<std::pair<size_t,Behaviour>> state();

/**
 * returns the hash of the behaviour at [index]. If no behaviour
 * is stored at this index, 0xffffffff is returned.
 */
uint32_t hash(size_t index);

/**
 * returns a hash value that takes all state indices into account.
 * this value is expected to change after any call to update/save/remove.
 * 
 * A (phone) application can compute this value locally given the set of 
 * index/behaviour pairs it expects to be present on the Crownstone.
 * Checking if this differs from the one received in the crownstone state message
 * enables the application to resync.
 */
uint32_t hash();
```
