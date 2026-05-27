#include "simulation.h"
#include "INIReader.h"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <numeric>
#include <fstream>


using namespace std;

// --------------- Person ---------------

Person::Person() : state_(State::SUSCEPTIBLE), days_left_(0) {}

bool Person::infect(const Disease& disease, mt19937& rng) {
    // Only susceptible people can be infected
    if (state_ != State::SUSCEPTIBLE) {
        return false;
    }

    uniform_real_distribution<double> roll(0.0, 1.0);
    if (roll(rng) < disease.getTransmissibility()) {
        state_     = State::INFECTIOUS;
        days_left_ = disease.getDuration();
        return true;
    }

    return false;
}

void Person::vaccinate() {
    // Only vaccinate if the person is still susceptible
    if (state_ == State::SUSCEPTIBLE) {
        state_ = State::VACCINATED;
    }
}

bool Person::one_more_day() {
    // Only infectious people progress toward recovery
    if (state_ != State::INFECTIOUS) {
        return false;
    }

    days_left_--;
    if (days_left_ <= 0) {
        // Person has recovered and is now immune
        state_    = State::RECOVERED;
        days_left_ = 0;
        return true;
    }
    return false;
}

std::string Person::status_string() const {
    switch (state_) {
        case State::SUSCEPTIBLE: return "Susceptible";
        case State::INFECTIOUS:
            return "Infectious (" + to_string(days_left_) + " days left)";
        case State::RECOVERED:  return "Recovered";
        case State::VACCINATED: return "Vaccinated";
        default:                return "Unknown";
    }
}


// --------------- Population ---------------

Population::Population(const std::string& name,
                       int size,
                       double vaccination_rate,
                       bool has_patient_zero)
    : name_(name),
      vaccination_rate_(vaccination_rate),
      has_patient_zero_(has_patient_zero),
      people_(size) {}

void Population::reset(const Disease& disease, mt19937& rng) {
    int n = (int)people_.size();

    // Reset every person back to susceptible
    for (auto& p : people_) {
        p = Person();
    }

    // Shuffle indices so vaccination is assigned randomly
    vector<int> indices(n);
    iota(indices.begin(), indices.end(), 0);
    shuffle(indices.begin(), indices.end(), rng);

    // Vaccinate the required fraction of the population
    int num_vax = (int)(vaccination_rate_ * n);
    for (int i = 0; i < num_vax; i++) {
        people_[indices[i]].vaccinate();
    }

    // Reset all counters to zero before recounting
    susceptible_count_ = 0;
    infectious_count_  = 0;
    recovered_count_   = 0;
    vaccinated_count_  = 0;

    // Count each person's current state
    for (const auto& p : people_) {
        if      (p.isSusceptible()) susceptible_count_++;
        else if (p.isInfectious())  infectious_count_++;
        else if (p.isVaccinated())  vaccinated_count_++;
    }

    // FIX: Patient zero must be force-infected (transmissibility=1.0).
    // Previously the code used the disease's own transmissibility probability,
    // which could randomly SKIP infection and leave infectious_count_=0.
    // That caused the simulation to exit immediately on day 0 without
    // ever running — effectively an "empty" run rather than a real one.
    // We guarantee exactly one infectious person at the start.
    if (has_patient_zero_) {
        Disease seed(disease.getName(),
                     disease.getDuration(),
                     1.0);  // transmissibility=1.0 ensures patient zero IS infected
        for (int idx : indices) {
            if (people_[idx].isSusceptible()) {
                people_[idx].infect(seed, rng);
                infectious_count_++;
                susceptible_count_--;
                break;  // Only one patient zero
            }
        }
    }

    // Build the initial list of infectious people for the first day
    infectious_indices_.clear();
    for (int i = 0; i < n; i++) {
        if (people_[i].isInfectious()) {
            infectious_indices_.push_back(i);
        }
    }
}

void Population::simulate_one_day(const Disease& disease, mt19937& rng) {
    int n = (int)people_.size();
    uniform_int_distribution<int> pick(0, n - 1);

    // Each infectious person contacts exactly 6 random people per day (as per instructions)
    for (int idx : infectious_indices_) {
        for (int i = 0; i < 6; i++) {
            int other_idx = pick(rng);
            if (people_[other_idx].infect(disease, rng)) {
                infectious_count_++;
                susceptible_count_--;
            }
        }
    }

    // Advance every person by one day and rebuild the infectious index buffer
    infectious_indices_.clear();
    for (int i = 0; i < n; i++) {
        if (people_[i].one_more_day()) {
            // Person just recovered
            recovered_count_++;
            infectious_count_--;
        }
        if (people_[i].isInfectious()) {
            infectious_indices_.push_back(i);
        }
    }
}

PopulationStats Population::run_simulation(const Disease& disease,
                                           int run_number,
                                           mt19937& rng,
                                           ofstream& details_file) {
    reset(disease, rng);

    // Safety limit: prevents runaway simulations if counters ever
    // desynchronise.  Real simulations always finish well below this.
    const int MAX_STEPS = 100000;

    int steps = 0;
    while (infectious_count_ > 0 && steps < MAX_STEPS) {

        simulate_one_day(disease, rng);
        steps++;

        // Log state after each day
        details_file << name_         << ","
                     << run_number    << ","
                     << infectious_count_  << ","
                     << recovered_count_   << ","
                     << susceptible_count_ << ","
                     << vaccinated_count_  << "\n";
    }

    if (steps >= MAX_STEPS) {
        cerr << "WARNING: " << name_
             << " hit the safety step limit (" << MAX_STEPS
             << "). Results may be incomplete." << endl;
    }

    PopulationStats stats;
    stats.run         = run_number;
    stats.total_steps = steps;
    stats.susceptible = susceptible_count_;
    stats.recovered   = recovered_count_;
    stats.vaccinated  = vaccinated_count_;
    return stats;
}


