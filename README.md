# c
c game 
rime
====

a small game in C. one file of terminal UI, one of simulation, one of
level generation, no dependencies past ncurses.

    make
    ./rime

wants a terminal of about 44x24 or bigger. `./rime --seed 1234` if you
want the same vault someone else got.


what it is
----------

you are the only warm thing in a frozen vault, and you are cooling down.
the way out is a stair down, and between you and it is rime: packed ice
you cannot break, cannot dig, cannot walk through. you can only melt it,
and the only heat down here is whatever is left in you.

so you carry braziers. you set one down beside a wall of rime and it
burns its way through in a few turns. you can pick the brazier back up
and carry it on. you can also just stand there and huddle, breathing on
the ice until it gives, which costs nothing but time and a great deal of
you.

the catch is that warmth wakes things. the Wept are the people who were
down here when it froze, standing where they fell, and heat is the only
thing that gets them moving. they walk as fast as you do. they keep
coming for about twenty turns after they wake and then stiffen again
wherever they happen to be standing, so a fire you lit three rooms ago
is still making enemies for you. if you are next to a brazier, huddling
actually warms you back up, which is the only way to recover, and it is
also exactly what they are looking for.

that is the whole game. every useful thing you can do is the thing that
endangers you.


keys
----

    h j k l / arrows   walk
    H                  huddle. stay put, warm the floor, warm yourself
                       if you are next to something burning
    b                  set a brazier down            (20 heat)
    p                  pick a brazier of yours up    (5 heat)
    f                  relight a cold brazier beside you (25 heat)
    ,                  look at the tile you are on
    >                  go down, standing on the stair
    ?                  the long version of all this
    q                  stop

walking into a frozen Wept shoves it along, which is how you clear a
doorway. walking into an awake one also shoves it, but it costs you
fifteen heat and leaves it reeling for a turn, and against a wall you
trade places with it instead. that always works. shove them into
meltwater and they go under for good.

seven depths. you keep whatever heat and braziers you had left.


things worth knowing
--------------------

- the floor has a temperature and you can feel it. the number after
  `floor` in the bar is the tile under you, over or under freezing.
- ice melts, water freezes. both happen while you watch. a corridor you
  walked through as water can be ice by the time you come back.
- rime shows its damage: `H` is solid, then `Y`, `U`, `n`, then water.
- a brazier you set down burns about forty-five turns. one you relight
  from the vault's own cold stock burns much longer but is not yours to
  carry.
- vents (`^`) are warm for ever and cost nothing. they are weak, but
  standing next to one and huddling will slowly put you back together.


building and testing
--------------------

    make            # the game
    make test       # drives the simulation with no terminal involved
    make clean

`make test` is the interesting one. it checks the generator over a couple
hundred levels (sealed, solvable, and solvable one burn at a time, which
is not the same thing), checks that fire melts rime and a body walking
past does not, that the Wept wake and stiffen and never end up walling a
level off, that water freezes and ice melts, that shoving works the way
the help screen says it does. then it plays twelve levels with a bot that
only knows how to walk at the stair and burn whatever is in the way, and
complains if the bot cannot get down at least half of them.
