#include "Bracket.hpp"

float pi = 3.14159265358979E+00;
float q  = 5.75646273248511E-03;
float qs = 3.31368631904900E-05;
bool update_ratings = true;

#ifdef _WIN32
int get_console_color() {
  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
  return info.wAttributes;
}
#endif

void print_status_msg(int status, std::string msg) {
#ifdef _WIN32
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  int orig_color = get_console_color();
  if (status == 0) {
    SetConsoleTextAttribute(hConsole, (orig_color & ~0xF) + IRED);
    std::cerr << "ERROR: ";
  } else if (status == 1) {
    SetConsoleTextAttribute(hConsole, (orig_color & ~0xF) + IYEL);
    std::cerr << "WARNING: ";
  } else {
    std::cerr << "STATUS: ";
  }
  SetConsoleTextAttribute(hConsole, orig_color);
  std::cerr << msg << std::endl;
#else
  if (status == 0)
    std::cerr << BOLD(FRED("ERROR: ")) << msg << std::endl;
  else if (status == 1)
    std::cerr << BOLD(FYEL("WARNING: ")) << msg << std::endl;
  else
    std::cerr << BOLD("STATUS: ") << msg << std::endl;
#endif
}

void throw_error(std::string msg) {
  print_status_msg(0, msg);
  exit(EXIT_FAILURE);
}

void throw_warning(std::string msg) {
  print_status_msg(1, msg);
}

// Return a random float between 0 and 1
float rand_float() {
  std::random_device rdv;
  static std::mt19937 gen(rdv());
  std::uniform_real_distribution<float> dis(0., 1.);
  return dis(gen);
}

// Return the square of a number
float square(float x) {
  return x * x;
}

// Take an int to the power of another int
int int_power(int x, int n) {
  if (x == 0)
    return 0;
  if (n <= 0)
    return 1;
  int y = 1;
  while (n > 1) {
    if (n % 2 == 0) {
      x *= x;
      n /= 2;
    } else {
      y *= x;
      x *= x;
      n = (n - 1) / 2;
    }
  }
  return x * y;
}

// Get the ordinal of a number
std::string get_ordinal(int a) {
  assert(a > 0);
  std::string suffix;
  int last_digit = a % 10;
  int second_last_digit = (a / 10) % 10;
  if (second_last_digit == 1)
    suffix = "th";
  else
    if (last_digit == 1)
      suffix = "st";
    else if (last_digit == 2)
      suffix = "nd";
    else if (last_digit == 3)
      suffix = "rd";
    else
      suffix = "th";
  return std::to_string(a) + suffix;
}

// Open a file
std::ifstream open_file(std::string fname) {
  std::ifstream infile(fname);
  if (infile.fail())
    throw_error(fname + " not found");
  return infile;
}

// Load a section from file
void load_section(std::ifstream& infile, std::vector<std::vector<int>>& out_vec) {
  std::string buffer;
  int temp;
  while (buffer.empty())
    std::getline(infile, buffer);
  while (!buffer.empty()) {
    std::stringstream iss(buffer);
    std::vector<int> tempvec;
    while (iss >> temp)
      tempvec.push_back(temp);
    out_vec.push_back(tempvec);
    std::getline(infile, buffer);
  }
}

// Load bracket parameters from file
void load_bracket_params(int& num_W, int& num_L,
                         std::vector<std::vector<int>>& wl_map,
                         std::vector<std::vector<int>>& res_fixed_W,
                         std::vector<std::vector<int>>& res_fixed_L,
                         std::vector<std::vector<int>>& res_fixed_G) {
  std::ifstream infile = open_file("bracket_params.txt");

  // Number of players in each side of the bracket
  std::string buffer;
  std::getline(infile, buffer);
  // Make sure num_W is a power of 2
  try {
    num_W = std::stoi(buffer);
    if (((num_W & (num_W - 1)) != 0) || num_W <= 0)
      throw 1;
  } catch (...) {
    throw_error("Number of players in winners bracket = " +
                buffer + ", must be a power of 2");
  }
  std::getline(infile, buffer);
  // Make sure num_L = num_W
  try {
    num_L = std::stoi(buffer);
    if ((num_L != num_W) && (num_L != 0))
      throw 1;
  } catch (...) {
    throw_error("Number of players in losers bracket = " + buffer +
                ", must be be either the same as the number of players in winners bracket or zero");
  }
  std::getline(infile, buffer);

  // Locations in losers bracket where players get sent from winners bracket
  load_section(infile, wl_map);

  // Fixed results
  load_section(infile, res_fixed_W);
  load_section(infile, res_fixed_L);
  load_section(infile, res_fixed_G);

  infile.close();
}

