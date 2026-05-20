#include "simulation.h"

#include "INIReader.h"
#include <iostream>
#include <string>
#include <vector>

Simulation::Simulation(std::string in_file) : input_file(std::move(in_file)) {}

void Simulation::start() {
  std::cout << "Starting simulation..." << std::endl;

  INIReader reader(input_file);
  if (reader.ParseError() < 0) {
    std::cerr << "Error: could not read configuration file '" << input_file << "'" << std::endl;
    return;
  }

  std::string simulation_name = reader.Get("global", "simulation_name", "unknown");
  long num_populations = reader.GetInteger("global", "num_populations", 0);

  std::cout << "Simulation name: " << simulation_name << std::endl;
  std::cout << "Number of populations: " << num_populations << std::endl;

  for (long i = 1; i <= num_populations; ++i) {
    std::string section = "population_" + std::to_string(i);
    long size = reader.GetInteger(section, "size", 0);
    double vaccination_rate = reader.GetReal(section, "vaccination_rate", 0.0);
    std::string patient_0 = reader.Get(section, "patient_0", "false");

    std::cout << "Population " << i << ": size=" << size
              << ", vaccination_rate=" << vaccination_rate
              << ", patient_0=" << patient_0 << std::endl;
  }

  std::cout << "Configuration loaded successfully." << std::endl;
}
