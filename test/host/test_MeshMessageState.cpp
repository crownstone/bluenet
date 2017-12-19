#include <protocol/mesh/cs_MeshMessageState.h>

#include <iostream>

using namespace std;

int main() {

	cout << "Test MeshMessageState implementation" << endl;
	cout << "MAX_STATE_ITEMS: " << MAX_STATE_ITEMS << endl;

	state_message_t smt;

	state_message sm(smt);

	sm.clear();

	state_item_t item0 = { 0, 0, 10, 0, -75, -750, 1000, 2000}; // type, id, switch, bitmask, powerfactor, power, energy, timestamp
	state_item_t item1 = { 0, 1, 11, 0, -50, -500, 1100, 2100};
	state_item_t item2 = { 0, 2, 12, 0, -25, -250, 1200, 2200};
	state_item_t item3 = { 0, 3, 13, 0, 20,  200,  1300, 2300};
	state_item_t item4 = { 0, 4, 14, 0, 40,  400,  1400, 2400};
	state_item_t item5 = { 0, 5, 15, 0, 80,  800,  1500, 2500};

	sm.push_back(item0);
	sm.push_back(item1);
	sm.push_back(item2);
	sm.push_back(item3);
	sm.push_back(item4);
	sm.push_back(item5);

	cout << "Power usage [0]: " << smt.list[0].powerUsageReal << endl;
	cout << "Power usage [1]: " << smt.list[1].powerUsageReal << endl;
	cout << "Power usage [2]: " << smt.list[2].powerUsageReal << endl;
	cout << "Power usage [3]: " << smt.list[3].powerUsageReal << endl;

	cout << "Head: " << (int)smt.head << endl;
	cout << "Tail: " << (int)smt.tail << endl;
	cout << "Size: " << (int)smt.size << endl;

	for (state_message::iterator iter = sm.begin(); iter != sm.end(); ++iter) {
		state_item_t si = *iter;
		cout << "Power usage [" <<  si.id << "]: " << si.powerUsageReal << endl;
	}
}
