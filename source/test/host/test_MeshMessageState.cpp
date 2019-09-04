#include <protocol/mesh/cs_MeshMessageState.h>

#include <iostream>

using namespace std;

int main() {

	cout << "Test MeshMessageState implementation" << endl;
	cout << "MAX_STATE_ITEMS: " << MAX_STATE_ITEMS << endl;

	state_message_t smt;

	state_message sm(smt);

	sm.clear();

//	stone_id_t id;
//	uint8_t   switchState;
//	uint8_t   flags;
//	int8_t    temperature;
//	int8_t    powerFactor;
//	int16_t   powerUsageReal;
//	int32_t   energyUsed;
//	uint16_t  partialTimestamp;

	state_item_t item0;
	item0.type = MESH_STATE_ITEM_TYPE_STATE;
	item0.state.id = 0;
	item0.state.switchState = 10;
	item0.state.flags = 0;
	item0.state.temperature = -100;
	item0.state.powerFactor = -90;
	item0.state.powerUsageReal = -30000;
	item0.state.energyUsed = -100000;
	item0.state.partialTimestamp = 60000;

	state_item_t item1;
	item1.type = MESH_STATE_ITEM_TYPE_STATE;
	item1.state.id = 1;
	item1.state.switchState = 11;
	item1.state.flags = 0;
	item1.state.temperature = -101;
	item1.state.powerFactor = -91;
	item1.state.powerUsageReal = -30001;
	item1.state.energyUsed = -100001;
	item1.state.partialTimestamp = 60001;

	state_item_t item2;
	item2.type = MESH_STATE_ITEM_TYPE_STATE;
	item2.state.id = 2;
	item2.state.switchState = 12;
	item2.state.flags = 0;
	item2.state.temperature = -102;
	item2.state.powerFactor = -92;
	item2.state.powerUsageReal = -30002;
	item2.state.energyUsed = -100002;
	item2.state.partialTimestamp = 60002;

	state_item_t item3;
	item3.type = MESH_STATE_ITEM_TYPE_STATE;
	item3.state.id = 3;
	item3.state.switchState = 13;
	item3.state.flags = 0;
	item3.state.temperature = -103;
	item3.state.powerFactor = -93;
	item3.state.powerUsageReal = -30003;
	item3.state.energyUsed = -100003;
	item3.state.partialTimestamp = 60003;

	state_item_t item4;
	item4.type = MESH_STATE_ITEM_TYPE_STATE;
	item4.state.id = 4;
	item4.state.switchState = 14;
	item4.state.flags = 0;
	item4.state.temperature = -104;
	item4.state.powerFactor = -94;
	item4.state.powerUsageReal = -30004;
	item4.state.energyUsed = -100004;
	item4.state.partialTimestamp = 60004;

	state_item_t item5;
	item5.type = MESH_STATE_ITEM_TYPE_STATE;
	item5.state.id = 5;
	item5.state.switchState = 15;
	item5.state.flags = 0;
	item5.state.temperature = -105;
	item5.state.powerFactor = -95;
	item5.state.powerUsageReal = -30005;
	item5.state.energyUsed = -100005;
	item5.state.partialTimestamp = 60005;

	sm.push_back(item0);
	sm.push_back(item1);
	sm.push_back(item2);
	sm.push_back(item3);
	sm.push_back(item4);
	sm.push_back(item5);

	cout << "Power usage [0]: " << smt.list[0].state.powerUsageReal << endl;
	cout << "Power usage [1]: " << smt.list[1].state.powerUsageReal << endl;
	cout << "Power usage [2]: " << smt.list[2].state.powerUsageReal << endl;
	cout << "Power usage [3]: " << smt.list[3].state.powerUsageReal << endl;

	cout << "Head: " << (int)smt.head << endl;
	cout << "Tail: " << (int)smt.tail << endl;
	cout << "Size: " << (int)smt.size << endl;

	for (state_message::iterator iter = sm.begin(); iter != sm.end(); ++iter) {
		state_item_t si = *iter;
		cout << "Power usage [" <<  si.state.id << "]: " << si.state.powerUsageReal << endl;
	}
}
