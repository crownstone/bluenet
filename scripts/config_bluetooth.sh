#!/bin/bash

sudo hciconfig hci0 down
sudo btmgmt -i hci0 le on
sudo btmgmt -i hci0 privacy on
sudo btmgmt -i hci0 discov on
sudo btmgmt -i hci0 sc off
sudo btmgmt -i hci0 ssp off
sudo btmgmt -i hci0 bredr off
sudo hciconfig hci0 up
