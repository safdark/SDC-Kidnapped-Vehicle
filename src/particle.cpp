#include "particle.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <cmath>

#include "helper_functions.h"
#include "map.h"

using std::normal_distribution;
using std::numeric_limits;
using std::string;
using std::vector;

#define EPS 0.00001

double Particle::calculateDistance(const Projection& landmark) const {
	double x1 = landmark.readX();
	double y1 = landmark.readY();

	double x2 = this->readY();    // TODO: (shouldn't it be getX() here?)
	double y2 = this->readX();    // ??

	return sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2));
}

Landmarks Particle::getLandmarksWithinRange(double sensor_range, const Landmarks& reference_landmarks) const {
    // Find landmarks in particle's range.
    double sensor_range_2 = sensor_range * sensor_range;
    Landmarks inRangeLandmarks;
    for(unsigned int j = 0; j < reference_landmarks.size(); j++) {
        float landmarkX = reference_landmarks[j].readX();
        float landmarkY = reference_landmarks[j].readY();
        int id = reference_landmarks[j].readId();
        double dX = this->readX() - landmarkX;
        double dY = this->readY() - landmarkY;
        if ( dX*dX + dY*dY <= sensor_range_2 ) {
            inRangeLandmarks.push_back(Landmark(id, landmarkX, landmarkY));
        }
    }
    return inRangeLandmarks;
}

Projections Particle::transformToGlobalPerspective(const Observations& observations) const {
    Projections projections(observations.size());
    for (int i = 0; i < observations.size(); i++) {
        projections[i] = this->getHomogenousTransformation(observations.at(i));
    }
    return projections;
}

Projection Particle::getHomogenousTransformation(const Observation& observation) const {
    Projection transformed_coords;

    transformed_coords.getId() = observation.readId();
    transformed_coords.getX() = observation.readX() * cos(this->theta) - observation.readY() * sin(this->theta) + this->readX();
    transformed_coords.getY() = observation.readX() * sin(this->theta) + observation.readY() * cos(this->theta) + this->readY();

    return transformed_coords;
}

tuple<Projections, Landmarks> Particle::alignObservationsWithClosestLandmarks(const Projections &projections, const Landmarks& landmarks) {
    this->aligned_landmarks.clear();
    this->projections.clear();

    for (unsigned int i = 0; i < projections.size(); i++) { // For each observation
        Projection projection = projections[i];
        double minDistance = std::numeric_limits<double>::max();
        int minLandmarkIdx = -1;

        for (unsigned j = 0; j < landmarks.size(); j++ ) { // For each landmark

            double delta_x = landmarks[i].readX() - projections[j].readX();
            double delta_y = landmarks[i].readY() - projections[j].readY();
            double distance_squared = pow(delta_x, 2) + pow(delta_y, 2);

            // If the "distance" is less than min, stored the id and update min.
            if ( distance_squared < minDistance ) {
                minDistance = distance_squared;
                minLandmarkIdx = j;
            }
        }

        if (minLandmarkIdx > -1) {
            this->projections.push_back(projection);
            this->aligned_landmarks.push_back(landmarks.at(minLandmarkIdx));
        }
    }

    return make_tuple(this->projections, this->aligned_landmarks);
}

void Particle::move(double delta_t, double velocity, double yaw_rate) {
    if (fabs(yaw_rate) < EPS) {
        // We treat this as a zero yaw-rate:
        this->getX() += velocity * delta_t * cos(this->getTheta());
        this->getY() += velocity * delta_t * sin(this->getTheta());
        this->getTheta() = this->getTheta();  // Since yaw-rate is zero, this remains the same
    } else {
        // We treat this as a non-zero yaw-rate
        this->getX() += velocity / yaw_rate * ( sin( this->getTheta() + yaw_rate * delta_t ) - sin( this->getTheta() ) );
        this->getY() += velocity / yaw_rate * ( cos( this->getTheta() ) - cos( this->getTheta() + yaw_rate * delta_t ) );
        this->getTheta() += yaw_rate * delta_t;
    }
}