// Load the initial player locations from file
void load_initial_players(std::vector<std::string>& players_W,
                          std::vector<std::string>& players_L) {
  std::ifstream infile = open_file("initial_bracket.txt");
  std::string name;

  // Winners bracket
  std::getline(infile, name);
  while (!name.empty()) {
    players_W.push_back(name);
    std::getline(infile, name);
  }

  // Losers bracket
  std::getline(infile, name);
  while (!name.empty()) {
    players_L.push_back(name);
    std::getline(infile, name);
  }
}

// Player object constructor
Player::Player(std::string nam, float rat, float rd) {
  name = nam;
  rating_orig = rat;
  RD_orig = rd;
  placings.resize(12);
  avg_points = 0.;
}

// Player object copy constructor
Player::Player(const Player& orig) {
  name = orig.name;
  rating = orig.rating;
  RD = orig.RD;
  rating_orig = orig.rating_orig;
  RD_orig = orig.RD_orig;
  placings = orig.placings;
  avg_points = orig.avg_points;
}

// Reset a player's rating back to its original value
void Player::reset_rating() {
  rating = rating_orig;
  RD = RD_orig;
}

// Set a player's original rating to its current value
void Player::update_orig_rating() {
  rating_orig = rating;
  RD_orig = RD;
}

// Calculate the average number of points obtained by a player
void Player::calc_avg_points() {
  int t = 0;
  float p = 100.;
  for (int i = 0; i < placings.size(); i++) {
    avg_points += placings[i] * p;
    t += placings[i];
    p *= 0.75;
  }
  avg_points /= t;
}

// Load player data from file
playerLibrary load_player_data() {
  playerLibrary player_library;
  std::ifstream infile = open_file("player_data.txt");
  std::string name;
  float rating_orig, RD_orig;
  while (infile >> name >> rating_orig >> RD_orig) {
    Player* player = new Player(name, rating_orig, RD_orig);
    player_library.insert(std::pair<std::string, Player*>(name, player));
  }
  return player_library;
}

// Copy a player library
playerLibrary copy_player_library(const playerLibrary& orig) {
  playerLibrary player_library;
  for (playerLibrary::const_iterator it = orig.begin(); it != orig.end(); it++) {
    std::string name = (*it).first;
    Player* player = new Player(*((*it).second));
    player_library.insert(std::pair<std::string, Player*>(name, player));
  }
  return player_library;
}

// Reset all player ratings and RDs
void reset_players(playerLibrary player_library) {
  for (playerLibrary::iterator it = player_library.begin();
       it != player_library.end(); it++) {
    (*it).second->reset_rating();
  }
}

// Match object constructor
Match::Match(std::string nam, char sid, int rid, int i) {
  name = nam;
  side = sid;
  round_id = rid;
  index = i;
}

// Set the structure of a match; i.e. where the winner and loser go next
void Match::set_structure(Match* wt, int wti, Match* lt, int lti) {
  winner_to = wt;
  loser_to = lt;
  wt_index = wti;
  lt_index = lti;
}

// Set the structure of Grand finals set 1
void Match::set_structure_gf1(Match* w_wt, int w_wti, Match* l_lt, int l_lti,
                              Match* w_lt, int w_lti, Match* l_wt, int l_wti) {
  wside_winner_to = w_wt;
  lside_loser_to = l_lt;
  wside_loser_to = w_lt;
  lside_winner_to = l_wt;
  wside_wt_index = w_wti;  // 0
  lside_lt_index = l_lti;  // 0
  wside_lt_index = w_lti;  // 1
  lside_wt_index = l_wti;  // 0
}

// Set the players taking place in a match
void Match::set_players(Player* p1, Player* p2) {
  player_1 = p1;
  player_2 = p2;
}

