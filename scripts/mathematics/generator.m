%
% --generator()
% --generator(switch_event)
% --generator(switch_event, cycles)
% 
%    Generate sine values between 0 and 255 and simulate switch events.
%
%    The argument switch_event can be:
%      
%        0: no switch event (default)
%        1: a switch event that keeps the voltage at a steady level (no load condition)
%        2: a switch event where voltage gets back to center line (127.5)
%
%    Configuration: 
%        * The center line is at 127.5. There is no additional offset w.r.t. 0.
%        * The switch delay is set at a fixed number of 25 samples.
%        * There are 100 samples per sine wave.
%        * The switch event can happen in any of the cycles except at the last cycle beyond "end - switch_delay".
%
function [t x] = generator(switch_event=0,cycles=3)
	switch_delay = 25;

	samples=100;
	resolution=2*pi/samples;
	t=0:resolution:2*pi*cycles-resolution;
	center_line=127.5;

	x=round((1-cos(t))*center_line);

	if (switch_event>0) 
		start = round(rand * (samples * cycles - switch_delay - 1)) + 1;
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
end
