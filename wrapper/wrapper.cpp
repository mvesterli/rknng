#include <cstdint>
#include <cstdlib>
#include <cstdbool>

#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>

// Order matters
#include "timer.h"
#include "util.h"
#include "globals.h"
#include "dataset.h"
#include "knngraph.h"
kNNGraph* g_ground_truth = nullptr;
#include "recall.h"
#include "rp_div.h"

extern "C" {

// Number of neighbors
static unsigned int K = 10;

// Size when to use brute force
int window_width = 20;

// Stop entirely when the fraction of neighbors updated during a single iteration falls
// below this treshold
float delta = 0.01;

// When the fraction of neighbors updated by a single update of rpdiv falls
// below this treshold, start running nndescent
float nndes_start = 0.1;

int max_iterations = 100;

bool configure(const char* var, const char* val) {
  if (strcmp(var, "count") == 0) {
    char* end;
    errno = 0;
    unsigned long long k = strtoull(val, &end, 10);
    if (errno != 0 || *val == 0 || *end != 0 || k < 0) {
      return false;
    } else {
      K = k;
      return true;
    }
  }  else if (strcmp(var, "window_width") == 0) {
    char* end;
    errno = 0;
    unsigned long long k = strtoull(val, &end, 10);
    if (errno != 0 || *val == 0 || *end != 0 || k < 0) {
      return false;
    } else {
      window_width = k;
      return true;
    }
  } else if (strcmp(var, "delta") == 0) {
    char* end;
    errno = 0;
    unsigned long long k = strtof(val, &end);
    if (errno != 0 || *val == 0 || *end != 0 || k < 0) {
      return false;
    } else {
      delta = k;
      return true;
    }
  } else if (strcmp(var, "nndes_start") == 0) {
    char* end;
    errno = 0;
    float k = strtof(val, &end);
    if (errno != 0 || *val == 0 || *end != 0 || k < 0) {
        return false;
    } else {
        nndes_start = k;
        return true;
    }
  } else return false;
}

bool end_configure(void) {
  return true;
}

static std::vector<std::vector<float>> pointset;

std::vector<float> parseEntry(const char* entry) {
  std::vector<float> e;
  std::string line(entry);
  float x;
  auto sstr = std::istringstream(line);
  while (sstr >> x) {
    e.push_back(x);
  }
  return e;
}

bool train(const char* entry) {
  auto parsed_entry = parseEntry(entry);
  pointset.push_back(parsed_entry);
  return true;
}

DataSet* dataset;
static kNNGraph* graph;


void end_train(void) {
    size_t d = pointset[0].size();
    dataset = init_DataSet(pointset.size(), d);
    for (size_t dataset_idx = 0; dataset_idx < pointset.size(); dataset_idx++) {
        for (size_t point_idx = 0; point_idx < d; point_idx++) {
            set_val(dataset, dataset_idx, point_idx, pointset[dataset_idx][point_idx]);
        }
    }

    graph = rpdiv_create_knng(
        dataset, dataset, K, window_width, delta, nndes_start, max_iterations);
    pointset.clear();
    pointset.shrink_to_fit();
}

bool prepare_query(const char* entry) {
    return false;
}

static int query_id;
static int position;

size_t query(const char* entry, size_t k) {
  position = 0;
  auto sstr = std::istringstream(std::string(entry));
  sstr >> query_id;
  return graph->list[query_id].size;
}

size_t query_result(void) {
  if (position < graph->list[query_id].size) {
    auto res = get_kNN_item_id(graph, query_id, position);
    position++;
    return res;
  } else return SIZE_MAX;
}

void end_query(void) {
  free_DataSet(dataset);
  free_kNNGraph(graph);
}

}