// Simulate a match
void Match::simulate() {
  float dif, RD, g, E;
  int s1, s2;

  result = result_fixed;
  assert(result == 0 || result == 1 || result == 2);

  // Determine a winner
  if (result == 0) {
    dif = player_1->rating - player_2->rating;
    RD = sqrt(square(player_1->RD) + square(player_2->RD));
    g = 1. / sqrt(1. + 3. * square(q * RD / pi));
    E = 1. / (1. + pow(10., -g * dif / 400.));
    if (E > rand_float())
      result = 1;  // Player 1 wins
    else
      result = 2;  // Player 2 wins
  }

  if (result == 1) {  // Player 1 has won
    s1 = 1;
    s2 = 0;
    winner = player_1;
    loser = player_2;
  } else {  // Player 2 has won
    s1 = 0;
    s2 = 1;
    winner = player_2;
    loser = player_1;
  }

  // Send the players to their next match
  // assert(wt_index == 0 || wt_index == 1);
  // assert(lt_index == 0 || lt_index == 1);
  if (side == 'G' && round_id == 1) {  // Grand finals set 1
    if (result == 1) {  // Player from winners bracket wins
      bracket_reset = false;
      if (wside_wt_index == 0)
        wside_winner_to->player_1 = winner;
      else
        wside_winner_to->player_2 = winner;
      if (lside_lt_index == 0)
        lside_loser_to->player_1 = loser;
      else
        lside_loser_to->player_2 = loser;
    } else {  // Player from losers bracket wins
      bracket_reset = true;
      if (wside_lt_index == 0)
        wside_loser_to->player_1 = loser;
      else
        wside_loser_to->player_2 = loser;
      if (lside_wt_index == 0)
        lside_winner_to->player_1 = winner;
      else
        lside_winner_to->player_2 = winner;
    }
  } else {  // Every other set
    if (wt_index == 0)
      winner_to->player_1 = winner;
    else
      winner_to->player_2 = winner;
    if (lt_index == 0)
      loser_to->player_1 = loser;
    else
      loser_to->player_2 = loser;
  }

  // Update ratings and RDs
  if (update_ratings) {
    float g1, g2, E1, E2, x1, x2, y1, y2;
    dif = player_1->rating - player_2->rating;
    g1 = 1. / sqrt(1. + 3. * square(q * player_1->RD / pi));
    g2 = 1. / sqrt(1. + 3. * square(q * player_2->RD / pi));
    E1 = 1. / (1. + pow(10., -g2 * dif / 400.));
    E2 = 1. / (1. + pow(10.,  g1 * dif / 400.));
    x1 = 1. / (square(player_1->RD));
    x2 = 1. / (square(player_2->RD));
    y1 = qs * square(g2) * E1 * (1. - E1);  // 1/(d^2)
    y2 = qs * square(g1) * E2 * (1. - E2);  // 1/(d^2)
    player_1->RD = (std::max)(30., sqrt(1. / (x1 + y1)));
    player_2->RD = (std::max)(30., sqrt(1. / (x2 + y2)));
    player_1->rating += q * g2 * (s1 - E1) / (x1 + y1);
    player_2->rating += q * g1 * (s2 - E2) / (x2 + y2);
  }
}

// Round object constructor
Round::Round(char sid, int rid) {
  side = sid;
  round_id = rid;
  assert(side == 'W' || side == 'L' || side == 'G' || side == 'P');

  if (side == 'W') {
    num_matches = int_power(2, round_id);
    if (round_id == 0)
      name = "Winners Finals";
    else if (round_id == 1)
      name = "Winners Semis";
    else if (round_id == 2)
      name = "Winners Quarters";
    else
      name = "Winners Top " + std::to_string(3 * int_power(2, round_id));
  } else if (side == 'L') {
    num_matches = int_power(2, round_id / 2);
    if (round_id == 0)
      name = "Losers Finals";
    else if (round_id == 1)
      name = "Losers Semis";
    else if (round_id == 2)
      name = "Losers Quarters";
    else
      name = "Losers Top " +
             std::to_string((round_id % 2 + 3) * int_power(2, (round_id / 2)));
  } else if (side == 'G') {
    num_matches = 1;
    if (round_id == 0)
      name = "Grand Finals S2";
    else if (round_id == 1)
      name = "Grand Finals";
    else
      name = "";
  } else if (side == 'P') {
    if (round_id < 6)
      num_matches = 1;
    else
      num_matches = int_power(2, round_id / 2 - 2);
    int placing;
    if (round_id < 2) {
      placing = round_id + 1;
    } else {
      placing = int_power(2, round_id / 2);
      placing += (round_id & 1) * (placing / 2) + 1;
    }
    name = get_ordinal(placing) + " Place";
  }

  // Create match objects
  for (int i = 0; i < num_matches; i++) {
    Match* match = new Match(name, side, round_id, i);
    matches.push_back(match);
  }
}

