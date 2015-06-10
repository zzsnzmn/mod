README  // teletype

HELP
	proof

remove debug prints
remove op text strings?


----> script tr execution clocked? on 1ms timer, where interrupts simply queue execution?
----> protect process() from ints? tele_tick??

v2//////////////
@ system ??
cv.set (kills slew, cuts to value)
input trigger state
i2c bidirectional
script dump: saving/loading to mass storage
NORMAL *** normal http://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
		http://c-faq.com/lib/gaussian.html
note names?
	/C /C# /D
tracker data views
	"note view"
	bars (coarse) -- voltage
	bars (fine) -- notes
	bars (fine bipolar) -- notes relative
	on/off boxes
hotkeys
	CTL-ESC kill timers/slew

///////

use static for "local" functions, optimizes jump distance and may inline
const for pointer args that don't change.
division by constant has optimized shortcut, see google
use speed optimization on compiler
pre-increment
pre-mask counters on wrap
use unsigned for bit shifting optimization
count down for simple loops
