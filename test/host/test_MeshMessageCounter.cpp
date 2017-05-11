/*
 * Author: Bart van Vliet
 * Copyright: Crownstone B.V. (https://crownstone.rocks)
 * Date: Apr 25, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <protocol/mesh/cs_MeshMessageCounter.h>

#include <iostream>
#include <limits>
#include <stdint.h>

using namespace std;

int main() {
	cout << "Test MeshMessageCounter implementation" << endl;

	MeshMessageCounter msgCounter;
	MeshMessageCounter msgCounter2;

	uint32_t val = msgCounter.getVal();
	cout << "counter start: " << val << endl;

	msgCounter.setVal(std::numeric_limits<uint32_t>::max()-1);
//	msgCounter.setVal(uint32_Max-1);
	cout << "set: " << msgCounter.getVal() << endl;


	val = (++msgCounter).getVal();
	cout << "inc: " << val << endl;
	val = (++msgCounter).getVal();
	cout << "inc: " << val << endl;
	val = (++msgCounter).getVal();
	cout << "inc: " << val << endl;

	int32_t delta;

	// New is larger than old
	msgCounter.setVal(1000);
	msgCounter2.setVal(std::numeric_limits<uint32_t>::max() - 1000);
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// New is smaller than old, but newer
	msgCounter.setVal(1000);
	msgCounter2.setVal(std::numeric_limits<uint16_t>::max());
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// New is smaller than old
	msgCounter.setVal(std::numeric_limits<uint32_t>::max() - 1000);
	msgCounter2.setVal(1000);
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// New is larger than old, but older
	msgCounter.setVal(std::numeric_limits<uint16_t>::max());
	msgCounter2.setVal(1000);
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// Old is not in loop yet
	msgCounter.setVal(1);
	msgCounter2.setVal(1000);
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// Old is not in loop yet, int32 overflow
	msgCounter.setVal(1);
	msgCounter2.setVal(std::numeric_limits<uint32_t>::max());
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// New is not in loop yet
	msgCounter.setVal(1000);
	msgCounter2.setVal(1);
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;

	// New is not in loop yet, int32 overflow
	msgCounter.setVal(std::numeric_limits<uint32_t>::max());
	msgCounter2.setVal(1);
	delta = msgCounter.calcDelta(msgCounter2.getVal());
	cout << "delta: " << msgCounter2.getVal() << " - " << msgCounter.getVal() << " = " << delta << endl;
}
