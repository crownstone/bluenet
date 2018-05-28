%
% --generator()
% --generator(switch_event)
% --generator(switch_event, noise)
% --generator(switch_event, noise, cycles)
% 
%    Generate sine values between 0 and 255 and simulate switch events.
%
%    The argument switch_event can be:
%      
%        0: no switch event (default)
%        1: a switch event that keeps the voltage at a steady level (no load condition)
%        2: a switch event where voltage gets back to center line (127.5)
%
%    The noise argument:
%    
%        0: no noise
%        1: a single spike of 10x center line
%
%    Configuration: 
%        * The center line is at 2300 and the range is from 380 to 3620.
%        * The switch delay is set at a fixed number of 25 samples.
%        * There are 100 samples per sine wave.
%        * The switch event can happen in any of the cycles except for the first half of the first cycle and 
%          last half of the last cycle 
%
function [t x] = generator(switch_event=0,noise=0,cycles=3)
	switch_delay = 25;

	top=3620;
	bottom=380;

	samples=100;
	resolution=2*pi/samples;
	t=0:resolution:2*pi*cycles-resolution;
	center_line=(top-bottom)/2;

	x=round(bottom + (1-cos(t))*center_line);

	if (switch_event>0) 
		start = round(rand * (samples * (cycles - 1) - (switch_delay - 1))) + samples/2;
		if (switch_event == 1)
			for i=1:switch_delay
				x(start+i) = x(start);
			end
		else
			for i=1:switch_delay
				x(start+i) = ( x(start+i-1) - center_line) * 80/100 + center_line;
			end
		end
	end

	switch(noise)
	case 1
		spike = center_line * 100;
		spike_at = round(rand*cycles*samples - 1) + 1;
		x(spike_at) = spike;
	case 2
		% noise should be a bit related to the size of the signal
		noise_values = (rand(1,length(x))-0.5)*5;
		x += noise_values;
	end
end