// --------------- Simulation ---------------

Simulation::Simulation(std::string in_file) : input_file(std::move(in_file)) {}

double Simulation::compute_mean(const std::vector<int>& values) {
    if (values.empty()) {
        return 0.0;
    }
    return accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double Simulation::compute_std(const std::vector<int>& values, double mean) {
    if (values.size() < 2) {
        return 0.0;
    }
    double sum_sq_diff = 0.0;
    for (int v : values) {
        sum_sq_diff += (v - mean) * (v - mean);
    }
    return sqrt(sum_sq_diff / (values.size() - 1));
}

void Simulation::write_stats_csv(const string& path,
                                  const AggregatedStats& stats) const {
    ofstream f(path);
    // Write aggregated statistics in the exact format required by the spec
    f << "key,mean,std\n"
      << "total_steps,"         << stats.mean_steps       << "," << stats.std_steps       << "\n"
      << "susceptible_persons," << stats.mean_susceptible  << "," << stats.std_susceptible << "\n"
      << "recovered_persons,"   << stats.mean_recovered    << "," << stats.std_recovered   << "\n"
      << "vaccinated_persons,"  << stats.mean_vaccinated   << "," << stats.std_vaccinated  << "\n";
}

void Simulation::start() {
    cout << "Starting simulation..." << endl;

    INIReader reader(input_file);
    if (reader.ParseError() < 0) {
        cerr << "Error reading config file: " << input_file << endl;
        return;
    }

    // Read global configuration
    string sim_name = reader.Get("global", "simulation_name", "unknown");
    long   num_pops = reader.GetInteger("global", "num_populations", 0);
    long   sim_runs = reader.GetInteger("global", "simulation_runs", 1);

    // Read disease parameters
    Disease disease(
        reader.Get    ("disease", "name",             "Unknown"),
        reader.GetInteger("disease", "duration",       8),
        reader.GetReal   ("disease", "transmissibility", 0.15)
    );

    // Read all population sections
    vector<Population> populations;
    for (long i = 1; i <= num_pops; i++) {
        string sec = "population_" + to_string(i);
        populations.emplace_back(
            reader.Get        (sec, "name",             "Pop" + to_string(i)),
            (int)reader.GetInteger(sec, "size",          1000),
            reader.GetReal    (sec, "vaccination_rate",  0.0),
            reader.GetBoolean (sec, "patient_0",         false)
        );
    }

    // FIX: CSV header had a typo "susceptiple" — corrected to "susceptible"
    ofstream details_file("disease_details.csv");
    details_file << "name,run,infectious,recovered,susceptible,vaccinated\n";

    mt19937 rng(random_device{}());

    vector<int> all_steps, all_susc, all_rec, all_vax;

    for (long run = 0; run < sim_runs; run++) {
        int run_steps = 0, run_susc = 0, run_rec = 0, run_vax = 0;

        for (auto& pop : populations) {
            PopulationStats s = pop.run_simulation(disease, (int)run, rng, details_file);
            run_steps = max(run_steps, s.total_steps);
            run_susc += s.susceptible;
            run_rec  += s.recovered;
            run_vax  += s.vaccinated;
        }

        all_steps.push_back(run_steps);
        all_susc.push_back(run_susc);
        all_rec.push_back(run_rec);
        all_vax.push_back(run_vax);

        // Print per-run summary to stdout as required
        cout << "Run "       << run       << "  steps="    << run_steps
             << "  recovered=" << run_rec   << "  susceptible=" << run_susc
             << "  vaccinated=" << run_vax  << endl;
    }

    // Compute aggregated statistics across all runs
    AggregatedStats agg;
    agg.mean_steps       = compute_mean(all_steps);
    agg.std_steps        = compute_std (all_steps,  agg.mean_steps);
    agg.mean_susceptible = compute_mean(all_susc);
    agg.std_susceptible  = compute_std (all_susc,   agg.mean_susceptible);
    agg.mean_recovered   = compute_mean(all_rec);
    agg.std_recovered    = compute_std (all_rec,    agg.mean_recovered);
    agg.mean_vaccinated  = compute_mean(all_vax);
    agg.std_vaccinated   = compute_std (all_vax,    agg.mean_vaccinated);

    write_stats_csv("disease_stats.csv", agg);

    // Print final summary statistics to stdout as required by the spec
    cout << "\n=== Final Summary ==="           << endl;
    cout << "Mean timesteps until extinction : " << agg.mean_steps       << " (std=" << agg.std_steps       << ")" << endl;
    cout << "Mean vaccinated persons          : " << agg.mean_vaccinated  << " (std=" << agg.std_vaccinated  << ")" << endl;
    cout << "Mean recovered persons           : " << agg.mean_recovered   << " (std=" << agg.std_recovered   << ")" << endl;
    cout << "Mean susceptible persons         : " << agg.mean_susceptible << " (std=" << agg.std_susceptible << ")" << endl;
    cout << "Done!" << endl;
}
