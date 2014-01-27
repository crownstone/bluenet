
Demonstrates Nrf51822 <-> Node connectivity.

## Requirements ##

###On Mac:###
	brew install cairo
	export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/opt/X11/lib/pkgconfig


###On Raspbian/Raspbery Pi:###

	sudo apt-get install libcairo2-dev libjpeg8-dev libpango1.0-dev libgif-dev build-essential g++ 
	wget http://node-arm.herokuapp.com/node_latest_armhf.deb
	sudo dpkg -i node_latest_armhf.deb
	sudo apt-get install libbluetooth-dev blues


###On Both:###
    npm install noble express socket.io canvas canvg xmldom

##Running:##

    node app.js
    open http://localhost:8080/usg.html

