#!/bin/bash
# sudo apt-get install cpufrequtils

# Check if cpufrequtils package is installed

if ! [ -x "$(command -v cpufreq-set)" ]; then
  echo "Error: cpufrequtils package is not installed. Please install it first."
  exit 1
fi

cpu_mode=performance
#cpu_mode=powersave
cmode=`cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`
echo "get cmode=${cmode}"
if [ $cmode = $cpu_mode ]; then
  echo "CPU is already in performance mode."
  exit 1
fi

# Get the number of CPU cores
cpu_cores=$(nproc)
echo "get cpu_cores: $cpu_cores"

# Set performance mode for each CPU core
for cpu in $(seq 0 $((cpu_cores-1)))}
do
  echo "cpufreq-set -c $cpu -g ${cpu_mode}"
  sudo cpufreq-set -c $cpu -g ${cpu_mode}
done

# Verify the current CPU frequency governor
cpufreq-info --policy | grep "current policy"

echo "Curent CPU(${cpu_cores}) are ${cpu_mode}."
# cat /proc/cpuinfo | grep processor | wc -l

