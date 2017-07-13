Melee Tournament Predictor
==========================

Follow me on twitter [here](https://twitter.com/Skull_64).

Introduction
------------

This program randomly simulates double elimination tournaments using Glicko
ratings. The project was inspired by the tournaments of Super Smash Bros. Melee,
but it will work for any tournament that uses the double elimination format and
where the Glicko ratings are known for most (ideally all) the players in the
bracket.

In the Glicko system, every set results in the winner gaining points and the
loser losing points, with the amount gained or lost dependent on the difference
in rating between the two players. A player who scores a big upset will
generally gain more points than a player who defeats someone below their skill
level. The Glicko system is similar to the Elo ratings one might see in Chess,
but in addition to assigning each player a rating, it also assigns each player a
"rating deviation" (RD) corresponding to how much uncertainty there is in that
player's rating. Players with a high RD gain or lose more points when winning or
losing than players with a low RD. The relevant formulas are described by Prof.
Mark E. Glickman, the creator of the Glicko system, in his paper
[Example of the Glicko-2 system](http://www.glicko.net/glicko/glicko2.pdf).

[Mobiusman](https://twitter.com/moby_osman) has been calculating Glicko ratings
for Melee players for quite some time now, and he goes into further detail about
how the Melee Glicko ratings work in his
[explanation of the June 2016 ratings on MIOM](http://www.meleeitonme.com/ssbm-elo-stats-june-2016-edition).

An important feature of the Glicko system that makes this type of analysis
possible is the fact that it is possible to calculate the probability that a
player wins a set given that the ratings and RDs for both players are known. For
example, in the September 2016 edition of the ratings, Mew2King had a rating of
2216 with an RD of 46, and Silent Wolf had a rating of 1956 with an RD of 70.
The formulas then dictate that Mew2King has about an 81% chance of defeating
Silent Wolf. If Mew2King wins, then their ratings change by +2 and -5,
respectively, whereas if Silent Wolf wins, their ratings change by -10 and +22,
espectively.

The infrastructure of a double elimination tournament is rather complicated, but
it is deterministic. This means that it is possible to simulate all the sets in
a tournament using the Glicko system many times over to calculate the
probability that each player will finish in each position.

Compilation
-----------

The Makefile in this repo currently only supports a Linux build. The program is
written in C++, so g++ or another compiler is needed. To build the predictor,
simply type `make`.

Usage
-----

**Setup**

The predictor requires three text files to be in the same directory:
`bracket_params.txt`, `initial_bracket.txt`, and `player_data.txt`.

`bracket_params.txt` contains information about how the bracket is set up, and
it contains five sections delimited by blank lines. The first section consists
two lines contain the number of players beginning in the winners and losers
bracket, respectively. The second section contains information about where
players that lose their winners bracket matches are placed into the losers
bracket, and it should contain a number of lines equivalent to the number of
rounds in the winners bracket before Winners Finals. The third, fourth, and
fifth sections contain information about matches that have already occurred in
the Winners bracket, Losers bracket, and Grand finals, respectively. These
section are quite difficult to describe, so see the following example which is
appropriate for
[Genesis 4](https://smash.gg/tournament/genesis-4/events/melee-singles/brackets?filter=%7B%22phaseId%22%3A104873%7D):

```
32
32

0 1
2 3 0 1
3 2 1 0 7 6 5 4
15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0

0
0 0
0 0 0 0
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

0
0
0 0
0 0
0 0 0 0
0 0 0 0
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

0
0
```

`initial_bracket.txt` contains the list of players that begin in the Winners
bracket, followed by a blank line, followed by the list of players that begin in
the Losers bracket. The order the players appear should be the same as the order
they appear in the actual bracket. For example, again for Genesis 4,

```
Mango
KirbyKaze
Wizzrobe
Mafia
***28 more players in Winners bracket***

Kalamazhu
RK
Medz
CIZ
***28 more players in Losers bracket***
```

Lastly, `player_data.txt` contains a list of players and their Glicko ratings
and RDs. For example, using the ratings as they were right before Genesis 4,

```
Armada     2463.081403  50.64365362
Hungrybox  2375.202872  45.31324573
Mango      2337.222557  48.35649073
Mew2King   2307.373054  45.07865783
Leffen     2289.015036  60.97024351
Plup       2175.044980  59.41493460
Westballz  2153.079024  44.93256734
SFAT       2144.626179  46.42156073
***the data for all the other players***
```

It should be noted that in both `initial_bracket.txt` and `player_data.txt`, all
spaces in player names must be replaced with underscores.

**Running**

If these three files are set up properly, then the program can be run with

```
./predictor [n]
```

where the optional parameter `n` is the number of simulations. If it is not
given, the default value of 100,000 will be used.

**Output**

The output is a list of all the players in the bracket along with the number of
times they ended up in each position. Additionally, a point value is assisgned
to each player to quantify their overall success. The maximum point value is
100, which corresponds to the player coming in 1st place in every simulation.
