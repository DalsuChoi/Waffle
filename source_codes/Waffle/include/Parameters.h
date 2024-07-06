#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#define NUM_WAFFLE_KNOBS 5

// Minimum and maximum values of each knob
static int MIN_nCell_space_lat = 100;
static int MAX_nCell_space_lat = 2000;
static int MIN_nCell_space_lon = 100;
static int MAX_nCell_space_lon = 2000;
static int MIN_MOPC = 2;
static int MAX_MOPC = 20;
static int MIN_nCell_chunk_lat = 2;
static int MAX_nCell_chunk_lat = 20;
static int MIN_nCell_chunk_lon = 2;
static int MAX_nCell_chunk_lon = 20;

// Hyperparameters that will be automatically tuned.
static std::vector<double> hyper_lr = {0.0005, 0.005};
static std::vector<int> hyper_batch = {32, 64, 128};
static std::vector<int> hyper_candidates = {500, 1500};
static std::vector<int> hyper_T = {1, 10};
static std::vector<int> hyper_recent = {200, 300};

// Predefined hyperparameters
const int nCell_state_lat = 32;
const int nCell_state_lon = 32;
#define ADAM_WEIGHT_DECAY 0
#define PER_ALPHA 0.6
#define MAX_MEMORY_SIZE 100000

const double DOUBLE_MAX = std::numeric_limits<double>::max();
const double DOUBLE_MIN = std::numeric_limits<double>::lowest();
const int INTEGER_MAX = std::numeric_limits<int>::max();
const int INTEGER_MIN = std::numeric_limits<int>::lowest();
const double SMALL_NUMBER = std::pow(10, -6);
#define EXCLUDE_TOO_EARLY_EXPERIENCE 1

// Query type
#define QUERYTYPE_TERMINATE -1
#define QUERYTYPE_INSERTION 1
#define QUERYTYPE_DELETION 2
#define QUERYTYPE_RANGE 3
#define QUERYTYPE_KNN 4
#define QUERYTYPE_SETSPACE 5

// Index type
#define ORIGINAL_INDEX 0
#define NEW_INDEX 1

// Lock type
#define EXCLUSIVE_LOCK true
#define SHARED_LOCK false

typedef int IDType;
typedef std::pair<int, int> CellCoordinate;
typedef unsigned long long uint64;

#endif