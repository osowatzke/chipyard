#!/bin/bash

# Exit if any command fails
set -e

# Parse arguments
force=0
clean=0
deep=0
testname="hello"
while getopts ":cdfhn:" flag; do
    case $flag in
        c)
            clean=1
            ;;
        d)
            deep=1
            ;;
        f)
            force=1
            ;;
        h)
            echo "Usage $0 [-c] [-d] [-f] [-h] [-n <benchmark_name>]"
            echo "    -c    clean build"
            echo "    -d    deep (use with -c for deep clean)"
            echo "    -f    force (bypass checks for uncommitted changes)"
            echo "    -h    help"
            echo "    -n    benchmark name"
            ;;
        n)
            testname=$OPTARG
            ;;
        :)
            echo "Option -${OPTARG} requires an argument."
            exit 1
            ;;
        *)
            ;;
    esac
done

# Determine git root directory
git_root=$(git rev-parse --show-toplevel)

# Change directory to git root
cd $git_root

# Clean repo
if [ $clean -eq 1 ]; then

    # Ensure the user wants to do a clean
    validInput=0
    clean=0
    while [ $validInput -eq 0 ]; do
        read -p "Do you really want to do a clean build (Y/N): " userInput
        if [ "${userInput,,}" == "y" ]; then
            validInput=1
            clean=1
        elif [ "${userInput,,}" == "n" ]; then
            validInput=1
            clean=0
        else
            echo "Invalid input. Please try again..."
        fi
    done
    
    if [ $clean -eq 1 ]; then
        
        if [ $deep -eq 1 ]; then
        
            # Ensure the user wants to do a deep clean
            validInput=0
            deep=0
            while [ $validInput -eq 0 ]; do
                read -p "Do you really want to do a deep clean. Regular clean is faster (Y/N): " userInput
                if [ "${userInput,,}" == "y" ]; then
                    validInput=1
                    deep=1
                elif [ "${userInput,,}" == "n" ]; then
                    validInput=1
                    deep=0
                else
                    echo "Invalid input. Please try again..."
                fi
            done
        fi
    
        if [ $deep -eq 1 ]; then
        
            # Determine desired conda environment
            conda_env=${git_root}/.conda-env

            # Disable conda environment if active
            if [ "$CONDA_DEFAULT_ENV" == "$conda_env" ]; then
                echo "Deactivating conda environment"
                conda deactivate
            fi

            # Ensure there are no uncommitted changes
            if [ -n "$(git status --porcelain)" ] && [ $force -eq 0 ]; then
                echo "Git repo has uncommitted changes. Please commit before proceeding"
                echo "or run again with the -f flag to discarded uncommitted changes."
                exit 1;
            fi
            
            # Clean git repo
            git clean -xdf

            # Build the RISC-V tools
            ./build-setup.sh -s 6 -s 7 -s 8 -s 9 riscv-tools
            
        else
        
            # Delete build folders
            cd ${git_root}/tests
            rm -f *.riscv
            rm -rf build
            cd ${git_root}/sims/verilator
            rm -f simulator-chipyard*
            rm -rf generated-src
            rm -rf output
            cd $git_root
        fi
    fi
fi

# Source env.sh
source env.sh

# Navigate to tests directory
cd tests

# Build benchmark
cmake -S ./ -B ./build/ -D CMAKE_BUILD_TYPE=Debug
cmake --build ./build/ --target ${testname}

# Navigate to simulation directory
cd ../sims/verilator

# Run test
make CONFIG=CnnHwAcceleratorRocketConfig BINARY=../../tests/${testname}.riscv run-binary