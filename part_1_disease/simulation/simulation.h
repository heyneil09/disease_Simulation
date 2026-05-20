
#ifndef HPC_DISEASE_SIMULATION_H_
#define HPC_DISEASE_SIMULATION_H_

#include <fstream>
#include <random>
#include <string>
#include <utility>
#include <vector>

// Represents the disease used in the simulation
class Disease {
public:
    // Default constructor
    Disease() = default;

    // Create disease with configurable properties
    Disease(const std::string& name,
            int duration,
            double transmissibility)
        : name_(name),
          duration_(duration),
          transmissibility_(transmissibility) {}

    std::string getName() const {
        return name_;
    }

    int getDuration() const {
        return duration_;
    }

    double getTransmissibility() const {
        return transmissibility_;
    }

private:
    std::string name_ = "Unknown";

    // Number of days person remains infected
    int duration_ = 0;

    // Probability of disease transmission
    double transmissibility_ = 0.0;
};

// Represents one person in the population
class Person {
public:
    enum class State {
        SUSCEPTIBLE,
        INFECTIOUS,
        RECOVERED,
        VACCINATED
    };

    Person();

    // Infect person if currently susceptible
    bool infect(const Disease& disease,
                std::mt19937& rng);

    // Vaccinate person before simulation starts
    void vaccinate();

    // Simulate one more day of infection
    bool one_more_day();

    bool isSusceptible() const {
        return state_ == State::SUSCEPTIBLE;
    }

    bool isInfectious() const {
        return state_ == State::INFECTIOUS;
    }

    bool isRecovered() const {
        return state_ == State::RECOVERED;
    }

    bool isVaccinated() const {
        return state_ == State::VACCINATED;
    }

    std::string status_string() const;

private:
    State state_ = State::SUSCEPTIBLE;

    // Remaining infected days
    int days_left_ = 0;
};

// Stores statistics from one simulation run
struct PopulationStats {
    int run = 0;

    int total_steps = 0;

    int susceptible = 0;

    int recovered = 0;

    int vaccinated = 0;
};

// Handles complete population behaviour
class Population {
public:
    Population(const std::string& name,
               int size,
               double vaccination_rate,
               bool has_patient_zero);

    // Run one full disease simulation
    PopulationStats run_simulation(
        const Disease& disease,
        int run_number,
        std::mt19937& rng,
        std::ofstream& details_file);

    // Cached counters improve performance
    int countSusceptible() const {
        return susceptible_count_;
    }

    int countInfectious() const {
        return infectious_count_;
    }

    int countRecovered() const {
        return recovered_count_;
    }

    int countVaccinated() const {
        return vaccinated_count_;
    }

    std::string getName() const {
        return name_;
    }

    int getSize() const {
        return static_cast<int>(people_.size());
    }

private:
    std::string name_;

    std::vector<Person> people_;

    double vaccination_rate_;

    bool has_patient_zero_;

    // Live counters avoid rescanning population
    int susceptible_count_ = 0;

    int infectious_count_ = 0;

    int recovered_count_ = 0;

    int vaccinated_count_ = 0;

    // Reused buffer for infected people
    std::vector<int> infectious_indices_;

    // Reset population before each run
    void reset(const Disease& disease,
               std::mt19937& rng);

    // Simulate one full day
    void simulate_one_day(const Disease& disease,
                          std::mt19937& rng);
};

// Stores aggregated statistics across runs
struct AggregatedStats {
    double mean_steps = 0.0;

    double std_steps = 0.0;

    double mean_susceptible = 0.0;

    double std_susceptible = 0.0;

    double mean_recovered = 0.0;

    double std_recovered = 0.0;

    double mean_vaccinated = 0.0;

    double std_vaccinated = 0.0;
};

// Main simulation controller
class Simulation {
public:
    // Load configuration file
    explicit Simulation(
        std::string in_file = "disease_in.ini");

    // Start all simulation runs
    void start();

private:
    // Configuration file path
    std::string input_file;

    // Save aggregated statistics into CSV
    void write_stats_csv(
        const std::string& path,
        const AggregatedStats& stats) const;

    // Calculate average value
    static double compute_mean(
        const std::vector<int>& values);

    // Calculate standard deviation
    static double compute_std(
        const std::vector<int>& values,
        double mean);
};

#endif // HPC_DISEASE_SIMULATION_H_
