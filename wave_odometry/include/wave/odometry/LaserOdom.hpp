#ifndef WAVE_LASERODOM_HPP
#define WAVE_LASERODOM_HPP

#include <vector>
#include <array>
#include <algorithm>
#include <utility>
#include <chrono>
#include <limits>
#include <iostream>
#include <fstream>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include "wave/matching/pointcloud_display.hpp"
#include "wave/odometry/kdtreetype.hpp"
#include "wave/odometry/PointXYZIR.hpp"
#include "wave/odometry/PointXYZIT.hpp"
#include "wave/odometry/laser_odom_residuals.hpp"
#include "wave/containers/measurement_container.hpp"
#include "wave/containers/measurement.hpp"
#include "wave/utils/math.hpp"

namespace wave {

using IMUMeasurement = Measurement<Vec6, char>;
using unlong = unsigned long;

struct LaserOdomParams {
    // Optimizer parameters
    int opt_iters = 25;     // How many times to refind correspondences
    float diff_tol = 1e-6;  // norm of transform vector must change by more than this to continue
    float huber_delta = 0.4;
    float max_correspondence_dist = 1;  // correspondences greater than this are discarded
    double rotation_stiffness = 1e-4;
    double translation_stiffness = 1e-3;
    double T_z_multiplier = 1;
    double T_y_multiplier = 1;
    double RP_multiplier = 1;

    // Sensor parameters
    float scan_period = 0.1;  // Seconds
    int max_ticks = 36000;    // encoder ticks per revolution
    unlong n_ring = 32;       // number of laser-detector pairs

    // Feature extraction parameters
    float occlusion_tol = 0.1;   // Don't know units
    float parallel_tol = 0.002;  // ditto
    float keypt_radius = 0.05;   // m2
    float edge_tol = 0.1;        // Edge features must have score higher than this
    float flat_tol = 0.1;        // Plane features must have score lower than this
    int n_edge = 40;             // How many edge features to pick out per ring
    int n_flat = 100;            // How many plane features to pick out per ring
    unlong knn = 5;              // 1/2 nearest neighbours for computing curvature

    // Setting flags
    bool imposePrior = false;             // Whether to add a prior constraint on transform from the previous scan match
    bool visualize = false;               // Whether to run a visualization for debugging
    bool output_trajectory = false;       // Whether to output solutions for debugging/plotting
    bool output_correspondences = false;  // Whether to output correpondences for debugging/plotting
};

class LaserOdom {
 public:
    LaserOdom(const LaserOdomParams params);
    ~LaserOdom();
    void addPoints(const std::vector<PointXYZIR> &pts, const int tick, TimeType stamp);
    void addIMU(std::vector<double> linacc, Quaternion orientation);
    std::vector<std::vector<PointXYZIT>> edges, flats;
    std::vector<FeatureKDTree<double>> prv_edges, prv_flats;
    // The transform is T_start_end
    std::array<double, 3> cur_translation, prev_translation;
    std::array<double, 3> cur_rotation, prev_rotation;
    bool new_features = false;

    void rollover(TimeType stamp);
    bool match();
    void registerOutputFunction(std::function<void(const TimeType &,
                                                   const std::array<double, 3> &,
                                                   const std::array<double, 3> &,
                                                   const pcl::PointCloud<pcl::PointXYZI> &)> output_function);

 private:
    // Visualizer elements, not allocated unless used
    PointCloudDisplay *display;
    pcl::PointCloud<pcl::PointXYZI>::Ptr prev_viz, cur_viz;
    void updateViz();
    // Output trajectory file
    std::ofstream file;

    // Flow control
    std::atomic_bool continue_output;
    bool fresh_output = false;
    std::mutex output_mutex;
    std::condition_variable output_condition;
    std::unique_ptr<std::thread> output_thread;
    void spinOutput();
    std::function<void()> f_output;
    void undistort();
    // Shared memory
    pcl::PointCloud<pcl::PointXYZI> undistorted_cld;
    TimeType undistorted_stamp;
    std::array<double, 3> undistort_translation, undistort_rotation;

    LaserOdomParams param;
    bool initialized = false;
    int prv_tick = std::numeric_limits<int>::max();
    std::vector<double> scale_lookup;

    void computeCurvature();
    void prefilter();
    void generateFeatures();
    void buildTrees();
    bool findCorrespondingPoints(const Vec3 &query,
                                 const uint16_t knn,
                                 const uint16_t k_per_ring,
                                 const bool searchPlanarPoints,
                                 std::vector<uint16_t> *rings,
                                 std::vector<size_t> *index);

    PCLPointXYZIT applyIMU(const PCLPointXYZIT &pt);

    // store for the IMU integral
    MeasurementContainer<IMUMeasurement> imu_trans;

    std::vector<double> lin_vel = {0, 0, 0};
    TimeType prv_time, cur_time;
    static float l2sqrd(const PCLPointXYZIT &p1, const PCLPointXYZIT &p2);
    static float l2sqrd(const PCLPointXYZIT &pt);
    static PCLPointXYZIT scale(const PCLPointXYZIT &pt, const float scale);
    void flagNearbyPoints(const unlong ring, const unlong index);
    std::vector<std::vector<std::pair<bool, float>>> cur_curve;
    std::vector<std::vector<std::pair<unlong, float>>> filter;
    std::vector<pcl::PointCloud<PCLPointXYZIT>> cur_scan;
    std::vector<kd_tree_t *> edge_idx;
    std::vector<kd_tree_t *> flat_idx;
};

}  // namespace wave

#endif  // WAVE_LASERODOM_HPP