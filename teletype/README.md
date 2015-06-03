README  // teletype


microfont http://www.timetrap.se/articles/freetajp.php


(((((((((((((((((((((((((tele)))))))))))))))))))))))))

@ system
HELP?

too many params: restructure to report error_detail
new error: non-return ops not on left
new error: sep in wrong place (index 0)

((((((((((((((((((((hardware))))))))))))))))))))


ui
	activity?

SPREADSHEET
	visual
		show start/end? index/wrap?
	functions
		edit
			set start (shift s)
			set end (shift e)
			grab knob val (scaled?)

preset
	flash
	screen/nav
	note?
	init

i2c
	earthsea
		clocking mode, clock step
	mp.sync

editing
	knob-read insertion

hotkeys
	manual TR
	kill timers/slew
	tr mute?

N/V/VV
	negative indexing

PATTERN
	negative index values

NORMAL				*** normal http://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
		http://c-faq.com/lib/gaussian.html
QT

DRUNK

SR
SR.L
SR.AVG

cv.set (kills slew, cuts to value)

input trigger state



move DOC strings to flash?


note names?
/C /C# /D
	these won't be visible in the tracker view, which is what people want.

----> script tr execution clocked? on 1ms timer, where interrupts simply queue execution?
----> protect process() from ints? tele_tick??

v2//////////////
i2c bidirectional
script dump: saving/loading to mass storage



///////

use static for "local" functions, optimizes jump distance and may inline
const for pointer args that don't change.
division by constant has optimized shortcut, see google
use speed optimization on compiler
pre-increment
pre-mask counters on wrap
use unsigned for bit shifting optimization
count down for simple loops
