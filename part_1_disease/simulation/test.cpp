#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/doctest.h"
#include "simulation.h"
#include <random>
#include <fstream>

using namespace std;

//Basic property test to ensure Simulation class can be instantiated and run without errors
TEST_CASE("Disease stores basic properties") {
    Disease d("Corona", 8, 0.15);

    CHECK(d.getName()             == "Corona");
    CHECK(d.getDuration()         == 8);
    CHECK(d.getTransmissibility() == doctest::Approx(0.15));
}


//TC for person CLass


//start with checking susceptibilty of person
TEST_CASE("New person starts susceptible") {
    Person p;
    CHECK(p.isSusceptible());
    CHECK_FALSE(p.isInfectious());
    CHECK_FALSE(p.isRecovered());
    CHECK_FALSE(p.isVaccinated());
}
// Chanses of Infection should be 100% if transmissibility is 1.0
TEST_CASE("Susceptible person can be infected") {
    Person p;
    Disease d("Test", 5, 1.0);   // transmissibility 1.0 → infection always happens
    mt19937 rng(42);

    bool infected = p.infect(d, rng);

    CHECK(infected);
    CHECK(p.isInfectious());
}

TEST_CASE("Vaccinated person does not become infectious") {
    Person p;
    p.vaccinate();

    Disease d("Test", 5, 1.0);
    mt19937 rng(42);

    bool infected = p.infect(d, rng);

    CHECK_FALSE(infected);
    CHECK(p.isVaccinated());
    CHECK_FALSE(p.isInfectious());
}
//Recovery after specified duration
TEST_CASE("Person recovers after given duration") {
    int duration = 4;
    Disease d("Test", duration, 1.0);
    Person p;
    mt19937 rng(42);

    p.infect(d, rng);

    for (int day = 0; day < duration; ++day) {
        CHECK(p.isInfectious());
        p.one_more_day();
    }

    CHECK(p.isRecovered());
    CHECK_FALSE(p.isInfectious());
}

TEST_CASE("Vaccination rate is applied roughly correctly") {
    mt19937 rng(123);
    int size = 100;
    double vax_rate = 0.3;

    Population pop("TestCity", size, vax_rate, false);
    Disease d("Test", 5, 0.1);

    pop.reset(d, rng);

    int vax = pop.countVaccinated();
    CHECK(vax == doctest::Approx(size * vax_rate).epsilon(0.2));
}

TEST_CASE("Patient zero exists when has_patient_zero is true") {
    mt19937 rng(123);
    Population pop("TestCity", 200, 0.0, true);
    Disease d("Test", 5, 0.1);

    pop.reset(d, rng);

    CHECK(pop.countInfectious() == 1);
}

TEST_CASE("Total persons count remains constant") {
    mt19937 rng(99);
    int size = 300;
    Population pop("TestCity", size, 0.2, true);
    Disease d("Test", 5, 0.2);

    pop.reset(d, rng);

    for (int day = 0; day < 50; ++day) {
        int total = pop.countSusceptible()
                  + pop.countInfectious()
                  + pop.countRecovered()
                  + pop.countVaccinated();
        CHECK(total == size);

        pop.simulate_one_day(d, rng);
    }
}

// ---------------------- Simulation / CSV test --------------

TEST_CASE("Simulation write_stats_csv writes file without throwing") {
    AggregatedStats stats;
    stats.mean_steps        = 10.0;
    stats.std_steps         = 2.0;
    stats.mean_susceptible  = 5.0;
    stats.std_susceptible   = 1.0;
    stats.mean_recovered    = 20.0;
    stats.std_recovered     = 3.0;
    stats.mean_vaccinated   = 15.0;
    stats.std_vaccinated    = 2.5;

    Simulation sim("disease_in.ini");

    CHECK_NOTHROW(sim.write_stats_csv("test_stats.csv", stats));

    ifstream f("test_stats.csv");
    CHECK(f.good());
}

