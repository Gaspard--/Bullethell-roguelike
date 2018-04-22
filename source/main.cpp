#include "Term.hpp"
#include "Output.hpp"

int main()
{
  Term term;
  Logic logic;

  logic.print();
  while (logic.nextInput());
}
