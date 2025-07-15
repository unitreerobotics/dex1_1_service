// Copyright (c) 2025, Unitree Robotics Co., Ltd.
// All rights reserved.

#pragma once

#include <stdint.h>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <memory>
#include <iomanip>


namespace param
{
inline std::string VERSION = "1.0.0";
inline std::filesystem::path proj_dir;

/* ---------- Command Line Parameters ---------- */
namespace po = boost::program_options;

//※ This function must be called at the beginning of main() function
inline po::variables_map helper(int argc, char** argv) 
{
    po::options_description desc("Unitree Dex1-1 Gripper Server");
    desc.add_options()
        ("help,h", "produce help message")
        ("version,v", "show version")
        ("calibration,c", "calibrate the gripper motor")
        ("network,n", po::value<std::string>()->default_value("eth0"), "dds networkInterface")
        ("left,l", "test left dex1 gripper")
        ("right,r", "test right dex1 gripper");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        exit(0);
    }
    if (vm.count("version"))
    {
        std::cout << "Version: " << VERSION << std::endl;
        exit(0);
    }

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    return vm;
}

}