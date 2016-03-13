===================
 Playback Overview
===================

:program:`percona-playback` is a utility for replaying database server load.
It currently supports replaying load captured via a |MySQL| query log file.

Inaccuracies in playback
========================

Due to the highly parallel nature of most database server workloads, it is
essentially impossible to ever 100% accurately replay what went on at any
particular time. :program:`percona-playback` makes a best effort to be
accurate and attempts to inform you of how much the replay differed from
what was originally executed.

You may get more inaccuracies when replaying against different database
server versions or a database server running on different hardware. This
could be due to how IO is scheduled, which competing transactions get
row locks first etc etc.
