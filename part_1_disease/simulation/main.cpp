#include <iostream>

#include "simulation.h"

int main() {
  std::cout << "Loading Disease Simulation..." << std::endl;
  Simulation sim = Simulation("../disease_in.ini");
  sim.start();
  return 0;
}
