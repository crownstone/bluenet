#include <protocol/mesh/cs_MeshMessageState.h>

#include <iostream>

using namespace std;

int main() {

	cout << "Test MeshMessageState implementation" << endl;
	cout << "MAX_STATE_ITEMS: " << MAX_STATE_ITEMS << endl;

	state_message_t smt;

	state_message sm(smt);

	sm.clear();

	state_item_t item0 = { 0, 1, 1024, 100, 50};
	state_item_t item1 = { 1, 1, 1024, 120, 40};
	state_item_t item2 = { 2, 0, 1024, 220, 40};
	state_item_t item3 = { 3, 0, 1024, 130, 20};
	state_item_t item4 = { 4, 1, 1024, 30, 30};
	state_item_t item5 = { 5, 1, 1024, 100, 100};

	sm.push_back(item0);
	sm.push_back(item1);
	sm.push_back(item2);
	sm.push_back(item3);
	sm.push_back(item4);
	sm.push_back(item5);

	cout << "Power usage [0]: " << smt.list[0].powerUsageApparant << endl;
	cout << "Power usage [1]: " << smt.list[1].powerUsageApparant << endl;
	cout << "Power usage [2]: " << smt.list[2].powerUsageApparant << endl;
	cout << "Power usage [3]: " << smt.list[3].powerUsageApparant << endl;

	cout << "Head: " << (int)smt.head << endl;
	cout << "Tail: " << (int)smt.tail << endl;
	cout << "Size: " << (int)smt.size << endl;

	for (state_message::iterator iter = sm.begin(); iter != sm.end(); ++iter) {
		state_item_t si = *iter;
		cout << "Power usage [" <<  si.id << "]: " << si.powerUsageApparant << endl;
	}
}
