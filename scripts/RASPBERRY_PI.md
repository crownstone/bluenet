# Development on the Raspberry PI

## Configuration

Set the right board type in `config/default/CMakeBuild.config`:

	HARDWARE_BOARD=ACR01B1D

## Flash binary

Build and upload to the PI:

	./firmware.sh -c build -t default
	./combine.sh default
	./scp_binary.sh

Flash to the Crownstone hardware on the PI:

	cd ~/dev/bluenet
	sudo openocd

This will use the local `openocd.cfg` script and the uploaded file `MERGED_OUTPUT.hex`.

The `openocd` script often **fails**.

	Error: timeout waiting for algorithm, a target reset is recommended
	Error: Failed to write to nrf5 flash
	Error: error writing to flash at address 0x00000000 at offset 0x00000000

The following error can be ignored:

	Error: nrf52.cpu -- clearing lockup after double fault

It has been successful when you see:

	wrote 516332 bytes from file MERGED_OUTPUT.hex in 10.100387s (49.922 KiB/s)
	** Programming Finished **
	** Verify Started **
	verified 318960 bytes in 1.509580s (206.338 KiB/s)
	** Verified OK **

A failure can be prevented by first calling `soft_reset_halt` in a telnet session (see below) before flashing new code.

## Output

Check the output locally using minicom

	ssh pi@10.27.8.158
	sudo minicom -b 230400 -c on -D /dev/ttyAMA0

## Control openocd

It is possible to control the openocd daemon remotely:

	telnet 10.27.8.158 4444

For example to reset the device through OpenOCD type the following in the telnet session:

	> soft_reset_halt
	> reset run

## SSH keys

Of course to save time, copy your pub key towards the PI:

	scp ~/.ssh/id_rsa.pub pi@10.27.8.158:/home/pi
	ssh pi@10.27.8.158
	cat ~/id_rsa.pub >> authorized_keys
	rm ~/id_rsa.pub

## Dashboard

To stream data to the dashboard:

	ssh pi@10.27.8.158
	cd ~/dev/bluenet-dashboard-backend
	python3 run.py

And locally in the repository cloned from <https://github.com/crownstone/bluenet-dashboard>:

	cd bluenet-dashboard
	xdg-open index.html

And connect. Fun fact: if you have a high-resolution screen this does not work, go in development mode and select a screen with lower resolution.