// Set the results of the matches in a round that are already known
void Round::set_res_fixed(std::vector<int> res_fixed) {
  for (int i = 0; i < res_fixed.size(); i++)
    matches[i]->result_fixed = res_fixed[i];
}

// Simulate all the matches in a round
void Round::simulate() {
  for (std::vector<Match*>::iterator it = matches.begin();
       it != matches.end(); it++)
    (*it)->simulate();
}

// Bracket object constructor
Bracket::Bracket(int numw, int numl) {
  num_W = numw;
  num_L = numl;

  // Number of rounds
  num_rounds_W = ((int) ceil(log2(num_W)));  // Winners bracket
  if (num_L == 0)
    num_rounds_L = 2 * (num_rounds_W - 1);  // Losers bracket
  else
    num_rounds_L = num_rounds_W + ((int) ceil(log2(num_L)));
  num_rounds_G = 2;  // Grand finals
  num_rounds_P = num_rounds_L + 2;  // Placings

  // Create round objects
  for (int i = 0; i < num_rounds_W; i++) {
    Round* round = new Round('W', i);
    winners.push_back(round);
  }
  for (int i = 0; i < num_rounds_L; i++) {
    Round* round = new Round('L', i);
    losers.push_back(round);
  }
  for (int i = 0; i < num_rounds_G; i++) {
    Round* round = new Round('G', i);
    grands.push_back(round);
  }
  for (int i = 0; i < num_rounds_P; i++) {
    Round* round = new Round('P', i);
    placings.push_back(round);
  }
}

// Set the player library to use for the bracket
void Bracket::set_player_library(playerLibrary pys) {
  player_library = pys;
}

// Setup the bracket structure; i.e. where the winner and loser of every match goes next
void Bracket::set_structure(std::vector<std::vector<int>> wl_map) {
  // Winners finals
  winners[0]->matches[0]->set_structure(grands[1]->matches[0], 0,
                                        losers[0]->matches[0], 0);
  // Other winners bracket
  for (int rid = 1; rid < num_rounds_W; rid++)
    for (int i = 0; i < winners[rid]->num_matches; i++)
      // Structure of first round of winners bracket is treated differently if no
      // players begin in losers bracket
      if (rid == num_rounds_W - 1 && num_L == 0)
        winners[rid]->matches[i]->set_structure(winners[rid - 1]->matches[i / 2], i % 2,
                                                losers[rid * 2 - 1]->matches[wl_map[rid - 1][i]], i % 2);
      else
        winners[rid]->matches[i]->set_structure(winners[rid - 1]->matches[i / 2], i % 2,
                                                losers[rid * 2]->matches[wl_map[rid - 1][i]], 0);

  // Losers finals
  losers[0]->matches[0]->set_structure(grands[1]->matches[0], 1,
                                       placings[2]->matches[0], 0);
  // Losers bracket: no players came directly from winners
  for (int rid = 1; rid < num_rounds_L; rid += 2)
    for (int i = 0; i < losers[rid]->num_matches; i++)
      losers[rid]->matches[i]->set_structure(losers[rid - 1]->matches[i], 1,
                                             placings[rid + 2]->matches[i / 2], i % 2);
  // Losers bracket: half the players came directly from winners
  for (int rid = 2; rid < num_rounds_L; rid += 2)
    for (int i = 0; i < losers[rid]->num_matches; i++)
      losers[rid]->matches[i]->set_structure(losers[rid - 1]->matches[i / 2], i % 2,
                                             placings[rid + 2]->matches[i / 2], i % 2);

  // Grand finals set 2
  grands[0]->matches[0]->set_structure(placings[0]->matches[0], 0,
                                       placings[1]->matches[0], 0);
  // Grand finals set 1
  grands[1]->matches[0]->set_structure_gf1(placings[0]->matches[0], 0,
                                           placings[1]->matches[0], 0,
                                           grands[0]->matches[0], 1,
                                           grands[0]->matches[0], 0);
}

