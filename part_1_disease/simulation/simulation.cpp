#include "simulation.h"
#include "INIReader.h"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <numeric>
#include <fstream>


using namespace std;

// for person 
Person::Person() : state_(State::SUSCEPTIBLE), days_left_(0) {}

bool Person::infect(const Disease& disease, mt19937& rng) {
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
    if (state_ == State::SUSCEPTIBLE) {
       state_ = State::VACCINATED;
    }
    state_ = State::VACCINATED;//Vaccination only works before infectio nhappend
}

bool Person::one_more_day() {
   if (state_ != State::INFECTIOUS) {
    return false;
   }

   days_left_--;
   if (days_left_ <= 0) {
    state_ = State::RECOVERED;
    days_left_ = 0;
    return true;
   }
   return false;


}

std::string Person::status_string() const {
    switch (state_) {
        case State::SUSCEPTIBLE:return "Susceptible";
        case State::INFECTIOUS:
            return "Infectious (" + to_string(days_left_) + " days left)";
        case State::RECOVERED:return "Recovered";
        case State::VACCINATED:return "Vaccinated";
        default:return "Unknown";
    }
}


//population
Population::Population(const std::string& name,
                       int size,
                       double vaccination_rate,
                       bool has_patient_zero)
    : name_(name),
      vaccination_rate_(vaccination_rate),
      has_patient_zero_(has_patient_zero),
      people_(size) {}
    // Vaccinate people based on vaccination rate
void Population::reset(const Disease& disease, mt19937& rng) {
    int n = (int)people_.size();

    //reset everyone back to susceptible
    for(auto& p : people_) {
        p = Person();
    }

    // random people to vaccinate
    vector<int> indices(n);
    iota(indices.begin(), indices.end(), 0);
    shuffle(indices.begin(), indices.end(), rng);

    int num_vax = (int)(vaccination_rate_ * n);
    for (int i = 0; i < num_vax; i++) {
        people_[indices[i]].vaccinate();
    }
    //reset counters
    susceptible_count_ = infectious_count_ = recovered_count_ = vaccinated_count_ = 0;
    for (const auto& p : people_) {
        if (p.isSusceptible()) {
            susceptible_count_++;
        } else if (p.isInfectious()) {
            infectious_count_++;
        }
      }
    
    if(has_patient_zero_) {
        // Infect one random person if patient zero is enabled
        Disease seed(disease.getName(), disease.getDuration(), disease.getTransmissibility());
        for(int idx : indices) {
            if (people_[idx].isSusceptible()) {
                people_[idx].infect(seed, rng);
                infectious_count_++;
                susceptible_count_--;
                break;
            }
        }
    }

    //initail infectious indices buffer
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


    //sick person will meet 6 random people each day AS PER INSTRUCTION
    for (int idx : infectious_indices_) {
        for (int i = 0; i < 6; i++) {
            int other_idx = pick(rng);
            if (people_[other_idx].infect(disease, rng)) {
                infectious_count_++;
                susceptible_count_--;
            }
        }
    }

    // Update infection status and counters by  one day
    infectious_indices_.clear();
    for (int i = 0; i < n; i++) {
        if (people_[i].one_more_day()) {
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

    int steps = 0;
    while (infectious_count_ > 0) {

        simulate_one_day(disease, rng);
        steps++;

        // Log daily details
        details_file  << name_ << "," << run_number << "," << 
                         infectious_count_ << "," << recovered_count_ << "," << susceptible_count_ << "," << vaccinated_count_ << "\n";  
    }
  

    PopulationStats stats;
    stats.run = run_number;
    stats.total_steps = steps;
    stats.susceptible = susceptible_count_;
    stats.recovered = recovered_count_;
    stats.vaccinated = vaccinated_count_;
    return stats;                        
    
}




// Simulation class implementation
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

