#pragma once

#include <termios.h>
#include <sys/ioctl.h>
#include <ncurses/curses.h>

class Term
{
  struct termios old_terminal;
  SCREEN *screen;

  void	init_colors() noexcept
  {
    start_color();
    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
  }

public:
  Term() noexcept
    : screen(newterm(NULL, stdout, stdin))
  {
    init_colors();
    curs_set(0);
    ioctl(0, TCGETS, &old_terminal);
    raw();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(0);
  }

  Term(Term const &) = delete;
  Term(Term &&) = delete;
  Term operator=(Term const &) = delete;
  Term operator=(Term &&) = delete;

  ~Term() noexcept
  {
    ioctl(0, TCSETS, &old_terminal);
    endwin();
    delscreen(screen);
  }
};
