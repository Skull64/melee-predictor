#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "math.h"

#ifdef _OPENMP
#include <omp.h>
#endif

// Output color and formatting
#define RST "\x1B[0m"
#define FRED(x) "\x1B[31m" x RST
#define FYEL(x) "\x1B[33m" x RST
#define BOLD(x) "\x1B[1m" x RST

extern float pi;
extern float q;
extern float qs;

void throw_error(std::string);
void throw_warning(std::string);

float rand_float();
float square();
int int_power();

std::ifstream open_file(std::string);

void load_section(std::ifstream&, std::vector<std::vector<int>>&);

void load_bracket_params(int&, int&,
                         std::vector<std::vector<int>>&,
                         std::vector<std::vector<int>>&,
                         std::vector<std::vector<int>>&,
                         std::vector<std::vector<int>>&);

void load_initial_players(std::vector<std::string>&, std::vector<std::string>&);

class Player {
 public:
  std::string name;
  float rating, RD;
  float rating_orig, RD_orig;
  std::vector<int> placings;
  float avg_points;

  Player(std::string, float, float);  // constructor
  Player(const Player&);  // copy constructor
  void reset_rating();
  void update_orig_rating();
  void calc_avg_points();
};

typedef std::map<std::string, Player*> playerLibrary;

playerLibrary load_player_data();

playerLibrary copy_player_library(const playerLibrary&);

void reset_players(playerLibrary);

struct by_avg_points {
  bool operator()(Player* p1, Player* p2) {
    return p1->avg_points > p2->avg_points;
  }
};

class Match {
 public:
  std::string name;
  char side;
  int round_id;
  int index;
  Player* player_1, *player_2;
  Match* winner_to, *loser_to;
  int wt_index, lt_index;
  int result, result_fixed;
  Player* winner, *loser;

  Match(std::string, char, int, int); // constructor
  void set_structure(Match*, int, Match*, int);
  void set_players(Player*, Player*);
  void simulate(bool);

  // Used for GF1 only
  bool bracket_reset;
  Match* wside_winner_to, *wside_loser_to, *lside_winner_to, *lside_loser_to;
  int wside_wt_index, wside_lt_index, lside_wt_index, lside_lt_index;
  void set_structure_gf1(Match*, int, Match*, int, Match*, int, Match*, int);
};

class Round {
 public:
  std::string name;
  char side;
  int round_id;
  int num_matches;
  std::vector<Match*> matches;

  Round(char, int);
  void set_res_fixed(std::vector<int>);
  void simulate(bool);
};

class Bracket {
 public:
  playerLibrary player_library;
  int num_rounds_W, num_rounds_L, num_rounds_G, num_rounds_P;
  std::vector<Player*> players_in_bracket;
  std::vector<Round*> winners, losers, grands, placings;

  Bracket(int, int);
  void set_player_library(playerLibrary);
  void set_structure(std::vector<std::vector<int>>);
  void set_initial_players(std::vector<std::string>,
                           std::vector<std::string>, int);
  void set_res_fixed(std::vector<std::vector<int>>,
                     std::vector<std::vector<int>>,
                     std::vector<std::vector<int>>);
  void update_player_results();
  void simulate(bool);
};
