#ifndef UTILS_H
#define UTILS_H

#include <Eigen/Dense>

struct RealCoord {
    RealCoord()=default;
    RealCoord(double lat, double lon) {
        RealCoord::lat = lat;
        RealCoord::lon = lon;
    }

    double lat;
    double lon;

    bool operator==(const RealCoord &right) const{
        return (this->lat == right.lat && this->lon == right.lon);
    }
    bool operator!=(const RealCoord &right) const{
        return !(operator==(right));
    }
    bool operator<(const RealCoord &right) const{
        if(this->lat < right.lat){
            return true;
        }
        else if(this->lat == right.lat && this->lon < right.lon){
            return true;
        }
        else {
            return false;
        }
    }
};

struct Object {
    int start_node_idx;
    int end_node_idx;
    RealCoord current;
    bool deleted;
};

struct normal_random_variable
{
    normal_random_variable(Eigen::MatrixXd const& covar)
            : normal_random_variable(Eigen::VectorXd::Zero(covar.rows()), covar)
    {}

    normal_random_variable(Eigen::VectorXd const& mean, Eigen::MatrixXd const& covar)
            : mean(mean)
    {
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver(covar);
        transform = eigenSolver.eigenvectors() * eigenSolver.eigenvalues().cwiseSqrt().asDiagonal();
    }

    Eigen::VectorXd mean;
    Eigen::MatrixXd transform;

    Eigen::VectorXd operator()() const
    {
        static std::mt19937 gen{ std::random_device{}() };
        static std::normal_distribution<> dist;

        return mean + transform * Eigen::VectorXd{ mean.size() }.unaryExpr([&](auto x) { return dist(gen); });
    }
};

#endif