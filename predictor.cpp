#include <chrono>

#ifdef __linux__
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "Bracket.hpp"

int main(int argc, char** argv) {
  // OpenMP setup
  int num_threads;
#ifdef _OPENMP
#pragma omp parallel
#pragma omp single
  num_threads = omp_get_num_threads();
#else
  num_threads = 1;
#endif

  // Number of simulations
  int n = 100000;
  if (argc > 1) {
    try {
      n = std::stoi(argv[1]);
      if (n <= 0)
        throw 1;
    } catch (...) {
      std::string n_str(argv[1]);
      throw_error("Number of simulations = " + n_str +
                  ", must be a positive integer");
    }
  }

  // Load bracket parameters from file
  int num_W, num_L;
  std::vector<std::vector<int>> wl_map, res_fixed_W, res_fixed_L, res_fixed_G;
  load_bracket_params(num_W, num_L, wl_map, res_fixed_W, res_fixed_L, res_fixed_G);

  // Load initial player locations from file
  std::vector<std::string> players_W, players_L;
  load_initial_players(players_W, players_L);

  // Load player data from file
  std::vector<playerLibrary> player_libraries(num_threads);
  player_libraries[0] = load_player_data();
  for (int t = 1; t < num_threads; t++)
    player_libraries[t] = copy_player_library(player_libraries[0]);

  // Setup the bracket
  std::vector<Bracket*> brackets(num_threads);
  for (int t = 0; t < num_threads; t++) {
    brackets[t] = new Bracket(num_W, num_L);
    brackets[t]->set_player_library(player_libraries[t]);
    brackets[t]->set_structure(wl_map);
    brackets[t]->set_initial_players(players_W, players_L, t);
    brackets[t]->set_res_fixed(res_fixed_W, res_fixed_L, res_fixed_G);
  }

  // Progress bar setup
#ifdef PROGRESS_BAR
  int j = 0;
  int pbarWidth, pos_prev, pct_prev;
  {
#if defined _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    pbarWidth = columns - 8;
#elif defined __linux__
    struct winsize console_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &console_size);
    pbarWidth = console_size.ws_col - 8;
#else
    pbarWidth = 100;
#endif
  }
#endif

  std::vector<int> num_sims_per_thread(num_threads);
  std::chrono::high_resolution_clock::time_point start, end;

  // Simulate the bracket n times
  start = std::chrono::high_resolution_clock::now();
#pragma omp parallel for schedule(guided)
  for (int i = 0; i < n; i++) {
    int t = THREAD_NUM;
    brackets[t]->simulate();
    num_sims_per_thread[t] += 1;
#ifdef PROGRESS_BAR
    j += 1;
    if (t == 0) {
      int pos = j * pbarWidth / n;
      int pct = j * 100 / n;
      if (pos != pos_prev || pct != pct_prev) {
        pos_prev = pos;
        pct_prev = pct;
        std::cout << "[" << std::string(pos, '=') << ">" <<
                  std::string(pbarWidth - pos - 1, ' ') << "] " << pct << "%\r";
        std::cout.flush();
      }
    }
#endif
  }
#ifdef PROGRESS_BAR
  std::cout << "[" << std::string(pbarWidth, '=') << "] 100%" << std::endl;
  std::cout << std::endl;
#endif
  end = std::chrono::high_resolution_clock::now();

  // Combine the results from all the threads
  std::vector<Player*> players_in_bracket = brackets[0]->players_in_bracket;
  int num_players = brackets[0]->players_in_bracket.size();
  int num_placings = brackets[0]->players_in_bracket[0]->placings.size();
  for (int i = 0; i < num_players; i++) {
    Player* player = players_in_bracket[i];
    for (int p = 0; p < num_placings; p++)
      for (int t = 1; t < num_threads; t++)
        player->placings[p] += brackets[t]->players_in_bracket[i]->placings[p];
    player->calc_avg_points();
  }

  // Print results
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
  std::cout << "Number run by each thread:" << std::endl;
  for (int t = 0; t < num_threads; t++)
    std::cout << num_sims_per_thread[t] << "  ";
  std::cout << std::endl;

  return 0;
}
