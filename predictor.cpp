#include <chrono>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

#include "Bracket.hpp"

int main(int argc, char** argv) {
  int n = 1e5;  // Number of simulations

  int num_W = 32;  // Number of players in winners bracket
  int num_L = 32;  // Number of players in losers bracket

  // Locations in losers bracket where you get sent from winners bracket
  std::vector<std::vector<int>> wl_map;
  wl_map.push_back({0, 1});  // WSF to LQF
  wl_map.push_back({2, 3, 0, 1});  // WQF to LT12
  wl_map.push_back({3, 2, 1, 0, 7, 6, 5, 4});  // WT24 to LT24
  wl_map.push_back({15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1});  // WT48 to LT48

  // Fixed results
  std::vector<std::vector<int>> res_fixed_W, res_fixed_L, res_fixed_G;
  res_fixed_W.push_back({0});  // WF
  res_fixed_W.push_back({0, 0});  // WSF
  res_fixed_W.push_back({0, 0, 0, 0});  // WQF
  res_fixed_W.push_back({0, 0, 0, 0, 0, 0, 0, 0});  // WT24
  res_fixed_W.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});  // WT48

  res_fixed_L.push_back({0});  // LF
  res_fixed_L.push_back({0});  // LSF
  res_fixed_L.push_back({0, 0});  // LQF
  res_fixed_L.push_back({0, 0});  // LT8
  res_fixed_L.push_back({0, 0, 0, 0});  // LT12
  res_fixed_L.push_back({0, 0, 0, 0});  // LT16
  res_fixed_L.push_back({0, 0, 0, 0, 0, 0, 0, 0});  // LT24
  res_fixed_L.push_back({0, 0, 0, 0, 0, 0, 0, 0});  // LT32
  res_fixed_L.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});  // LT48
  res_fixed_L.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});  // LT64

  res_fixed_G.push_back({0});  // GF
  res_fixed_G.push_back({0});  // GF2

  // Load player data from file
  playerLibrary players = load_player_data();

  // Setup the bracket
  Bracket* bracket = new Bracket(num_W, num_L);
  bracket->set_player_library(players);
  bracket->set_structure(wl_map);
  std::vector<Player*> players_in_bracket = bracket->set_initial_players();
  bracket->set_res_fixed(res_fixed_W, res_fixed_L, res_fixed_G);

  std::chrono::high_resolution_clock::time_point start, end;

  // Simulate the bracket n times
  struct winsize console_size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
  int pbarWidth = console_size.ws_col - 8;
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < n; i++) {
    bracket->simulate(true);
    if (n > pbarWidth && i % (n / pbarWidth) == 0) {
      int pos = i * pbarWidth / n;
      int pct = i * 100 / n;
      std::cout << "[" << std::string(pos, '=') << ">" <<
                std::string(pbarWidth - pos - 1, ' ') << "] " << pct << "%\r";
      std::cout.flush();
    }
  }
  std::cout << "[" << std::string(pbarWidth, '=') << "] 100%" << std::endl;
  std::cout << std::endl;
  end = std::chrono::high_resolution_clock::now();

  // Print results
  for (std::vector<Player*>::iterator it = players_in_bracket.begin();
       it != players_in_bracket.end(); it++) {
    (*it)->calc_avg_points();
  }
  std::sort(players_in_bracket.begin(), players_in_bracket.end(), by_avg_points());
  printf("  %-16s%9s%9s%9s%9s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n", "", "Points",
         "1st", "2nd", "3rd", "4th", "5th", "7th",
         "9th", "13th", "17th", "25th", "33rd", "49th");
  for (std::vector<Player*>::iterator it = players_in_bracket.begin();
       it != players_in_bracket.end(); it++) {
    printf("  %-16s  %7.2f", (*it)->name.c_str(), (*it)->avg_points);
    for (int i = 0; i < 12; i++) {
      printf("  %7u", (*it)->placings[i]);
    }
    printf("\n");
  }
  printf("\n");

  // Print timing results
  long dur_ms = std::chrono::  // microseconds
                duration_cast<std::chrono::microseconds>(end - start).count();
  float duration = (float) dur_ms * 1.e-6;
  float sims_per_second = n / duration;
  std::cout << "Number of simulations run: " << n << std::endl;
  std::cout << "Time taken: " << duration << " seconds; "
            << sims_per_second << " per second" << std::endl;

  return 0;
}