// Set the initial player locations
void Bracket::set_initial_players(std::vector<std::string> players_W,
                                  std::vector<std::string> players_L, int t) {
  std::string name;
  Player* player, *player_1, *player_2;

  float rating_default = 1600.;
  float RD_default = 200.;

  // Winners bracket
  int i = 0;
  for (std::vector<std::string>::iterator it = players_W.begin();
       it != players_W.end(); it++) {
    std::string name = *it;
    // If player not found, create player with default values
    if (player_library.find(name) == player_library.end()) {
      if (t == 0)
        throw_warning("Player \"" + name + "\" not found. Using default rating, RD of " +
                      std::to_string(rating_default) + ", " + std::to_string(RD_default));
      player = new Player(name, rating_default, RD_default);
      player_library.insert(std::pair<std::string, Player*>(name, player));
    }
    // Place the players in winners bracket
    if (i % 2 == 0) {
      player_1 = player_library.find(name)->second;
    } else {
      player_2 = player_library.find(name)->second;
      winners.back()->matches[i / 2]->set_players(player_1, player_2);
      players_in_bracket.push_back(player_1);
      players_in_bracket.push_back(player_2);
    }
    i++;
  }

  // Losers bracket
  i = 0;
  for (std::vector<std::string>::iterator it = players_L.begin();
       it != players_L.end(); it++) {
    std::string name = *it;
    // If player not found, create player with default values
    if (player_library.find(name) == player_library.end()) {
      if (t == 0)
        throw_warning("Player \"" + name + "\" not found. Using default rating, RD of " +
                      std::to_string(rating_default) + ", " + std::to_string(RD_default));
      player = new Player(name, rating_default, RD_default);
      player_library.insert(std::pair<std::string, Player*>(name, player));
    }
    // Place the players in losers bracket
    if (i % 2 == 0) {
      player_1 = player_library.find(name)->second;
    } else {
      player_2 = player_library.find(name)->second;
      losers.back()->matches[i / 2]->set_players(player_1, player_2);
      players_in_bracket.push_back(player_1);
      players_in_bracket.push_back(player_2);
    }
    i++;
  }
}

// Set the results of the matches in a bracket that are already known
void Bracket::set_res_fixed(std::vector<std::vector<int>> res_fixed_W,
                            std::vector<std::vector<int>> res_fixed_L,
                            std::vector<std::vector<int>> res_fixed_G) {
  for (int i = 0; i < res_fixed_W.size(); i++)
    winners[i]->set_res_fixed(res_fixed_W[i]);
  for (int i = 0; i < res_fixed_L.size(); i++)
    losers[i]->set_res_fixed(res_fixed_L[i]);
  for (int i = 0; i < res_fixed_G.size(); i++)
    grands[i]->set_res_fixed(res_fixed_G[i]);
}

// Update player results (post-simulation)
void Bracket::update_player_results() {
  // 1st-4th place: 1 player each
  for (int i = 0; i < 4; i++) {
    placings[i]->matches[0]->player_1->placings[i] += 1;
  }
  // 5th place and on: multiple players each
  for (int i = 4; i < num_rounds_P; i++) {
    for (std::vector<Match*>::iterator it = placings[i]->matches.begin();
         it != placings[i]->matches.end(); it++) {
      (*it)->player_1->placings[i] += 1;
      (*it)->player_2->placings[i] += 1;
    }
  }
}

// Simulate the full bracket
void Bracket::simulate() {
  reset_players(player_library);
  for (std::vector<Round*>::reverse_iterator it = winners.rbegin();
       it != winners.rend(); it++)
    (*it)->simulate();
  for (std::vector<Round*>::reverse_iterator it = losers.rbegin();
       it != losers.rend(); it++)
    (*it)->simulate();
  grands[1]->simulate();
  if (grands[1]->matches[0]->bracket_reset)
    grands[0]->simulate();

  update_player_results();
}
