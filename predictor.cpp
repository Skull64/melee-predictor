#include <chrono>
#include <string>
#include <vector>

#if defined __linux__
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined _WIN32
#include <windows.h>
#endif

#include "Bracket.hpp"

int main(int argc, char** argv) {
  // Number of simulations
  int n = 100000;
  if (argc > 1)
    n = std::stoi(argv[1]);

  // Load bracket parameters from file
  int num_W, num_L;
  std::vector<std::vector<int>> wl_map, res_fixed_W, res_fixed_L, res_fixed_G;
  load_bracket_params(num_W, num_L, wl_map, res_fixed_W, res_fixed_L, res_fixed_G);

  // Load player data from file
  playerLibrary players = load_player_data();

  // Load initial player locations from file
  std::vector<std::string> players_W, players_L;
  load_initial_players(players_W, players_L);

  // Setup the bracket
  Bracket* bracket = new Bracket(num_W, num_L);
  bracket->set_player_library(players);
  bracket->set_structure(wl_map);
  std::vector<Player*> players_in_bracket = bracket->set_initial_players(players_W, players_L);
  bracket->set_res_fixed(res_fixed_W, res_fixed_L, res_fixed_G);

  std::chrono::high_resolution_clock::time_point start, end;

  // Progress bar setup
  int pbarWidth;
  {
#if defined __linux__
    struct winsize console_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
    pbarWidth = console_size.ws_col - 8;
#elif defined _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    pbarWidth = columns - 8;
#else
    pbarWidth = 100;
#endif
  }

  // Simulate the bracket n times
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
