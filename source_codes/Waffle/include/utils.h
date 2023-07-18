#ifndef UTILS_H
#define UTILS_H

#include "Parameters.h"

#define KNN_Q_MEMBER_OBJECT 2
#define KNN_Q_MEMBER_CELL 1
#define KNN_Q_MEMBER_LEVEL 0

struct kNN_q_member
{

    kNN_q_member(double query_lat, double query_lon, double object_lat, double object_lon, ID_TYPE id)
    {
        type = KNN_Q_MEMBER_OBJECT;
        real_lat = object_lat;
        real_lon = object_lon;
        kNN_q_member::id = id;
        kNN_q_member::distance = pow(object_lat - query_lat, 2) + pow(object_lon - query_lon, 2);
    }

    kNN_q_member(double query_lat, double query_lon, int cell_lat, int cell_lon, double distance)
    {
        type = KNN_Q_MEMBER_CELL;
        kNN_q_member::cell_lat = cell_lat;
        kNN_q_member::cell_lon = cell_lon;
        kNN_q_member::distance = distance;
    }

    kNN_q_member(double distance, int type)
    {
        kNN_q_member::type = type;
        kNN_q_member::distance = distance;
    }

    double distance;
    int type;
    ID_TYPE id;
    int cell_lat;
    int cell_lon;
    double real_lat;
    double real_lon;
};

struct compare_kNN
{
    bool operator()(kNN_q_member const &p1, kNN_q_member const &p2)
    {
        return p1.distance > p2.distance;
    }
};

struct Hyperparameter
{
    Hyperparameter(double, double, int, int, double, int);

    double reward;
    double lr;
    int batch_size;
    int candidates;
    double T;
    int recent;

    bool operator<(const Hyperparameter &rhs) const
    {
        return reward < rhs.reward; // max heap
    }
};

#endif