#!/bin/bash


sudo kextunload /System/Library/Extensions/IOUSBFamily.kext/Contents/PlugIns/AppleUSBCDCACMData.kext
sudo kextunload /System/Library/Extensions/IOUSBFamily.kext/Contents/PlugIns/AppleUSBCDCECMData.kext
sudo kextunload /System/Library/Extensions/IOUSBFamily.kext/Contents/PlugIns/AppleUSBCDCACMControl.kext
sudo kextunload /System/Library/Extensions/IOUSBFamily.kext/Contents/PlugIns/AppleUSBCDC.kext
